#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "data_type.h"
#include "cube.h"
#include "cube_define.h"
#include "cube_record.h"
#include "file_struct.h"
#include "sm4_attack.h"

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
		if(message_get_flag(recv_msg) &MSG_FLAG_CRYPT)
			proc_hack_message(sub_proc,recv_msg);
		else
			proc_transfer_message(sub_proc,recv_msg);
	}
	return 0;
}

int proc_transfer_message(void * sub_proc,void * message)
{
	int ret = ex_module_sendmsg(sub_proc,message);
	return ret;
}

int proc_hack_message(void * sub_proc,void * message)
{
    int i,ret;

    BYTE * blob;
    int blob_size;

    BYTE* bind_blob;
    int bind_blob_size;

    bind_blob_size=message_get_blob(message,(void **)&bind_blob);
    if(bind_blob_size<=0)
         return -EINVAL;
	char brute_pass[9];

	for(i=0;i<10000000;i++)
	{
		sprintf(brute_pass,"%d",i);
		blob_size=sm4_context_decrypt(bind_blob,&blob,bind_blob_size,brute_pass);
		if(blob_size<0)
			return blob_size;

		if(Strncmp(blob+DIGEST_SIZE,"test.txt",DIGEST_SIZE)==0)
		{
        	message_set_blob(message,blob,blob_size);
			int flag=message_get_flag(message);
            message_set_flag(message,flag&(~MSG_FLAG_CRYPT));
			message_load_record(message);
			printf("Decrypt data succeed passwd=%d\n",i);
			break;
		}
	}
    ex_module_sendmsg(sub_proc,message);
    return ret;
}
