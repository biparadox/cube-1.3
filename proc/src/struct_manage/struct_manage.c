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
#include "sys_func.h"

char Buf[DIGEST_SIZE*32];

int struct_manage_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	return 0;
}

int struct_manage_start(void * sub_proc,void * para)
{
	int ret;
	void * recv_msg;
	int i;
	int type;
	int subtype;

	__proc_output_describe(sub_proc,DB_NAMELIST,0,NULL);

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
		if((type==DTYPE_MESSAGE)&&(subtype==SUBTYPE_TYPES))
		{
			ret=proc_output_describe(sub_proc,recv_msg);
		}
	}
	return 0;
};

int proc_output_describe(void * sub_proc,void * message)
{
	int type;
	int subtype;
	int i;
	int ret;
	printf("begin proc echo \n");

	type=message_get_type(message);
	subtype=message_get_subtype(message);
	if((type!=DTYPE_MESSAGE) || (subtype!=SUBTYPE_TYPES))
		return -EINVAL;

	struct types_pair * types;
	ret=message_get_record(message,&types,0);
	if(ret<0)
		return ret;
	if(types==NULL)
		return 0;

	return __proc_output_describe(sub_proc,types->type,types->subtype,message);
	return ret;
}

int __proc_output_describe(void * sub_proc,int type,int subtype,void * message)
{
	int ret;

	int i=0;
	int count=0;
	int total=0;

	DB_RECORD * record;
	void * send_msg;

       record=memdb_get_first(type,subtype);
        while(record!=NULL)
        {
		if(count==0)
		{
			send_msg=message_create(type,subtype,message);
			if(send_msg==NULL)
				return -EINVAL;
		}

		count++;
		total++;
		message_add_record(send_msg,record->record);
                record=memdb_get_next(type,subtype);
		if(record==NULL)
		{
			ex_module_sendmsg(sub_proc,send_msg);
			break;
		}
		if(count==5)
		{
			count=0;
			ex_module_sendmsg(sub_proc,send_msg);
			sleep(1);
		}
        }
	return ret;
}
