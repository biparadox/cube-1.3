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
#include "key_manage1.h"

#include "../../include/app_struct.h"

static struct timeval time_val={0,50*1000};
int print_error(char * str, int result)
{
	printf("%s %s",str,tss_err_string(result));
}

int key_manage1_init(void * sub_proc,void * para)
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

	// prepare the slot sock
	void * slot_port=slot_port_init("key_request",2);
	slot_port_addrecordpin(slot_port,DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO);
	slot_port_addmessagepin(slot_port,DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY);
	ex_module_addslot(sub_proc,slot_port);
	return 0;
}

int key_manage1_start(void * sub_proc,void * para)
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

	printf("begin tpm key manage1 start!\n");

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
		
		if((type==DTYPE_TRUST_DEMO)&&(subtype==SUBTYPE_KEY_INFO))
		{
			proc_key_check(sub_proc,recv_msg);
		}
		if((type==DTYPE_TESI_KEY_STRUCT)&&(subtype==SUBTYPE_WRAPPED_KEY))
		{
			proc_key_store(sub_proc,recv_msg);
		}
	}

	return 0;
};


int proc_key_check(void * sub_proc,void * recv_msg)
{
	int ret=0;
	int i=0;
	struct trust_demo_keyinfo * keyinfo;
	struct trust_demo_keyinfo * comp_keyinfo;
	struct vTPM_wrappedkey    * key_record;	
	void * send_msg;
	BYTE uuid[DIGEST_SIZE];

	ret=message_get_record(recv_msg,&keyinfo,0);
	if(keyinfo!=NULL)
	{
//		comp_keyinfo=memdb_get_first_record(DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO);

		// build a slot sock to store the (DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO) keyinfo record and wait for the
		// (DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY) message
		void * slot_port=ex_module_findport(sub_proc,"key_request");
		if(slot_port==NULL)
			return -EINVAL;
		ret=message_get_uuid(recv_msg,uuid);						
		if(ret<0)
			return ret;
		void * sock=slot_create_sock(slot_port,uuid);
		ret=slot_sock_addrecorddata(sock,DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO,keyinfo);
		if(ret<0)
			return ret;
		ex_module_addsock(sub_proc,sock);		

		// build (DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY) message and send it
		key_record=Talloc0(sizeof(*key_record));
		if(key_record==NULL)
			return -ENOMEM;
		key_record->key_type=keyinfo->key_type; 
		key_record->key_size=1024;
		key_record->keypass=dup_str(keyinfo->passwd,DIGEST_SIZE);
		send_msg=message_create(DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,recv_msg);
		if(send_msg==NULL)
			return -EINVAL;
		message_add_record(send_msg,key_record);
		ex_module_sendmsg(sub_proc,send_msg);	
	}	

	return ret;
}

int proc_key_store(void * sub_proc,void * recv_msg)
{
	int ret=0;
	BYTE uuid[DIGEST_SIZE];
	void * sock;	
	struct vTPM_wrappedkey    * key_record;	
	struct trust_demo_keyinfo * keyinfo;

	ret=message_get_uuid(recv_msg,keyinfo->uuid);						
	if(ret<0)
		return ret;
	sock=ex_module_findsock(sub_proc,uuid);
	if(sock==NULL)
		return -EINVAL;	
	ret=slot_sock_addmsg(sock,recv_msg);
	
	if(ret<=0)	
		return ret;

	keyinfo= slot_sock_removerecord(sock,DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO);
	if(keyinfo==NULL)
		return -EINVAL;
	ret=message_get_record(recv_msg,&key_record,0);
	if(key_record==NULL)
		return -EINVAL;				
}
