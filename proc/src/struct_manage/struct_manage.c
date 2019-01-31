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
#include "sys_func.h"
#include "memdb_internal.h" 

char Buf[DIGEST_SIZE*32];

int struct_manage_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	return 0;
}

int struct_manage_start(void * sub_proc,void * para)
{
	int ret;
	void * recv_msg;
	int i;
	int type;
	int subtype;
	int action=0;
	

	sleep(1);

	sleep(1);




	while(1)
	{
		usleep(time_val.tv_usec);
		ret=ex_module_recvmsg(sub_proc,&recv_msg);
		if(ret<0)
			continue;
		if(recv_msg==NULL)
			continue;
		if(action==0)
		{
			action=1;
			__proc_output_describe(sub_proc,DB_TYPELIST,0,NULL,NULL);
			{
/*
				struct types_pair test_types;
				void * send_msg;
				// test subtypelist
				test_types.type=DB_SUBTYPELIST;
				test_types.subtype=0;
				send_msg=message_create(DTYPE_MESSAGE,SUBTYPE_TYPES,NULL);
				if(send_msg==NULL)
					return -EINVAL;
				message_add_record(send_msg,&test_types);
				test_types.type=DTYPE_MESSAGE;
				message_add_expand_data(send_msg,DTYPE_MESSAGE,SUBTYPE_TYPES,&test_types);
				ex_module_sendmsg(sub_proc,send_msg);
				sleep(1);
*/
			}
		}
		
		type=message_get_type(recv_msg);
		subtype=message_get_subtype(recv_msg);
		if(!memdb_find_recordtype(type,subtype))
		{
			printf("message format (%d %d) is not registered!\n",
				message_get_type(recv_msg),message_get_subtype(recv_msg));
			continue;
		}
		if((type==DTYPE_MESSAGE)&&(subtype==SUBTYPE_TYPES))
		{
			ret=proc_output_describe(sub_proc,recv_msg);
/*
			if(action==1)
			{
				action=2;
				struct types_pair test_types;
				void * send_msg;

				// test recordtype
				test_types.type=DB_RECORDTYPE;
				test_types.subtype=0;
				send_msg=message_create(DTYPE_MESSAGE,SUBTYPE_TYPES,NULL);
				if(send_msg==NULL)
					return -EINVAL;
				message_add_record(send_msg,&test_types);
				test_types.type=DTYPE_MESSAGE;
				test_types.subtype=SUBTYPE_TYPES;
				message_add_expand_data(send_msg,DTYPE_MESSAGE,SUBTYPE_TYPES,&test_types);
				ex_module_sendmsg(sub_proc,send_msg);
				sleep(1);
			}
*/

		}
		
	}
	return 0;
};

int proc_output_describe(void * sub_proc,void * message)
{
	int type;
	int subtype;
	int i;
	int ret;
	printf("begin proc echo \n");

	type=message_get_type(message);
	subtype=message_get_subtype(message);
	if((type!=DTYPE_MESSAGE) || (subtype!=SUBTYPE_TYPES))
		return -EINVAL;

	struct types_pair * types;
	ret=message_get_record(message,&types,0);
	if(ret<0)
		return ret;
	if(types==NULL)
		return 0;
	
	switch(types->type)
	{
		case DB_TYPELIST:
			return __proc_output_describe(sub_proc,types->type,types->subtype,message);
		case DB_SUBTYPELIST:
		case DB_RECORDTYPE:
		{
			MSG_EXPAND * msg_expand;
			ret=message_get_define_expand(message,&msg_expand,DTYPE_MESSAGE,SUBTYPE_TYPES);
			if(ret<0)
				return -EINVAL;
			if(msg_expand==NULL)
				return __proc_output_describe(sub_proc,types->type,types->subtype,message,NULL);
			else
				return __proc_output_describe(sub_proc,types->type,types->subtype,message,msg_expand->expand);
		}
		default:
			return -EINVAL;
	}
}

int __proc_output_describe(void * sub_proc,int type,int subtype,void * message,void * expand)
{
	int ret;

	int i=0;
	int count=0;
	int total=0;

	DB_RECORD * record;
	void * send_msg;
	MSG_EXPAND * expand_data;

	switch(type)
	{
		case DB_TYPELIST:
		{
			struct struct_typelist * type_record;
			struct struct_namelist * type_namelist;

        		record=memdb_get_first(type,subtype);
        		while(record!=NULL)
        		{
				send_msg=message_create(type,subtype,message);
				if(send_msg==NULL)
					return -EINVAL;

				total++;
				message_add_record(send_msg,record->record);
				type_record=record->record;
				record=memdb_find(type_record->uuid,DB_NAMELIST,0);
				if(record==NULL)
					return -EINVAL;
				message_add_expand_data(send_msg,DB_NAMELIST,0,record->record);	
				ex_module_sendmsg(sub_proc,send_msg);
				usleep(50*1000);
                		record=memdb_get_next(type,subtype);
			}
			break;
		}
		case DB_SUBTYPELIST:
		{
			struct types_pair * types_expand;
			struct struct_subtypelist * subtype_record;
			struct struct_namelist * type_namelist;
			types_expand=expand;
			if(expand==NULL)
				return -EINVAL;

        		record=memdb_get_first(type,0);
			while(record!=NULL)
			{
				subtype_record=record->record;
				if(subtype_record->type==types_expand->type)
					break;
        			record=memdb_get_next(type,0);
			}
			if(record==NULL)	
				return -EINVAL;
			send_msg=message_create(type,subtype,message);
			if(send_msg==NULL)
				return -EINVAL;
			message_add_record(send_msg,subtype_record);
			record=memdb_find(subtype_record->uuid,DB_NAMELIST,0);
			if(record==NULL)
				return -EINVAL;
			message_add_expand_data(send_msg,DB_NAMELIST,0,record->record);	
			ex_module_sendmsg(sub_proc,send_msg);
			usleep(50*1000);
			break;
		}	
		case DB_RECORDTYPE:
		{
			struct types_pair * types_expand;
			struct struct_recordtype * recordtype_record;
			struct struct_desc_record * desc_record;
			types_expand=expand;
			if(expand==NULL)
				return -EINVAL;

        		record=memdb_get_first(type,0);
			while(record!=NULL)
			{
				recordtype_record=record->record;
				if((recordtype_record->type==types_expand->type)
					&&(recordtype_record->subtype==types_expand->subtype))
					break;
        			record=memdb_get_next(type,0);
			}
			if(record==NULL)	
				return -EINVAL;
			send_msg=message_create(type,subtype,message);
			if(send_msg==NULL)
				return -EINVAL;
			message_add_record(send_msg,recordtype_record);
			record=memdb_find(recordtype_record->uuid,DB_STRUCT_DESC,0);
			if(record==NULL)
				return -EINVAL;
			message_add_expand_data(send_msg,DB_STRUCT_DESC,0,record->record);	
			ex_module_sendmsg(sub_proc,send_msg);
			usleep(50*1000);
			break;
			
		}
		default:
			break;
        }
	return ret;
}
