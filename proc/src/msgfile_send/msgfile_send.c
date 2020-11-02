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
#include "msgfile_send.h"
// add para lib_include

int start_delay=500*1000;
int infile_delay=500*1000;
int exfile_delay=500*1000;

int proc_msgfile_send(void * sub_proc,char * filename,void * recv_msg);
int proc_basemsg(void * sub_proc,void * recv_msg);
int msgfile_send_init(void * sub_proc, void * para)
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
int msgfile_send_start(void * sub_proc, void * para)
{
	int ret;
	int i;
	void * recv_msg;
	struct start_para * start_para=para;
	int type;
	int subtype;
	// add yorself's module exec func here
	print_cubeaudit("begin msgfile_send module!\n");

	usleep(start_delay);
	
	if(para!=NULL)
	{
		if(start_para->argc>=2)
		{
			for(i=1;i<start_para->argc;i++)
			{
				ret=proc_msgfile_send(sub_proc,start_para->argv[i],NULL);
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
		if((type==DTYPE_MESSAGE) &&(subtype==SUBTYPE_BASE_MSG))
		{
			proc_basemsg(sub_proc,recv_msg);
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
		proc_msgfile_send(sub_proc,base_msg->message,recv_msg);
		ret=message_get_record(recv_msg,&base_msg,i++);	
	}
	return i;
}

int proc_msgfile_send(void * sub_proc, char * filename,void * recv_msg)
{
	int count=0;
	int type,subtype;
	void * head_node;
	void * record_node;	
	void * elem_node;	
	int fd;
	int ret;
	void * record_template;
	void * new_msg;
	int  mode=0;
	
	fd=open(filename,O_RDONLY);
	if(fd<0)
		return -EIO;
	ret=read_json_node(fd,&head_node);
	if(ret<0)
		return -EINVAL;
	if(head_node==NULL)
		return -EINVAL;

	// get message's type
	elem_node=json_find_elem("type",head_node);
	if(elem_node==NULL)
	{
		close(fd);
		return -EINVAL;
	}
	type=memdb_get_typeno(json_get_valuestr(elem_node));
	if(type<=0)
	{
		close(fd);
		return -EINVAL;
	} 			
	// get message's subtype
	elem_node=json_find_elem("subtype",head_node);
	if(elem_node==NULL)
	{
		close(fd);
		return -EINVAL;
	}
	subtype=memdb_get_subtypeno(type,json_get_valuestr(elem_node));
	if(subtype<=0)
	{
		close(fd);
		return -EINVAL;
	} 
	// get record template
	record_template=memdb_get_template(type,subtype);
	if(record_template==NULL)
	{
		close(fd);
		return -EINVAL;
	}		

	// get message send mode
	elem_node=json_find_elem("mode",head_node);
	if(elem_node==NULL)
	{
		mode=0;
	}
	else
	{
		char * modestr=json_get_valuestr(elem_node);
		if(modestr==NULL)
		{
			print_cubeerr("error msg send mode!\n");
			return -EINVAL;
		}
		if(Strcmp(modestr,"INT")==0)
		{
			mode=1;
		}
		else if(Strcmp(modestr,"SEP")==0)
		{
			mode=0;
		}
		else
		{
			print_cubeerr("error msg send mode define!\n");
			return -EINVAL;
		}
		
	}
	
	ret=read_json_node(fd,&record_node);
	int record_size=struct_size(record_template);	
	void * record=NULL;

	new_msg=message_create(type,subtype,recv_msg);
	while(ret>0)
	{
		if(record==NULL)
			record=Talloc(record_size);
		if(record_node!=NULL)
		{
			ret=json_2_struct(record_node,record,record_template);
			if(ret>=0)
			{
				if(new_msg==NULL)
				{
					printf("create message (%d %d) error!\n",type,subtype);
					return -EINVAL;	
				}	
				message_add_record(new_msg,record);
				if(mode==0)
				{
					ex_module_sendmsg(sub_proc,new_msg);
					new_msg=message_create(type,subtype,recv_msg);
					usleep(infile_delay);
				}
				count++;
			}
		}	
		ret=read_json_node(fd,&record_node);
	}
	if(mode==1)
	{
		if(count>0)
			ex_module_sendmsg(sub_proc,new_msg);
	}
	close(fd);
	return count;
}
