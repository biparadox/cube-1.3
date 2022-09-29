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

#include "utc_timer.h"
extern struct timeval time_val;

char * default_name;
int  default_mode=0;

int utc_timer_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
    struct init_para * init_para=para;
	
	if(init_para!=NULL)
	{
        default_name=dup_str(init_para->label_string,0);
        default_mode = init_para->mode;
	}
	return 0;
}

int utc_timer_start(void * sub_proc,void * para)
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
        proc_utc_timer_addtime(sub_proc,recv_msg);
        
	}
	
	return 0;
};

int proc_utc_timer_addtime(void * sub_proc,void * recv_msg)
{
	int ret;
	int i;
    RECORD(GENERAL_RETURN,TIME_VAL) * output_time;
    struct timeval curr_time;
    struct timezone curr_timezone;

       ret = gettimeofday(&curr_time,&curr_timezone); 
            output_time=Talloc0(sizeof(*output_time));
            if(output_time==NULL)
                return -ENOMEM;
            if(default_name!=NULL)
                output_time->name=dup_str(default_name,0);
            output_time->tv_sec=curr_time.tv_sec;
            output_time->tv_usec=curr_time.tv_usec;


    switch(default_mode) {
        case 0:
        {
            printf(" curr time: second %d usecond %d \n",output_time->tv_sec,output_time->tv_usec);
            break;
        }
        case 1:
        {
            ret = message_add_expand_data(recv_msg,TYPE_PAIR(GENERAL_RETURN,TIME_VAL),output_time);
            if(ret<0)
                return ret;
            break;
        }
        case 2:
        {
            void * new_msg = message_create(TYPE_PAIR(GENERAL_RETURN,TIME_VAL),NULL);
            if(new_msg==NULL)
                return -EINVAL;
            message_add_record(new_msg,output_time);
            ret = ex_module_sendmsg(sub_proc,new_msg);
            break;
        }
        default:
            return -EINVAL;

    }

	ret=ex_module_sendmsg(sub_proc,recv_msg);
	return ret;
}
