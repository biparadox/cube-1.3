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
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "message.h"
#include "ex_module.h"

#include "symm_crypt.h"

static struct timeval time_val={0,50*1000};
static char passwd[DIGEST_SIZE];
unsigned char iv[16] = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10};

int symm_crypt_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
    	struct init_struct * init_para=para;
    	if(para==NULL)	 
		return -EINVAL;
	Memset(passwd,0,DIGEST_SIZE);	   
	Strncpy(passwd,init_para->passwd,DIGEST_SIZE);
	return 0;
}

int symm_crypt_start(void * sub_proc,void * para)
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
		if(message_get_flag(recv_msg) &MSG_FLAG_CRYPT)
			proc_uncrypt_message(sub_proc,recv_msg);
		else
			proc_crypt_message(sub_proc,recv_msg);
	}
	return 0;


};

int proc_crypt_message(void * sub_proc,void * message)
{
        int i;
        int ret;

        BYTE * blob;
        int blob_size;

        BYTE* bind_blob;
        int bind_blob_size;

        blob_size=message_get_blob(message,&blob);
        if(blob_size<=0)
                return -EINVAL;

	bind_blob_size=sm4_context_crypt(blob,&bind_blob,blob_size,passwd);
	if(bind_blob_size<0)
		return bind_blob_size;

        message_set_blob(message,bind_blob,bind_blob_size);
        message_set_flag(message,MSG_FLAG_CRYPT);

        ex_module_sendmsg(sub_proc,message);
        return ret;
}

int proc_uncrypt_message(void * sub_proc,void * message)
{
        int i;
        int ret;

        BYTE * blob;
        int blob_size;

        BYTE* bind_blob;
        int bind_blob_size;

        bind_blob_size=message_get_blob(message,&bind_blob);
        if(bind_blob_size<=0)
                return -EINVAL;
	blob_size=sm4_context_decrypt(bind_blob,&blob,bind_blob_size,passwd);
	if(blob_size<0)
		return blob_size;

        message_set_blob(message,blob,blob_size);
	int flag=message_get_flag(message);
        message_set_flag(message,flag&(~MSG_FLAG_CRYPT));

	message_load_record(message);

        ex_module_sendmsg(sub_proc,message);
        return ret;
}
