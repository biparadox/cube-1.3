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
	void * sock;	
	BYTE uuid[DIGEST_SIZE];

	char local_uuid[DIGEST_SIZE];
	char proc_name[DIGEST_SIZE];
	
	ret=proc_share_data_getvalue("uuid",local_uuid);
	ret=proc_share_data_getvalue("proc_name",proc_name);

	// build a  slot for key request info
	void * slot_port=slot_port_init("key_request",2);
	slot_port_addrecordpin(slot_port,DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO);
	slot_port_addmessagepin(slot_port,DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY);
	slot_port_addmessagepin(slot_port,DTYPE_TESI_KEY_STRUCT,SUBTYPE_PUBLIC_KEY);
	ex_module_addslot(sub_proc,slot_port);


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
		if(((type==DTYPE_TESI_KEY_STRUCT)&&(subtype==SUBTYPE_WRAPPED_KEY))
			||((type==DTYPE_TESI_KEY_STRUCT)&&(subtype==SUBTYPE_PUBLIC_KEY)))
		{
			ret=message_get_uuid(recv_msg,uuid);						
			if(ret<0)
				continue;
			sock=ex_module_findsock(sub_proc,uuid);
			if(sock==NULL)
				continue;	
			ret=slot_sock_addmsg(sock,recv_msg);
	
			if(ret>0)	
				proc_key_store(sub_proc,sock);
		}
	}

	return 0;
};

void * find_key_info(void * key_info)
{
	struct trust_demo_keyinfo * in_keyinfo=key_info;
	struct trust_demo_keyinfo * db_keyinfo;
	DB_RECORD * record;
	void * comp_template;
	int ret;
	comp_template=memdb_get_template(DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO);
	ret=struct_set_flag(comp_template,CUBE_ELEM_FLAG_TEMP,"vtpm_uuid,owner,peer,usage,key_type");
	if(ret<0)
		return NULL;
	record=memdb_get_first(DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO);
	while(record!=NULL)
	{
		db_keyinfo=record->record;
		if(db_keyinfo==NULL)
			return NULL;
		if(struct_part_compare(db_keyinfo,in_keyinfo,comp_template,CUBE_ELEM_FLAG_TEMP))
			break;
		record=memdb_get_next(DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO);
	}

	return record;
}

int proc_key_check(void * sub_proc,void * recv_msg)
{
	int ret=0;
	int i=0;
	struct trust_demo_keyinfo * keyinfo;
	struct trust_demo_keyinfo * comp_keyinfo;
	struct vTPM_wrappedkey    * key_struct;	
	void * send_msg;
	BYTE uuid[DIGEST_SIZE];
	DB_RECORD * keyinfo_record;
	DB_RECORD * key_record;

	// build a slot_port
	void * slot_port=ex_module_findport(sub_proc,"key_request");
	if(slot_port==NULL)
		return -EINVAL;
	ret=message_get_uuid(recv_msg,uuid);						
	if(ret<0)
		return ret;
	// build a sock
	void * sock=slot_create_sock(slot_port,uuid);

	// get message
	ret=message_get_record(recv_msg,&keyinfo,0);
	if(keyinfo!=NULL)
	{
		keyinfo_record=find_key_info(keyinfo);
		if(keyinfo_record!=NULL)
		{
			comp_keyinfo=keyinfo_record->record;
			key_record=memdb_find_first(DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,"uuid",comp_keyinfo->uuid);
			if(key_record!=NULL)
				key_struct=key_record->record;		
			ret=slot_sock_addrecorddata(sock,DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO,comp_keyinfo);
			if(ret<0)
				return ret;
			ex_module_addsock(sub_proc,sock);		
		}
		
		else
		{
			// build a slot sock to store the (DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO) keyinfo record and wait for the
			// (DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY) message
			// build (DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY) message and send it
			key_struct=Talloc0(sizeof(*key_struct));
			if(key_struct==NULL)
				return -ENOMEM;
			Memcpy(key_struct->vtpm_uuid,keyinfo->vtpm_uuid,DIGEST_SIZE); 
			key_struct->key_type=keyinfo->key_type; 
			key_struct->key_size=1024;
			key_struct->keypass=dup_str(keyinfo->passwd,DIGEST_SIZE);
			ret=slot_sock_addrecorddata(sock,DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO,keyinfo);
			if(ret<0)
				return ret;
			ex_module_addsock(sub_proc,sock);		
		}

		send_msg=message_create(DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,recv_msg);
		if(send_msg==NULL)
			return -EINVAL;
		message_add_record(send_msg,key_struct);
		ex_module_sendmsg(sub_proc,send_msg);	
	}	

	return ret;
}

