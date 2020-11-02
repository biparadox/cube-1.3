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
#include "memdb.h"
#include "message.h"
#include "ex_module.h"

#include "../include/data_define.h"

int send_int_array(char* name,int num,int * array,void * sub_proc);
int send_index_array(char * name,enum data_type type, int num,int * index,void * sub_proc);
struct timeval time_val={0,50*1000};

int proc_verify(void * sub_proc,void * message);
int user_addr_store(void * sub_proc,void * message);

int bubble_sort_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	time_t seeds;
	time(&seeds);
	srand(seeds);
	return 0;
}

int bubble_sort_start(void * sub_proc,void * para)
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
		proc_bubble_sort(sub_proc,recv_msg);
	}

	return 0;
};

int proc_bubble_sort(void * sub_proc,void * message)
{
	int type;
	int subtype;
	int ret;
	int i;
	printf("begin proc login_verify \n");

	type=message_get_type(message);
	subtype=message_get_subtype(message);

	int size=10;
	int   * value;
	printf("begin proc bubble_sort \n");

	struct visual_req * req_data;
	
	ret=message_get_record(message,&req_data,0);
	
	size=req_data->size;	

	value=(int *)malloc(sizeof(int)*size);
	if(value==NULL)
		return -EINVAL;

	for(i=0;i<size;i++)
	{
		value[i]=rand()%256;
	}

	// web visual debug start: send array value
	send_int_array("bubble",size,value,sub_proc);
	// web visual debug end

	sleep(2);
	ret=bubble_sort(size,value,sub_proc,req_data->interval);
	free(value);
	return ret;
}

int bubble_sort(int size, int * value,void * sub_proc,int interval)
{
	int index[2];
	int i,j;
	int ret=size;
	
	for(i=size;i>0;i--)
	{
		for(j=0;j<i-1;j++)
		{
			int temp;
			// web visual debug start: record curr bubble sort site
			index[0]=j;index[1]=j+1;
			// web visual debug end

			if(value[j]>value[j+1])
			{	
				temp=value[j];value[j]=value[j+1];value[j+1]=temp;
				// web visual debug start:
				send_index_array("bubble",DATA_SWAP,2,index,sub_proc);			
				//web visual debug end
			}
/*
			else
			{
				// web visual debug start:
				send_index_array("bubble",DATA_KEEP,2,index,sub_proc);			
				//web visual debug end
			}
*/
			usleep(1000*interval);
		}	
		usleep(1000*interval);
	}
	return ret;
}

int send_int_array(char * name,int num,int * array,void * sub_proc)
{
	struct visual_data * data;
	int i;
	int ret;
	struct message_box * new_msg;
	new_msg=message_create(DTYPE_VISUAL_SORT,SUBTYPE_VISUAL_DATA,NULL);
	if(new_msg==NULL)
		return -EINVAL;
	for(i=0;i<num;i++)
	{
		data=Talloc0(sizeof(struct visual_data));
		if(data==NULL)
			return -ENOMEM;
		data->name=dup_str(name,0);	
		
		data->type=DATA_INIT;
		data->index=0;
		data->value=array[i];
		message_add_record(new_msg,data);
	}
	ex_module_sendmsg(sub_proc,new_msg);
	return num;
}

int send_index_array(char * name,enum data_type type, int num,int * index,void * sub_proc)
{
	struct visual_data * data;
	int i;
	int ret;
	struct message_box * new_msg;
	new_msg=message_create(DTYPE_VISUAL_SORT,SUBTYPE_VISUAL_DATA,NULL);
	if(new_msg==NULL)
		return -EINVAL;
	for(i=0;i<num;i++)
	{
		data=Talloc0(sizeof(struct visual_data));
		if(data==NULL)
			return -ENOMEM;
	
		Memset(data,0,sizeof(*data));
		data->name=dup_str(name,0);	
		data->type=type;
		data->index=index[i];
		message_add_record(new_msg,data);
	}
	ex_module_sendmsg(sub_proc,new_msg);
	return num;
}
