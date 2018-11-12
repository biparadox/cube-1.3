#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>
 
#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "message.h"
#include "ex_module.h"
#include "sys_func.h"
#include "msgrecord_store.h"
// add para lib_include
int msgrecord_store_init(void * sub_proc, void * para)
{
	int ret;
	// add yorself's module init func here
	return 0;
}
int msgrecord_store_start(void * sub_proc, void * para)
{
	int ret;
	void * recv_msg;
	int type;
	int subtype;
	// add yorself's module exec func here
	while(1)
	{
		usleep(time_val.tv_usec);
		ret=ex_module_recvmsg(sub_proc,&recv_msg);
		if(ret<0)
			continue;
		if(recv_msg==NULL)
			continue;
		type=message_get_type(recv_msg);
		subtype=message_get_subtype(recv_msg);
		if(!memdb_find_recordtype(type,subtype))
		{
			printf("message format (%d %d) is not registered!\n",
			message_get_type(recv_msg),message_get_subtype(recv_msg));
			continue;
		}
		if((type==DTYPE_MESSAGE)&&(subtype==SUBTYPE_CTRL_MSG))
		{
			ret=proc_ctrl_message(sub_proc,recv_msg);
			if(ret==MSG_CTRL_EXIT)
				return MSG_CTRL_EXIT;
		}
		else
		{
			proc_store_message(sub_proc,recv_msg);
		}
	}
	return 0;
}

int proc_store_message(void * sub_proc,void * message)
{
	int type;
	int subtype;
	int i,count;
	int ret;
	printf("begin proc store \n");

	type=message_get_type(message);
	subtype=message_get_subtype(message);

	void * new_msg;
	void * record;
	DB_RECORD * db_record;
	RECORD(MESSAGE,TYPES) type_pair;

	i=0;
	count=0;

	ret=message_get_record(message,&record,i++);
	if(ret<0)
		return ret;
	while(record!=NULL)
	{
		db_record=memdb_store(record,type,subtype,NULL);
		if(db_record!=NULL)
			count++;
		ret=message_get_record(message,&record,i++);
		if(ret<0)
			return ret;
	}

	new_msg=message_create( TYPE_PAIR(MESSAGE,TYPES),message);
	if(new_msg==NULL)
		return -EINVAL;
	type_pair.type=type;
	type_pair.subtype=subtype;
	message_add_record(new_msg,&type_pair);
	ex_module_sendmsg(sub_proc,new_msg);

	return ret;
}

int proc_ctrl_message(void * sub_proc,void * message)
{
	int ret;
	int i=0;
	printf("begin proc echo \n");

	struct ctrl_message * ctrl_msg;

	
	ret=message_get_record(message,&ctrl_msg,i++);
	if(ret<0)
		return ret;
	if(ctrl_msg!=NULL)
	{
		ret=ctrl_msg->ctrl; 
	}

	ex_module_sendmsg(sub_proc,message);

	return ret;
}