int proc_key_store(void * sub_proc,void * sock)
{
	int ret=0;
	BYTE uuid[DIGEST_SIZE];
	struct vTPM_wrappedkey    * key_record;	
	struct vTPM_wrappedkey    * pubkey_record;	
	struct trust_demo_keyinfo * keyinfo;
	struct trust_demo_keyinfo * pubkeyinfo;
	void * send_msg;
	void * recv_msg;
	struct types_pair * types;

	// get keyinfo record
	keyinfo= slot_sock_removerecord(sock,DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO);
	if(keyinfo==NULL)
		return -EINVAL;

	// get WEAPPED_KEY message
	recv_msg=slot_sock_removemessage(sock,DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY);
	if(recv_msg==NULL)
		return -EINVAL;

	ret=message_get_record(recv_msg,&key_record,0);
	if(key_record==NULL)
		return -EINVAL;				

	Memcpy(keyinfo->uuid,key_record->uuid,DIGEST_SIZE);
	memdb_store(keyinfo,DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO,NULL);


	//memdb_store(key_record,DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,NULL);
	
	send_msg=message_create(DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,recv_msg);
	if(send_msg==NULL)
		return -EINVAL;
	message_add_record(send_msg,key_record);
	ex_module_sendmsg(sub_proc,send_msg);	

	// get PUBLIC_KEY message
	recv_msg=slot_sock_removemessage(sock,DTYPE_TESI_KEY_STRUCT,SUBTYPE_PUBLIC_KEY);
	if(recv_msg==NULL)
		return -EINVAL;

	ret=message_get_record(recv_msg,&pubkey_record,0);
	if(key_record==NULL)
		return -EINVAL;				


	send_msg=message_create(DTYPE_TESI_KEY_STRUCT,SUBTYPE_PUBLIC_KEY,recv_msg);
	if(send_msg==NULL)
		return -EINVAL;
	message_add_record(send_msg,pubkey_record);
	ex_module_sendmsg(sub_proc,send_msg);	

	

	// build a public key_info struct 
/*
	Memcpy(pubkeyinfo,keyinfo,sizeof(*pubkey_info));
	Memcpy(pubkeyinfo->uuid,pubkey_record->uuid,DIGEST_SIZE);
	keyinfo->ispubkey=1;
	send_msg=message_create(DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO,NULL);
	if(send_msg==NULL)
		return -EINVAL;
	message_add_record(send_msg,keyinfo);
	ex_module_sendmsg(sub_proc,send_msg);	
*/

	send_msg=message_create(DTYPE_MESSAGE,SUBTYPE_TYPES,NULL);
	if(send_msg==NULL)
		return -EINVAL;
	types=Talloc(sizeof(*types));
	if(types==NULL)
		return -EINVAL;
	types->type=DTYPE_TRUST_DEMO;
	types->subtype=SUBTYPE_KEY_INFO;
	message_add_record(send_msg,types);
	types=Talloc(sizeof(*types));
	if(types==NULL)
		return -EINVAL;
	types->type=DTYPE_TESI_KEY_STRUCT;
	types->subtype=SUBTYPE_WRAPPED_KEY;
	message_add_record(send_msg,types);
	types=Talloc(sizeof(*types));
	if(types==NULL)
		return -EINVAL;
	types->type=DTYPE_TESI_KEY_STRUCT;
	types->subtype=SUBTYPE_PUBLIC_KEY;
	message_add_record(send_msg,types);
	ex_module_sendmsg(sub_proc,send_msg);	
	return 0;
}
