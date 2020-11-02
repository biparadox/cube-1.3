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

struct timeval time_val={0,50*1000};

int proc_dispatch(void * sub_proc,void * message);

int msg_broadcast_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	return 0;
}

int msg_broadcast_start(void * sub_proc,void * para)
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
		proc_broadcast(sub_proc,recv_msg);
	}

	return 0;
};

int proc_broadcast(void * sub_proc,void * message)
{
	int type;
	int subtype;
	int ret;
	printf("begin proc login_verify \n");

	type=message_get_type(message);
	if(type!=DTYPE_CRYPTO_DEMO)
		return -EINVAL;
	subtype=message_get_subtype(message);
	if(subtype!=SUBTYPE_CHAT_MSG)
		return -EINVAL;

	void * new_msg;
	void * record;
	
	struct session_msg  * chat_msg;
	struct uuid_record * receiver_addr;
	struct user_address * user_addr;
	struct DB_RECORD * db_record;

/*	

	ret=message_get_record(message,&chat_msg,0);
	if(ret<0)
		return ret;
	if(chat_msg==NULL)
		return -EINVAL;	
	
	db_record=memdb_find_byname(chat_msg->sender,DTYPE_CRYPTO_DEMO,SUBTYPE_USER_ADDR);
	if(db_record==NULL)
	{
		printf("user %s has not logined yet!\n",chat_msg->sender);
		return -EINVAL;
	}
*/

	user_addr=memdb_get_first_record(DTYPE_CRYPTO_DEMO,SUBTYPE_USER_ADDR);
	while(user_addr!=NULL)
	{

		receiver_addr=Talloc0(sizeof(*receiver_addr));
		if(receiver_addr==NULL)
			return -ENOMEM;
		Memcpy(receiver_addr->uuid,user_addr->addr,DIGEST_SIZE);
		new_msg=message_clone(message);
		if(new_msg==NULL)
			return -EINVAL;
		ret=message_add_expand_data(new_msg,DTYPE_MESSAGE,SUBTYPE_UUID_RECORD,receiver_addr);
		if(ret<0)
			return ret;
		ex_module_sendmsg(sub_proc,new_msg);
		user_addr=memdb_get_next_record(DTYPE_CRYPTO_DEMO,SUBTYPE_USER_ADDR);
	}
	return 0;
}
