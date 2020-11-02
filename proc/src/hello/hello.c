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
#include "basetype.h"
#include "message.h"
#include "ex_module.h"

extern struct timeval time_val={0,50*1000};

int hello_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	return 0;
}

int hello_start(void * sub_proc,void * para)
{
	proc_hello_message(sub_proc,para);
	sleep(2);
//	proc_exit_message(sub_proc,NULL);
	sleep(2);
	return 0;
};

int proc_hello_message(void * sub_proc,void * para)
{
	int ret;
	int i;

	struct start_para * start_para=para;

	void * new_msg;
	new_msg=message_create(DTYPE_MESSAGE,SUBTYPE_BASE_MSG,NULL);
	struct basic_message * msg_info;
	msg_info=Talloc(sizeof(*msg_info));
	if((start_para==NULL) || (start_para->argc<=1))
	{
		msg_info->message=dup_str("Hello,world!",0);
	}
	else
	{
		char * buffer = Talloc(start_para->argc * DIGEST_SIZE);
		Strcpy(buffer,"Hello,");	
		for(i=1;i<start_para->argc;i++)
			Strncat(buffer,start_para->argv[i],DIGEST_SIZE);
		msg_info->message=dup_str(buffer,0);
		Free(buffer);
	}
	ret=message_add_record(new_msg,msg_info);
	if(ret<0)
		return ret;
	ret=ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}

int proc_exit_message(void * sub_proc, void * recv_msg)
{
	int ret;
	struct ctrl_message * ctrl_msg;
	ctrl_msg=Talloc(sizeof(*ctrl_msg));
	if(ctrl_msg==NULL)
		return -ENOMEM;
	ctrl_msg->name=dup_str(ex_module_getname(sub_proc),0);
	ctrl_msg->ctrl=MSG_CTRL_EXIT;
	void * new_msg;
	new_msg=message_create(DTYPE_MESSAGE,SUBTYPE_CTRL_MSG,recv_msg);
	if(new_msg==NULL)
		return -EINVAL;
	ret=message_add_record(new_msg,ctrl_msg);
	if(ret<0)
		return ret;
	ret=ex_module_sendmsg(sub_proc,new_msg);
	return ret;	
}
