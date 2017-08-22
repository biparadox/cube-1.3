#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>

#include "data_type.h"
#include "errno.h"
#include "alloc.h"
#include "string.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "message.h"
#include "ex_module.h"
#include "tesi.h"

#include "file_struct.h"
#include "tesi_key.h"
#include "tesi_aik_struct.h"
#include "aik_casign.h"

static struct timeval time_val={0,50*1000};
static char passwd[DIGEST_SIZE];

struct aik_proc_pointer
{
	RSA *  cakey;
	TSS_HKEY hCAKey;
};

int load_policy_pubek(char *pubek_name)
{
	struct vTPM_publickey *pubkey;
        BYTE digest[DIGEST_SIZE];
        char buffer[DIGEST_SIZE*2];
	int retval;
	int len;
	char filename[256];

	pubkey=malloc(sizeof(struct vTPM_publickey));
        if(pubkey==NULL)
        {
                return -ENOMEM;
        }
	snprintf(filename,DIGEST_SIZE*2,"%s.pem",pubek_name);
        memset(pubkey,0,sizeof(struct vTPM_publickey));
        calculate_sm3(pubek_name,digest);

        memcpy(pubkey->uuid,digest,DIGEST_SIZE);
        pubkey->ispubek=1;
	len=sizeof(char)*Strlen(pubek_name);
	// we must add the '\0' as the name's end
	pubkey->key_filename=(char *)malloc(len+1);
        memcpy(pubkey->key_filename,pubek_name,len+1);
	retval=memdb_store(pubkey,DTYPE_TESI_KEY_STRUCT,SUBTYPE_PUBLIC_KEY,NULL);

	return retval;
}

int public_key_memdb_init()
{
	TSS_RESULT * result;
	int ret;
	int retval;
	char * pubek_dirname="pubek";
	char namebuf[512];
	DIR * pubek_dir;

	// open the pubek's dir
	pubek_dir=opendir(pubek_dirname);
	if(pubek_dir==NULL)
	{
		return -EINVAL;
	}
	struct dirent * dentry;

	while((dentry=readdir(pubek_dir))!=NULL)
	{
		if(dentry->d_type !=DT_REG)
			continue;
		// check if file's tail is string ".pem"
		int namelen=strlen(dentry->d_name);
		if(namelen<=4)
			continue;
		char * tail=dentry->d_name+namelen-4;
		if(strcmp(tail,".pem")!=0)
			continue;
		strcpy(namebuf,pubek_dirname);
		strcat(namebuf,"/");
		strncat(namebuf,dentry->d_name,256);

		retval=load_policy_pubek(namebuf);
		if(IS_ERR(retval))
			return retval;
		printf("load pubek %s succeed!\n",namebuf);
	}
	return 0;
}
int aik_casign_init(void * sub_proc,void * para)
{
	int ret;
	TSS_RESULT result;	
	BYTE local_uuid[DIGEST_SIZE];
	
	struct aik_proc_pointer * aik_pointer;
	aik_pointer= malloc(sizeof(struct aik_proc_pointer));
	if(aik_pointer==NULL)
		return -ENOMEM;
	memset(aik_pointer,0,sizeof(struct aik_proc_pointer));

	struct init_struct * init_para=para;
	if(para==NULL)
		return -EINVAL;
	Memset(passwd,0,DIGEST_SIZE);
	strncpy(passwd,init_para->passwd,DIGEST_SIZE);

	OpenSSL_add_all_algorithms();
        ERR_load_crypto_strings();
	result=TESI_Local_Reload();
	if(result!=TSS_SUCCESS)
	{
		printf("open tpm error %d!\n",result);
		return -ENFILE;
	}

	public_key_memdb_init();
	ReadPrivKey(&(aik_pointer->cakey),"privkey/CA",passwd);
	if(aik_pointer->cakey == NULL)
	{
		printf("read rsa private key failed!\n");
		return 0;
	}

	result=TESI_Local_GetPubKeyFromCA(&(aik_pointer->hCAKey),"cert/CA");
	if(result!=TSS_SUCCESS)
		return result;
	void * context;
	ret=ex_module_setpointer(sub_proc,aik_pointer);
	if(ret<0)
		return ret;
	return 0;
}

