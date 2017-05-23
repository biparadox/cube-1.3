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
#include "key_request1.h"

#include "../../include/app_struct.h"

static struct timeval time_val={0,50*1000};
int print_error(char * str, int result)
{
	printf("%s %s",str,tss_err_string(result));
}

int key_request1_init(void * sub_proc,void * para)
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

int key_request1_start(void * sub_proc,void * para)
{
	int ret;
	int retval;
	void * recv_msg;
	void * send_msg;
	void * context;
	void * sock;
	BYTE uuid[DIGEST_SIZE];
	int i;
	int type;
	int subtype;

	// build a  slot for key request info
	void * slot_port=slot_port_init("key_request",2);
	slot_port_addmessagepin(slot_port,DTYPE_TRUST_DEMO,SUBTYPE_ENCDATA_INFO);
	slot_port_addmessagepin(slot_port,DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY);
	ex_module_addslot(sub_proc,slot_port);
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
		 if(((type==DTYPE_TESI_KEY_STRUCT)&&(subtype==SUBTYPE_WRAPPED_KEY))
			||((type==DTYPE_TRUST_DEMO)&&(subtype==SUBTYPE_ENCDATA_INFO)))
		{
			ret=message_get_uuid(recv_msg,uuid);						
			if(ret<0)
				continue;
			sock=ex_module_findsock(sub_proc,uuid);
			if(sock==NULL)
			{
				sock = slot_create_sock(slot_port,uuid);
				ret=slot_sock_addmsg(sock,recv_msg);	
				ex_module_addsock(sub_proc,sock);
					continue;
			}	
			ret=slot_sock_addmsg(sock,recv_msg);
	
			if(ret>0)	
				proc_key_load(sub_proc,sock);
		}
	}

	return 0;
};

int proc_key_load(void * sub_proc,void * sock)
{
	int ret;
	BYTE uuid[DIGEST_SIZE];
	int i;
	void * key_msg;
	void * encdata_msg;
	void * send_msg;

	TSS_HKEY hTempKey;
	TSS_RESULT result;
	struct vTPM_wrappedkey * wrapkey;
	struct trust_encdata_info * encdata;
	struct trust_filecrypt_info * filecrypt_info;

	BYTE * buffer;

	key_msg=slot_sock_removemessage(sock,DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY);
	if(key_msg==NULL)
		return -EINVAL;
		
	ret=message_get_record(key_msg,&wrapkey,0);
	if(ret<0)
		return ret;
	if(wrapkey==NULL)
		return -EINVAL;
	
	char filename[DIGEST_SIZE*8];

	Strncpy(filename,wrapkey->key_filename,DIGEST_SIZE*8);
	for(i=Strnlen(filename,DIGEST_SIZE*8);i>0;i--)
	{
		if(filename[i]=='.')
		{
			filename[i]=0;
			break;
		}
	}

	result=TESI_Local_ReadKeyBlob(&hTempKey,filename);
	if(result!=TSS_SUCCESS)
	{
		return result;	
	}
	result=TESI_Local_LoadKey(hTempKey,NULL,wrapkey->keypass);
	if(result!=TSS_SUCCESS)
	{
		return result;	
	}
	
	printf("Load Wrapkey Success!\n");

	encdata_msg=slot_sock_removemessage(sock,DTYPE_TRUST_DEMO,SUBTYPE_ENCDATA_INFO);
	if(encdata_msg==NULL)
		return -EINVAL;
		
	ret=message_get_record(encdata_msg,&encdata,0);
	if(ret<0)
		return ret;
	if(encdata==NULL)
		return -EINVAL;

	int fd=open(encdata->filename,O_RDONLY);
	if(fd<0)
		return -EINVAL;
	struct stat stat_buf;
	ret=fstat(fd,&stat_buf);

	buffer=Talloc(stat_buf.st_size);
	if(buffer==NULL)
		return -ENOMEM;
	ret=read(fd,buffer,stat_buf.st_size);
	if(ret!=stat_buf.st_size)
		return -EINVAL;

	BYTE * outdata;
	int outlength;
	result=TESI_Local_UnBindBuffer(buffer,stat_buf.st_size,hTempKey,&outdata,&outlength);
	if(result!=TSS_SUCCESS)
	{
		return result;	
	}

	send_msg=message_create(DTYPE_MESSAGE,SUBTYPE_UUID_RECORD,key_msg);
	message_add_record(send_msg,outdata);

	ex_module_sendmsg(sub_proc,send_msg);	

	filecrypt_info=Talloc0(sizeof(*filecrypt_info));
	if(filecrypt_info==NULL)
		return -EINVAL;
	Memcpy(filecrypt_info->vtpm_uuid,encdata->vtpm_uuid,DIGEST_SIZE);
	Memcpy(filecrypt_info->encdata_uuid,encdata->uuid,DIGEST_SIZE);
	
	send_msg=message_create(DTYPE_TRUST_DEMO,SUBTYPE_FILECRYPT_INFO,key_msg);
	message_add_record(send_msg,filecrypt_info);

	ex_module_sendmsg(sub_proc,send_msg);	
	
	return 0;
}
