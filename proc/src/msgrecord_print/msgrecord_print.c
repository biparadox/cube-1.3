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
#include "msgrecord_print.h"
// add para lib_include
int msgrecord_print_init(void * sub_proc, void * para)
{
	int ret;
	// add yorself's module init func here
	return 0;
}
int msgrecord_print_start(void * sub_proc, void * para)
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
		proc_msgrecord_print(sub_proc,recv_msg);
	}
	return 0;
}

int proc_msgrecord_print(void * sub_proc,void * recv_msg)
{
	int type;
	int subtype;
	int i;
	int ret;
	char record_buf[DIGEST_SIZE*32];

	type=message_get_type(recv_msg);
	subtype=message_get_subtype(recv_msg);

	void * record;

	void * record_template;

	i=0;

	record_template=memdb_get_template(type,subtype);

	if(record_template==NULL)
	{
		printf("record (%d %d) has no template!\n",type,subtype);
		return 0;
	}	

	ret=message_get_record(recv_msg,&record,i++);
	if(ret<0)
		return ret;
	while(record!=NULL)
	{

		ret=struct_2_json(record,record_buf,record_template);		
		if(ret<0)
		{
			printf("record (%d %d) format error!\n",type,subtype);
			return ret;
		}
		printf("%s\n",record_buf);
		ret=message_get_record(recv_msg,&record,i++);
		if(ret<0)
			return ret;
	}

	ex_module_sendmsg(sub_proc,recv_msg);

	return i;
}
