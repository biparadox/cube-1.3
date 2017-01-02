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
#include "sec_entity.h"

extern struct timeval time_val={0,50*1000};

int hello_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	return 0;
}

int hello_start(void * sub_proc,void * para)
{
	int ret;
	int i;
	const char * type;


	for(i=0;i<3000*1000;i++)
	{
		usleep(time_val.tv_usec);
		if(i==10000)
			proc_hello_message(sub_proc,NULL);
	}

	return 0;
};

int proc_hello_message(void * sub_proc,void * message)
{
	int type;
	int subtype;
	int i;
	int ret;
	printf("begin proc hello \n");

	struct message_box * new_msg;
	void * record;
	new_msg=message_create(DB_DTYPE_START,DB_STYPE_MSG,message);
	
	i=0;

	struct basic_message * msg_info;
	msg_info=Talloc(sizeof(*msg_info));

	msg_info->message=dup_str("hello,world!",0);

	message_add_record(new_msg,&record);
	sec_subject_sendmsg(sub_proc,new_msg);
	return ret;
}
