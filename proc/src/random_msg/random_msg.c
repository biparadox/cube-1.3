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
#include "cube.h"
#include "cube_define.h"
#include "cube_record.h"

#include "random_msg.h"
extern struct timeval time_val;

char * default_name;
int  default_type;
int  default_size;
int  dynamic_flag;

int random_msg_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
    struct init_para * init_para=para;
	
	if(init_para!=NULL)
	{
        default_name=dup_str(init_para->default_name,0);
        default_type = init_para->default_type;
        default_size = init_para->size;
        dynamic_flag = init_para->dynamic_flag;
	}
	return 0;
}

int random_msg_start(void * sub_proc,void * para)
{
	int ret;
	int i;
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
        
        if(type != TYPE(GENERAL_RETURN))
        {
            ex_module_sendmsg(sub_proc,recv_msg);
            continue;
        }
        switch(subtype)
        {
            case SUBTYPE(GENERAL_RETURN,INT):
                proc_randomint_msg(sub_proc,recv_msg);
                break;
            case SUBTYPE(GENERAL_RETURN,UUID):
                proc_randomuuid_msg(sub_proc,recv_msg);
                break;
            case SUBTYPE(GENERAL_RETURN,BINDATA):
                proc_randombindata_msg(sub_proc,recv_msg);
                break;
            default:
                ex_module_sendmsg(sub_proc,recv_msg);
                break;
        }
	}
	
	return 0;
};

int proc_randomint_msg(void * sub_proc,void * recv_msg)
{
	int ret;
	int i;
    RECORD(GENERAL_RETURN,INT) * input;

    ret=message_get_record(recv_msg,&input,0);
    if(input==NULL)
    {
        return -EINVAL;
    }
    RAND_bytes(&input->return_value,sizeof(int));

	void * new_msg;
	new_msg=message_create(TYPE_PAIR(GENERAL_RETURN,INT),recv_msg);
    if(new_msg==NULL)
        return -EINVAL;
    ret=message_add_record(new_msg,input);
	if(ret<0)
		return ret;
	ret=ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}

int proc_randomuuid_msg(void * sub_proc,void * recv_msg)
{
	int ret;
	int i;
    RECORD(GENERAL_RETURN,UUID) * input;

    ret=message_get_record(recv_msg,&input,0);
    if(input==NULL)
    {
        return -EINVAL;
    }
    RAND_bytes(input->return_value,DIGEST_SIZE);

	void * new_msg;
	new_msg=message_create(TYPE_PAIR(GENERAL_RETURN,UUID),recv_msg);
    if(new_msg==NULL)
        return -EINVAL;
    ret=message_add_record(new_msg,input);
	if(ret<0)
		return ret;
	ret=ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}

int proc_randombindata_msg(void * sub_proc,void * recv_msg)
{
	int ret;
	int i;
    RECORD(GENERAL_RETURN,BINDATA) * input;

    ret=message_get_record(recv_msg,&input,0);
    if(input==NULL)
    {
        return -EINVAL;
    }

    input->bindata=Talloc0(input->size);
    RAND_bytes(input->bindata,input->size);


	void * new_msg;
	new_msg=message_create(TYPE_PAIR(GENERAL_RETURN,BINDATA),recv_msg);
    if(new_msg==NULL)
        return -EINVAL;
    ret=message_add_record(new_msg,input);
	if(ret<0)
		return ret;
	ret=ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}
