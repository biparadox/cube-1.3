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
#include "msg_expand_send.h"
// add para lib_include

int start_delay=500*1000;
int infile_delay=500*1000;
int exfile_delay=500*1000;

int proc_msg_expand_send(void * sub_proc,char * filename,void * recv_msg);
int proc_basemsg(void * sub_proc,void * recv_msg);
int msg_expand_send_init(void * sub_proc, void * para)
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
int msg_expand_send_start(void * sub_proc, void * para)
{
	int ret;
	int i;
	void * recv_msg;
	struct start_para * start_para=para;
	int type;
	int subtype;
	// add yorself's module exec func here
	print_cubeaudit("begin msg_expand_send module!\n");

	usleep(start_delay);
	
	if(para!=NULL)
	{
		if(start_para->argc>=2)
		{
			for(i=1;i<start_para->argc;i++)
			{
				ret=proc_msg_expand_send(sub_proc,start_para->argv[i],NULL);
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
		if((type==TYPE(MESSAGE)) &&(subtype==SUBTYPE(MESSAGE,BASE_MSG)))
		{
			proc_basemsg(sub_proc,recv_msg);
		}
		else if((type==TYPE(MESSAGE)) &&(subtype==SUBTYPE(MESSAGE,CTRL_MSG)))
		{
			ret = proc_msg_expand_send_ctrl(sub_proc,recv_msg);
            		switch(ret)
            		{
                		case MSG_CTRL_INIT:   
                		case MSG_CTRL_START:   
                		case MSG_CTRL_SLEEP:   
                		case MSG_CTRL_RESUME:   
                    			break;
                		case MSG_CTRL_STOP:
	               	 		printf("stop the proc! \n");
                    			sleep(1);
                    			return ret;
                		case MSG_CTRL_EXIT:
                		default:
	                		printf("exit the proc! \n");
                    		exit(ret);
            		}
		}
	}
	
	return 0;
}

int proc_basemsg(void * sub_proc,void * recv_msg)
{
	int ret;
	int i=0;

	RECORD(MESSAGE,BASE_MSG) * base_msg;
	
	ret =message_get_record(recv_msg,&base_msg,i++);
	if(ret<0)
		return ret;
	while(base_msg!=NULL)
	{
		proc_msg_expand_send(sub_proc,base_msg->message,NULL);
		ret=message_get_record(recv_msg,&base_msg,i++);	
	}
	return i;
}

int proc_msg_expand_send(void * sub_proc, char * filename,void * recv_msg)
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
		print_cubeerr("msg_expand_send: message format error!");
		return -EINVAL;
	}

	message_set_activemsg(new_msg,recv_msg);

	ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}

int proc_msg_expand_send_ctrl(void * sub_proc,void * message)
{
	int ret;
	int i=0;

	RECORD(MESSAGE,CTRL_MSG) * ctrl_msg;

	
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
