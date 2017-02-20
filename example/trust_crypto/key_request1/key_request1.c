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
	int i;
	int type;
	int subtype;

	char local_uuid[DIGEST_SIZE];
	char proc_name[DIGEST_SIZE];
	
	ret=proc_share_data_getvalue("uuid",local_uuid);
	ret=proc_share_data_getvalue("proc_name",proc_name);

	printf("begin tpm key create start!\n");

	proc_key_request(sub_proc,NULL);

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
			proc_key_request(sub_proc,recv_msg);
		}
	}

	return 0;
};


int proc_key_request(void * sub_proc,void * recv_msg)
{
	struct vTPM_wrappedkey * key_frame;
	struct vTPM_wrappedkey * wrapkey_frame;
	struct types_pair *privkey_types;
	struct types_pair *pubkey_types;
	int i;
	int ret;

	printf("begin to generate key request!\n");

	privkey_types=Talloc0(sizeof(*privkey_types));
	privkey_types->type=DTYPE_TESI_KEY_STRUCT;
	privkey_types->subtype=SUBTYPE_WRAPPED_KEY;

	key_frame=Talloc0(sizeof(*key_frame));
	if(key_frame==NULL)
		return -EINVAL;
	key_frame->key_size=1024;	
	key_frame->keypass=dup_str("kkk",0);	
	void * send_msg;
	void * notice_msg;
	if(recv_msg==NULL)
	{
		key_frame->key_type=TPM_KEY_STORAGE;
		send_msg=message_create(DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,NULL);
		ret=message_add_record(send_msg,key_frame);
		if(ret<0)
			return ret;
	}
	else
	{

	// create a signkey and write its key in localsignkey.key, write its pubkey in localsignkey.pem

		key_frame->key_type=TPM_KEY_SIGNING;
		ret=message_get_record(recv_msg,&wrapkey_frame,0);
		if(ret<0)
			return -EINVAL;
		if(wrapkey_frame==NULL)
			return -EINVAL;
		Memcpy(key_frame->wrapkey_uuid,wrapkey_frame->uuid,DIGEST_SIZE);
		send_msg=message_create(DTYPE_MESSAGE,SUBTYPE_TYPES,NULL);
		message_add_record(send_msg,privkey_types);	
		
	}
	ret=ex_module_sendmsg(sub_proc,send_msg);
	return ret;
}
