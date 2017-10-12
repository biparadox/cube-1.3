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
#include <tss/tddl_error.h>
#include <tss/tcs_error.h>
#include <tss/tspi.h>

#include "data_type.h"
#include "errno.h"
#include "alloc.h"
#include "memfunc.h"
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
#include "tpm_key_create.h"

static struct timeval time_val={0,50*1000};
int print_error(char * str, int result)
{
	printf("%s %s",str,tss_err_string(result));
}

int tpm_key_create_init(void * sub_proc,void * para)
{
	int ret;
	TSS_RESULT result;	
	char local_uuid[DIGEST_SIZE];
	
	result=TESI_Local_ReloadWithAuth("ooo","sss");
	if(result!=TSS_SUCCESS)
	{
		printf("open tpm error %d!\n",result);
		return -ENFILE;
	}
	return 0;
}

int tpm_key_create_start(void * sub_proc,void * para)
{
	int ret;
	int retval;
	void * recv_msg;
	void * send_msg;
	void * context;
	int i;
	int type;
	int subtype;

	char local_uuid[DIGEST_SIZE];
	char proc_name[DIGEST_SIZE];
	
	ret=proc_share_data_getvalue("uuid",local_uuid);
	ret=proc_share_data_getvalue("proc_name",proc_name);

	printf("begin tpm key create start!\n");

	for(i=0;i<300*1000;i++)
	{
		usleep(time_val.tv_usec);
		ret=ex_module_recvmsg(sub_proc,&recv_msg);
		if(ret<0)
			continue;
		if(recv_msg==NULL)
			continue;

 		type=message_get_type(recv_msg);
		subtype=message_get_subtype(recv_msg);
		
		if((type==DTYPE_TESI_KEY_STRUCT)&&(subtype==SUBTYPE_WRAPPED_KEY))
		{
			proc_tpm_key_generate(sub_proc,recv_msg);
		}
		else if((type =DTYPE_TESI_KEY_STRUCT)&&(subtype==SUBTYPE_KEY_CERTIFY))
		{
			proc_tpm_key_certify(sub_proc,recv_msg);
		}
	}

	return 0;
};


int proc_tpm_key_generate(void * sub_proc,void * recv_msg)
{
	TSS_RESULT result;
	TSS_HKEY   hKey;
	TSS_HKEY   hWrapKey;
	int i;
	struct vTPM_wrappedkey * key_frame;
	struct vTPM_publickey * pubkey_frame;
	int ret;

	char filename[DIGEST_SIZE*4];
	
	printf("begin tpm key generate!\n");
	char buffer[1024];
	char digest[DIGEST_SIZE];
	int blobsize=0;
	int fd;

	// create a signkey and write its key in localsignkey.key, write its pubkey in localsignkey.pem
	result=TESI_Local_ReloadWithAuth("ooo","sss");

	i=0;

	MSG_HEAD * msghead;
	msghead=message_get_head(recv_msg);
	if(msghead==NULL)
		return -EINVAL;

	void * send_msg=message_create(DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,recv_msg);
	void * send_pubkey_msg=message_create(DTYPE_TESI_KEY_STRUCT,SUBTYPE_PUBLIC_KEY,recv_msg);
	for(i=0;i<msghead->record_num;i++)
	{
		ret=message_get_record(recv_msg,&key_frame,i);
		if(ret<0)
			return -EINVAL;
		if(key_frame==NULL)
			break;
		

		if(!Isemptyuuid(key_frame->uuid))
		{
			ret=find_tpm_key(key_frame,&pubkey_frame);
			if(ret<0)
				return -EINVAL;
			if(ret==0)
			{
				Memset(key_frame->uuid,0,DIGEST_SIZE);
			}		
		}
		else
		{
			ret=create_tpm_key(key_frame,&pubkey_frame);
			if(ret<0)
				return ret;
		}
		ret=message_add_record(send_msg,key_frame);
		if(ret<0)
			break;
		ret=message_add_record(send_pubkey_msg,pubkey_frame);
		if(ret<0)
			break;
	};
	ret=ex_module_sendmsg(sub_proc,send_msg);
	if(ret>=0)
		ret=ex_module_sendmsg(sub_proc,send_pubkey_msg);
	return ret;
}

int proc_tpm_key_certify(void * sub_proc,void * recv_msg)
{
	TSS_RESULT result;
	TSS_HKEY   hKey;
	TSS_HKEY   hAIK;
	int i;
	struct vTPM_wrappedkey * key_frame;
	struct vTPM_publickey * pubkey_frame;
	KEY_CERT * key_cert;
	int ret;
	DB_RECORD * db_record;
	char filename[DIGEST_SIZE*4];
	
	printf("begin tpm key certify!\n");
	char buffer[1024];
	char digest[DIGEST_SIZE];
	int blobsize=0;
	int fd;

	// create a signkey and write its key in localsignkey.key, write its pubkey in localsignkey.pem
	result=TESI_Local_Reload();

	i=0;

	MSG_HEAD * msghead;
	msghead=message_get_head(recv_msg);
	if(msghead==NULL)
		return -EINVAL;

	void * send_msg=message_create(DTYPE_TESI_KEY_STRUCT,SUBTYPE_KEY_CERTIFY,recv_msg);
	for(i=0;i<msghead->record_num;i++)
	{
		ret=message_get_record(recv_msg,&key_cert,i);
		if(ret<0)
			return -EINVAL;
		if(key_cert==NULL)
			break;

		if(Isemptyuuid(key_cert->uuid))
		{
			result=_load_tpm_key(key_cert->keyuuid,&hKey);
			if(result!=0)
				return -EINVAL;
			result=_load_tpm_key(key_cert->aikuuid,&hAIK);
			if(result!=0)
				return -EINVAL;
			result=TESI_Report_CertifyKey(hKey,hAIK,"cert/key");	
			if ( result != TSS_SUCCESS )
			{
				printf( "Certify key failed %s!\n",tss_err_string(result));
				return result;
			}
			ret=convert_uuidname("cert/key",".val",key_cert->uuid,filename);
			if(ret<0)
				return ret;
			key_cert->filename=dup_str(filename,0);

			// change key_cert's keyid to public_key
			db_record=memdb_find_first(DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,"uuid",key_cert->keyuuid);
			if(db_record==NULL)
				return -EINVAL;
			key_frame=db_record->record;
			key_cert->keyusage=key_frame->key_type;
			Memcpy(key_cert->keyuuid,key_frame->pubkey_uuid,DIGEST_SIZE);

			// change aik's keyid to public_key
			db_record=memdb_find_first(DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,"uuid",key_cert->aikuuid);
			if(db_record==NULL)
				return -EINVAL;
			key_frame=db_record->record;
			Memcpy(key_cert->aikuuid,key_frame->pubkey_uuid,DIGEST_SIZE);
		
			memdb_store(key_cert,DTYPE_TESI_KEY_STRUCT,SUBTYPE_KEY_CERTIFY,NULL);
		}
		ret=message_add_record(send_msg,key_cert);
		if(ret<0)
			break;
	};
	ret=ex_module_sendmsg(sub_proc,send_msg);
	return ret;
}
