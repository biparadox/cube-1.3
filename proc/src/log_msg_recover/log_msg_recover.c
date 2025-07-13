#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
 
#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "basefunc.h"
#include "json.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "message.h"
#include "ex_module.h"
#include "sys_func.h"
#include "log_msg_recover.h"
// add para lib_include

int start_delay=500*1000;
int infile_delay=500*1000;
int exfile_delay=500*1000;

int proc_log_msg_recover(void * sub_proc,char * filename,void * recv_msg);
int proc_basemsg(void * sub_proc,void * recv_msg);
int log_msg_recover_init(void * sub_proc, void * para)
{
	int ret;
        struct init_para * init_para=para;
	// add yorself's module init func here
	
	if(init_para!=NULL)
	{
		start_delay=init_para->start_delay*1000;
		infile_delay=init_para->infile_delay*1000;
		exfile_delay=init_para->exfile_delay*1000;
	}
	
	
	return 0;
}

int log_msg_recover_start(void * sub_proc, void * para)
{
	int ret;
	int i;
	void * recv_msg;
	struct start_para * start_para=para;
	int type;
	int subtype;
	// add yorself's module exec func here
	print_cubeaudit("begin log_msg_recover module!\n");

	usleep(start_delay);
	
	if(para!=NULL)
	{
		if(start_para->argc>=2)
		{
			for(i=1;i<start_para->argc;i++)
			{
				ret=proc_log_msg_recover(sub_proc,start_para->argv[i],NULL);
				usleep(exfile_delay);
			}
		}	
	}	
	sleep(1);

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
		ex_module_sendmsg(sub_proc,recv_msg);
	}
	return 0;
}

int proc_log_msg_recover(void * sub_proc, char * filename,void * recv_msg)
{
	int count=0;
	int type,subtype;
	void * head_node;
	void * record_node;	
	void * elem_node;	
	int fd;
	int ret;
	void * new_msg;
	int  mode=0;
	char * buf;
	const int buf_size=DIGEST_SIZE*64;	
	int read_size;

	
	fd=open(filename,O_RDONLY);
	if(fd<0)
		return -EIO;

	buf = Talloc0(buf_size);
	if(buf==NULL)
		return -ENOMEM;
			
	read_size=read(fd,buf,buf_size);
	
	if(read_size >=buf_size)
		return -EINVAL;
	
	ret = json_2_message(buf,&new_msg);
	if(new_msg == NULL)
	{
		print_cubeerr("log_msg_recover: message format error!");
		return -EINVAL;
	}
/*	
	ret=message_load_record(new_msg);
	if(ret<0)
	{
		printf("load record failed!\n");
		return ret;
	}
	ret=message_load_expand(new_msg);
	if(ret<0)
	{
		printf("load expand failed!\n");
		return ret;
	}
	*/

	message_set_activemsg(new_msg,recv_msg);

	ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}
