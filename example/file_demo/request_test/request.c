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
#include "basetype.h"
#include "message.h"
#include "ex_module.h"
#include "../include/file_struct.h"

#include "request.h"

static struct timeval time_val={0,50*1000};
static char filename[DIGEST_SIZE*4];

int request_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
    	struct init_struct * init_para=para;
    	if(para==NULL)	 
		return -EINVAL;
	Memset(filename,0,DIGEST_SIZE*4);	   
	Strncpy(filename,init_para->filename,DIGEST_SIZE*4);
	return 0;
}

int request_start(void * sub_proc,void * para)
{
	proc_request_message(sub_proc,NULL);
	return 0;
};

int proc_request_message(void * sub_proc,void * message)
{
	int ret;
	printf("begin proc request \n");

	void * new_msg;
	new_msg=message_create(DTYPE_FILE_TRANS,SUBTYPE_FILE_REQUEST,message);
	struct policyfile_req * file_req;
	file_req=Talloc0(sizeof(*file_req));
	file_req->filename=dup_str(filename,0);
	ret=message_add_record(new_msg,file_req);
	if(ret<0)
		return ret;
	ret=ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}
