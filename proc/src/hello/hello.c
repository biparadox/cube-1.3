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
	proc_hello_message(sub_proc,NULL);
	return 0;
};

int proc_hello_message(void * sub_proc,void * message)
{
	int ret;
	printf("begin proc hello \n");

	void * new_msg;
	new_msg=message_create(DTYPE_MESSAGE,SUBTYPE_BASE_MSG,message);
	struct basic_message * msg_info;
	msg_info=Talloc(sizeof(*msg_info));
	msg_info->message=dup_str("hello,world!",0);
	ret=message_add_record(new_msg,msg_info);
	if(ret<0)
		return ret;
	ret=ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}
