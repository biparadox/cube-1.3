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
#include "json.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "basefunc.h"
#include "memdb.h"
#include "message.h"
#include "connector.h"
#include "ex_module.h"
#include "channel.h"
#include "sys_func.h"

#include "msg2channel.h"

#define MAX_LINE_LEN 1024

static unsigned char Buf[DIGEST_SIZE*128];
static BYTE * ReadBuf=Buf;
static int readbuf_len=0;
static BYTE * WriteBuf=Buf+DIGEST_SIZE*64;
static int write_len=0;

static int index = 0;
static char errorbuf[1024];
static unsigned char sendbuf[4096];
static CHANNEL * msg2channel;
static void * extend_template;

int msg2channel_init(void * sub_proc,void * para)
{
    int ret;
    struct msg2channel_init_para * init_para=para;
    if(para==NULL)
	return -EINVAL;
    msg2channel=channel_find(init_para->channel_name);

    if(msg2channel==NULL)
	return -EINVAL;	
    return 0;
}

int msg2channel_start(void * sub_proc,void * para)
{
    int ret = 0, len = 0, i = 0, j = 0;
    int rc = 0;

    int offset=0;	
    int readbuf_len=0;
	BYTE local_uuid[DIGEST_SIZE];	
        ret=proc_share_data_getvalue("uuid",local_uuid);
	if(ret<0)
		return ret;

    while(1)
    {
        usleep(time_val.tv_usec/10);
	ret=channel_read(msg2channel,ReadBuf+readbuf_len,DIGEST_SIZE*32-readbuf_len);
	if(ret<0)
		return ret;
	if(ret>0)
	{
		print_cubeaudit("msg2channel read %d data",ret);
	    	void * message;
		MSG_HEAD * msg_head;
		int str_size;
		readbuf_len+=ret;
	
  		ret=json_2_message(ReadBuf+offset,&message);
	   	if(ret>=0)
	    	{
			str_size=ret;
			if(message_get_state(message)==0)
				message_set_state(message,MSG_FLOW_INIT);
			msg_head=message_get_head(message);
			if(Isstrinuuid(msg_head->nonce))
			{
				ret=get_random_uuid(msg_head->nonce);
				if(ret<0)
					break; 
			}	
			message_set_sender(message,local_uuid);
	    	    	ex_module_sendmsg(sub_proc,message);	
			offset+=str_size;
			if(readbuf_len-offset<sizeof(MSG_HEAD))
			{
					readbuf_len=0;
					offset=0;
			}
		}
		else
		{
			print_cubeerr("resolve channel message failed!\n");
			print_cubeerr("error channel message: %s\n",ReadBuf+offset);
			readbuf_len=0;
		}
	}
					
     	void *message_box ;
     	if((ex_module_recvmsg(sub_proc,&message_box)>=0)
		&&(message_box!=NULL))
     	{
            int stroffset;		 
	    	stroffset=message_output_json(message_box,WriteBuf);
	    	if(stroffset>0)
			{	

	   	 		len=channel_write(msg2channel,WriteBuf,stroffset);
            	if (len != stroffset)
                	print_cubeerr("msg2channel write failed!\n");
				else
					print_cubeaudit("msg2channel write %d data",stroffset);
			}
        }
    }
    return 0;
}