int aik_casign_start(void * sub_proc,void * para)
{
	int ret;
	void * recv_msg;
	int type;
	int subtype;
	int i;

	printf("begin aik casign start!\n");

	for(i=0;i<3000*1000;i++)
	{
		usleep(time_val.tv_usec);
		ret=ex_module_recvmsg(sub_proc,&recv_msg);
		if(ret<0)
			continue;
		if(recv_msg==NULL)
			continue;

 		type=message_get_type(recv_msg);
 		subtype=message_get_subtype(recv_msg);
		if(type==NULL)
		{
			message_free(recv_msg);
			continue;
		}
		if(!memdb_find_recordtype(type,subtype))
		{
			printf("message format (%d %d) is not registered!\n",
				message_get_type(recv_msg),message_get_subtype(recv_msg));
			continue;
		}
		if(type==DTYPE_FILE_TRANS)
		{
			if(subtype==SUBTYPE_FILE_NOTICE)
			{
				proc_aik_casign(sub_proc,recv_msg);
			}
		}
	}
	return 0;
};

int proc_aik_casign(void * sub_proc,void * recv_msg)
{
	TSS_RESULT result;
	TSS_HKEY 	hSignKey;
	TSS_HKEY	hAIKey, hCAKey;
	struct aik_cert_info certinfo;
	struct policyfile_notice * file_notice;

	TCPA_IDENTITY_PROOF	identityProof;
	int ret;

	char local_uuid[DIGEST_SIZE];
	char proc_name[DIGEST_SIZE];
	struct aik_proc_pointer * aik_pointer;
	
	ret=proc_share_data_getvalue("uuid",local_uuid);
	ret=proc_share_data_getvalue("proc_name",proc_name);

	printf("begin aik casign!\n");
	char buffer[1024];
	char filename[DIGEST_SIZE*3];
	char digest[DIGEST_SIZE];
	int blobsize=0;
	int fd;

	aik_pointer=ex_module_getpointer(sub_proc);
	if(aik_pointer==NULL)
		return -EINVAL;

	ret = message_get_record(recv_msg,&file_notice,0);
	if(ret<0)
		return -EINVAL;
	if(file_notice==NULL)
		return 0;

	Strcpy(buffer,file_notice->filename);
	buffer[Strlen(file_notice->filename)-4]=0;

	TESI_AIK_VerifyReq(aik_pointer->cakey,aik_pointer->hCAKey,buffer,&hAIKey,&identityProof);
	struct ca_cert usercert;

	// read the req info from aik request package
	void * struct_template=memdb_get_template(DTYPE_AIK_STRUCT,SUBTYPE_AIK_CERT_INFO);
	if(struct_template==NULL)
		return -EINVAL;
	blobsize=blob_2_struct(identityProof.labelArea,&certinfo,struct_template);
	if(blobsize!=identityProof.labelSize)
		return -EINVAL;

	// get the pubek
	DB_RECORD * record;
	struct vTPM_publickey * pubek;
	record=memdb_find_first(DTYPE_TESI_KEY_STRUCT,SUBTYPE_PUBLIC_KEY,"uuid",certinfo.pubkey_uuid);
	
	if(record==NULL)
	{
		printf("can't find pubek!\n");
		return -EINVAL;
	}
	pubek=record->record;
	char * pubek_name;
	pubek_name=dup_str(pubek->key_filename,128);
	pubek_name[strlen(pubek_name)-4]=0;
	printf("find pubek %s\n!",pubek_name);

	// get the uuid of identity key and write it to user cert
	TESI_Local_WritePubKey(hAIKey,"identkey");

	calculate_sm3("identkey.pem",certinfo.pubkey_uuid);

	// get the usercert's blob 
	blobsize=struct_2_blob(&certinfo,buffer,struct_template);
	printf("create usercert succeed!\n");
	
	if (result = TESI_AIK_CreateAIKCert(hAIKey,aik_pointer->cakey,buffer,blobsize,pubek_name,"cert/active")) {
		printf("ca_create_credential %s", tss_err_string(result));
		Free(pubek_name);
		return result;
	}
	printf("create active.req succeed!\n");
	Free(pubek_name);

	ret=convert_uuidname("cert/active",".req",digest,filename);

	if(ret<0)
		return ret;

	struct policyfile_req * aikfile_req = Talloc0(sizeof(*aikfile_req));
	if(aikfile_req==NULL)
		return -EINVAL;
	Memcpy(aikfile_req->uuid,digest,DIGEST_SIZE);
	Strcat(buffer,".req");
	aikfile_req->filename=dup_str(filename,0);
	//aikfile_req->requestor=dup_str(user_info->user_name,0);
		

	void * send_msg;
	send_msg=message_create(DTYPE_FILE_TRANS,SUBTYPE_FILE_REQUEST,recv_msg);
	if(send_msg==NULL)
		return -EINVAL;
	message_add_record(send_msg,aikfile_req);
	ex_module_sendmsg(sub_proc,send_msg);

	return 0;

}
