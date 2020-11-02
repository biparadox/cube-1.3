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
#include "crypto_func.h"
#include "memdb.h"
#include "message.h"
#include "ex_module.h"
#include "../include/app_struct.h"
#include "../include/server_struct.h"
#include "sm4_attack.h"


static struct timeval time_val={0,50*1000};
static char passwd[DIGEST_SIZE];

int sm4_attack_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	return 0;
}

int sm4_attack_start(void * sub_proc,void * para)
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
		if(message_get_flag(recv_msg) &MSG_FLAG_CRYPT)
			proc_hack_message(sub_proc,recv_msg);
		else
			proc_transfer_message(sub_proc,recv_msg);
	}
	return 0;
}

int proc_transfer_message(void * sub_proc,void * message)
{
	int type;
	int subtype;
	int i;
	int ret;
	printf("begin proc echo \n");
	struct message_box * msg_box=message;
	type=message_get_type(message);
	subtype=message_get_subtype(message);

	ex_module_sendmsg(sub_proc,message);
	return ret;
}

int proc_hack_message(void * sub_proc,void * message)
{
        int i;
        int ret;
	int type;
	int subtype;

        BYTE * blob;
        int blob_size;

        BYTE* bind_blob;
        int bind_blob_size;
	type=message_get_type(message);
	subtype=message_get_subtype(message);

        bind_blob_size=message_get_blob(message,&bind_blob);
        if(bind_blob_size<=0)
                return -EINVAL;
	char brute_pass[9];

	BYTE comp_value[DIGEST_SIZE/4*3];
	Memset(comp_value,0,DIGEST_SIZE/4*3);

	for(i=0;i<10000000;i++)
	{
	
		sprintf(brute_pass,"%d",i);
		blob_size=sm4_context_decrypt(bind_blob,&blob,bind_blob_size,brute_pass);
		if(blob_size<0)
			return blob_size;

		if(i>123455)
			printf("test to 123455 \n");
		if(Memcmp(blob+DIGEST_SIZE/2,comp_value,DIGEST_SIZE/4*3)==0)
		{
        		message_set_blob(message,blob,blob_size);
			int flag=message_get_flag(message);
        		message_set_flag(message,flag&(~MSG_FLAG_CRYPT));
			message_load_record(message);
			break;
		}
		free(blob);
	}

        ex_module_sendmsg(sub_proc,message);
        return ret;
}
