#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "basetype.h"
#include "message.h"
#include "ex_module.h"
#include "sys_func.h"
#include "channel.h"
#include "echo_channel.h"

CHANNEL * echo_channel;
BYTE * Buffer;

int echo_channel_init(void * sub_proc,void * para)
{

    int ret;
    struct echo_init_para * init_para=para;

    char local_uuid[DIGEST_SIZE];
    char proc_name[DIGEST_SIZE];
 	
    if(init_para==NULL)
    {
	   print_cubeerr("can't find websocket port address!\n");
	   return -EINVAL;
    }	


    Buffer=Dalloc(4096,NULL);
    Memset(Buffer,0,4096);


    echo_channel=channel_find(init_para->channel_name);
    if(echo_channel==NULL)
	return -EINVAL;
   return 0;
}

int echo_channel_start(void * sub_proc,void * para)
{
    int ret;
    int retval;
    void * message_box;
    void * context;
    int i;
    struct timeval conn_val;
    conn_val.tv_usec=time_val.tv_usec;

    char local_uuid[DIGEST_SIZE];
    char proc_name[DIGEST_SIZE];
    int stroffset;
    int str_size;
	 int offset=0;
	
    print_cubeaudit("begin echo channel process!\n");
    ret=proc_share_data_getvalue("uuid",local_uuid);
    if(ret<0)
        return ret;
    ret=proc_share_data_getvalue("proc_name",proc_name);

    if(ret<0)
	return ret;

    while(1)
    {
        usleep(time_val.tv_usec);
        // read ex_channel 
        ret=channel_read(echo_channel,Buffer,4096);
        if(ret<0)
                return ret;
        if(ret>0)
	{
		print_cubeaudit("read %d data!\n",ret);
		offset=channel_write(echo_channel,Buffer,ret);	
		print_cubeaudit("echo %d data!\n",ret);
	}
	// send message to the remote
	
    }
    return 0;
}
