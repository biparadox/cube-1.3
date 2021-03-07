#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
 
#include "data_type.h"
#include "cube.h"
#include "cube_define.h"
#include "cube_record.h"

#include "record_find.h"

// add para lib_include
int record_find_init(void * sub_proc, void * para)
{
	int ret;
	// Init Tcm Func
//	ret=TCM_LibInit();

	// add yorself's module init func here
	return 0;
}
int record_find_start(void * sub_proc, void * para)
{
	int ret;
	void * recv_msg;
	int type;
	int subtype;
	// add yorself's module exec func here
	sleep(1);

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
		if((type==TYPE(GENERAL_RETURN))&&(subtype==SUBTYPE(GENERAL_RETURN,STRING)))
		{
			ret=proc_find_record_byname(sub_proc,recv_msg);
		}
		if((type==TYPE(GENERAL_RETURN))&&(subtype==SUBTYPE(GENERAL_RETURN,UUID)))
		{
			ret=proc_find_record_byuuid(sub_proc,recv_msg);
		}
	}
	return 0;
}

int proc_find_record_byname(void * sub_proc,void * recv_msg)
{
	int ret=0;

    RECORD(MESSAGE,TYPES) * msg_types;
    RECORD(GENERAL_RETURN,STRING) * find_name;

	DB_RECORD * db_record;

    MSG_EXPAND * msg_expand;
	void * new_msg;

	printf("begin node query!\n");
	// get the query info
	ret =message_get_record(recv_msg,&find_name,0);
	if(ret<0)
		return -EINVAL;

    ret =message_get_define_expand(recv_msg,&msg_expand,TYPE_PAIR(MESSAGE,TYPES));
    if(ret<0)
        return -EINVAL;
    if(msg_expand==NULL)
        return -EINVAL;
    msg_types=msg_expand->expand;
    if(msg_types==NULL)
        return -EINVAL;

    db_record=memdb_find_byname(find_name->return_value,msg_types->type,msg_types->subtype);
    if(db_record==NULL)
        return -EINVAL;
	if(db_record!=NULL)
	{
	    new_msg=message_create(msg_types->type,msg_types->subtype,recv_msg);	
	    if(new_msg==NULL)
		    return -EINVAL;
	    ret=message_add_record(new_msg,db_record->record);
		if(ret<0)
			return ret;
	}

	ret=ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}

int proc_find_record_byuuid(void * sub_proc,void * recv_msg)
{
	int ret=0;

    RECORD(MESSAGE,TYPES) * msg_types;
    RECORD(GENERAL_RETURN,UUID) * find_uuid;

	DB_RECORD * db_record;

    MSG_EXPAND * msg_expand;
	void * new_msg;

	printf("begin node query!\n");
	// get the query info
	ret =message_get_record(recv_msg,&find_uuid,0);
	if(ret<0)
		return -EINVAL;

    ret =message_get_define_expand(recv_msg,&msg_expand,TYPE_PAIR(MESSAGE,TYPES));
    if(ret<0)
        return -EINVAL;
    if(msg_expand==NULL)
        return -EINVAL;
    msg_types=msg_expand->expand;
    if(msg_types==NULL)
        return -EINVAL;

    db_record=memdb_find(find_uuid->return_value,msg_types->type,msg_types->subtype);
    if(db_record==NULL)
        return -EINVAL;
	if(db_record!=NULL)
	{
	    new_msg=message_create(msg_types->type,msg_types->subtype,recv_msg);	
	    if(new_msg==NULL)
		    return -EINVAL;
	    ret=message_add_record(new_msg,db_record->record);
		if(ret<0)
			return ret;
	}

	ret=ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}
