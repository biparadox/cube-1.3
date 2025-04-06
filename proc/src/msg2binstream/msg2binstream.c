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

#include "msg2binstream.h"

#define MAX_LINE_LEN 1024

static unsigned char Buf[DIGEST_SIZE*128];
static BYTE * ReadBuf=Buf;
static int readbuf_len=0;
static BYTE * WriteBuf=Buf+DIGEST_SIZE*64;
static int write_len=0;

static int index = 0;
static char errorbuf[1024];
static unsigned char sendbuf[4096];
static CHANNEL * binchannel;
static void * extend_template;

int msg2binstream_init(void * sub_proc,void * para)
{
    int ret;
    struct msg2binstream_init_para * init_para=para;
    if(para==NULL)
	return -EINVAL;
    binchannel=channel_find(init_para->channel_name);

    if(binchannel==NULL)
	return -EINVAL;	
    return 0;
}

int msg2binstream_start(void * sub_proc,void * para)
{
    int ret = 0, len = 0, i = 0, j = 0;
    int rc = 0;

    int offset=0;	
    int readbuf_len=0;
	BYTE local_uuid[DIGEST_SIZE];	
    BYTE Buf[DIGEST_SIZE*32];
        ret=proc_share_data_getvalue("uuid",local_uuid);
	if(ret<0)
		return ret;

    RECORD(MESSAGE,TYPES) type_pair;
    void * record;
    void * new_msg;
    void * struct_template;

    while(1)
    {
        usleep(time_val.tv_usec*10);
	    ret=channel_read(binchannel,ReadBuf+readbuf_len,DIGEST_SIZE*32-readbuf_len);
	    if(ret<0)
		    return ret;
	    if(ret>0)
	    {
		    print_cubeaudit("msg2binstream read %d  %d data %s",ret,offset,ReadBuf+offset);
            offset=0;
	   	    void * message;
		    MSG_HEAD * msg_head;
		    int str_size;
		    readbuf_len+=ret;

            Memcpy(&type_pair,ReadBuf+offset,sizeof(type_pair));

            offset+=sizeof(type_pair);

            struct_template=memdb_get_template(type_pair.type,type_pair.subtype);
            if(struct_template==NULL)
                return -EINVAL;

            record=Talloc0(struct_size(struct_template));
            if(record==NULL)
                return -ENOMEM;

            ret = blob_2_struct(ReadBuf+offset,record,struct_template);
            if(ret<0)
                return ret;

            new_msg=message_create(type_pair.type,type_pair.subtype,NULL);
            if(new_msg == NULL)
                return -EINVAL;
        
            ret=message_add_record(new_msg,record);
            if(ret<0)
                return -EINVAL;

	   	    ex_module_sendmsg(sub_proc,new_msg);	
	    }
    					
        void *message_box ;
        if((ex_module_recvmsg(sub_proc,&message_box)>=0)
	        &&(message_box!=NULL))
        {
            type_pair.type=message_get_type(message_box);        
            type_pair.subtype=message_get_subtype(message_box);    
            offset=0;
            Memcpy(Buf,&type_pair,sizeof(type_pair));

            offset+=sizeof(type_pair);

            ret =  message_get_record(message_box, &record,0);
            if(ret<0)
                return ret;
            struct_template=memdb_get_template(type_pair.type,type_pair.subtype);
            if(struct_template==NULL)
                return -EINVAL;

            ret=struct_2_blob(record,Buf+offset,struct_template);
            if(ret<0)
                return -EINVAL;
            offset+=ret;
   	 		len=channel_write(binchannel,Buf,offset);
           	if (len != offset)
           	print_cubeerr("msg2binstream write failed!\n");
			else
				print_cubeaudit("msg2binstream write %d data %s",offset,WriteBuf);
		}
    }
    return 0;
}
