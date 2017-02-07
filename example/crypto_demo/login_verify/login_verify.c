#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "data_type.h"
#include "alloc.h"
#include "string.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "message.h"
#include "ex_module.h"
#include "../include/app_struct.h"
#include "../include/server_struct.h"

struct timeval time_val={0,50*1000};

int proc_verify(void * sub_proc,void * message);
int user_addr_store(void * sub_proc,void * message);

int login_verify_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	return 0;
}

int login_verify_start(void * sub_proc,void * para)
{
	int ret;
	void * recv_msg;
	int i;
	int type;
	int subtype;


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
		if(!memdb_find_recordtype(type,subtype))
		{
			printf("message format (%d %d) is not registered!\n",
				message_get_type(recv_msg),message_get_subtype(recv_msg));
			continue;
		}
		proc_verify(sub_proc,recv_msg);
	}

	return 0;
};

int proc_verify(void * sub_proc,void * message)
{
	int type;
	int subtype;
	int ret;
	printf("begin proc login_verify \n");

	type=message_get_type(message);
	if(type!=DTYPE_CRYPTO_DEMO)
		return -EINVAL;
	subtype=message_get_subtype(message);
	if(subtype!=SUBTYPE_LOGIN_INFO)
		return -EINVAL;

	void * new_msg;
	void * record;
	
	struct login_info * login_data;
	struct login_info * lib_data;
	struct verify_return * return_data;

	new_msg=message_create(DTYPE_CRYPTO_DEMO,SUBTYPE_VERIFY_RETURN,message);
	return_data=Talloc0(sizeof(*return_data));

	ret=message_get_record(message,&login_data,0);
	if(ret<0)
		return ret;

	lib_data=memdb_get_first_record(type,subtype);
	while(lib_data!=NULL)
	{
		if(Strncmp(lib_data->user,login_data->user,DIGEST_SIZE)==0)
			break;
		lib_data=memdb_get_next_record(type,subtype);
	}
	if(lib_data==NULL)
	{
		return_data->ret_data=dup_str("no such user!",0);
		return_data->retval=-1;
	}
	else
	{
		if (Strncmp(lib_data->passwd,login_data->passwd,DIGEST_SIZE)!=0)
		{
			return_data->ret_data=dup_str("error passwd!",0);
			return_data->retval=0;
		}
		else
		{
			return_data->ret_data=dup_str("login_succeed!",0);
			return_data->retval=1;
			ret=user_addr_store(sub_proc,message);
			if(ret<0)
				return ret;
		}
	}
	return_data->ret_data_size=Strlen(return_data->ret_data)+1;
	message_add_record(new_msg,return_data);
	ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}

int user_addr_store(void * sub_proc,void * message)
{
	
	int ret;
	struct user_address * user_addr;
	MSG_EXPAND * expand;
	struct expand_flow_trace * flow_trace;
	struct login_info * login_data;
	DB_RECORD * db_record;
	int trace_offset;
	
	ret=message_get_record(message,&login_data,0);
	if(ret<0)
		return ret;
	ret=message_get_define_expand(message,&expand,DTYPE_MSG_EXPAND,SUBTYPE_FLOW_TRACE);
	if(ret<0)
		return ret;
	if(expand==NULL)
		return -EINVAL;
	flow_trace=expand->expand;

	if(flow_trace->record_num<=0)
		return -EINVAL;
	trace_offset=DIGEST_SIZE*(flow_trace->record_num-1);
	db_record=memdb_find_byname(login_data->user,DTYPE_CRYPTO_DEMO,SUBTYPE_USER_ADDR);
	if(db_record!=NULL)
	{
		user_addr=(struct user_address *)db_record->record;
		if(Memcmp(user_addr->addr,flow_trace->trace_record+trace_offset,DIGEST_SIZE)==0)
			return 0;	
		memdb_remove_record(db_record);
	}
	user_addr=Talloc0(sizeof(struct user_address));
	if(user_addr==NULL)
		return -ENOMEM;
	Strncpy(user_addr->user,login_data->user,DIGEST_SIZE);
	Memcpy(user_addr->addr,flow_trace->trace_record+trace_offset,DIGEST_SIZE);
	memdb_store(user_addr,DTYPE_CRYPTO_DEMO,SUBTYPE_USER_ADDR,login_data->user);
	return 1;
}
