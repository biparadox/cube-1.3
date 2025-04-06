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
#include "memfunc.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "message.h"
#include "ex_module.h"

extern struct timeval time_val={0,50*1000};

int echo_plugin_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	return 0;
}

int echo_plugin_start(void * sub_proc,void * para)
{
	int ret;
	void * recv_msg;
	int i;
	int type;
	int subtype;


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
			ex_module_sendmsg(sub_proc,recv_msg);
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
			ex_module_sendmsg(sub_proc,recv_msg);
			//proc_echo_message(sub_proc,recv_msg);
		}
	}
	return 0;
};

int proc_echo_message(void * sub_proc,void * message)
{
	int type;
	int subtype;
	int i;
	int ret;
	printf("begin proc echo \n");

	type=message_get_type(message);
	subtype=message_get_subtype(message);

	void * new_msg;
	void * record;
	new_msg=message_create(type,subtype,message);
	
	i=0;

	ret=message_get_record(message,&record,i++);
	if(ret<0)
		return ret;
	while(record!=NULL)
	{
		message_add_record(new_msg,record);
		ret=message_get_record(message,&record,i++);
		if(ret<0)
			return ret;
	}

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
