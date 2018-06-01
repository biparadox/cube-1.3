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
#include "basetype.h"
#include "message.h"
#include "ex_module.h"
#include "../include/file_struct.h"

#include "request.h"

static struct timeval time_val={0,50*1000};
static char filename[DIGEST_SIZE*4];
int proc_request_message(void * sub_proc,void * message);
int proc_notice_message(void * sub_proc,void * message);

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
	int ret;
	void * recv_msg;
	int i;
	int type;
	int subtype;

	// send init message
//	usleep(10*1000);

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
		if((type ==TYPE(MESSAGE)) &&
			(subtype ==SUBTYPE_CONN_SYNI))
		{
			proc_request_message(sub_proc,NULL);
		}
		else if((type ==TYPE(FILE_TRANS)) &&
			(subtype ==SUBTYPE_FILE_NOTICE))
		{
			proc_notice_message(sub_proc,recv_msg);
		}
		else
			printf("file_request receive error message!\n");
	}

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


int proc_notice_message(void * sub_proc,void * message)
{
	int ret;
	struct policyfile_notice * notice;
	char uuidname[DIGEST_SIZE*2+1];
	printf("receive proc notice \n");

	ret=message_get_record(message,(void **)(&notice),0);
	if(ret<0)
		return ret;		
	digest_to_uuid(notice->uuid,uuidname);
	uuidname[DIGEST_SIZE*2]=0;
	if(notice->result==1)
		printf("send file %s succeed,digest is %s!\n",notice->filename,uuidname);
	else
		printf("send file %s failed!\n",notice->filename);
	
	return notice->result;
}
