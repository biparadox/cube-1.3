
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include  "../include/kernel_comp.h"
#include "../include/list.h"
#include "../include/attrlist.h"
#include "../include/data_type.h"
#include "../include/struct_deal.h"
#include "../include/message_struct.h"
#include "../include/vmlist.h"
#include "../include/vtpm_struct.h"
#include "../include/vm_policy.h"
#include "../include/valuename.h"
#include "../include/message_struct_desc.h" 
#include "../include/vm_policy_desc.h"
#include "../include/vmlist_desc.h"
#include "../include/vtpm_desc.h"


#include "message_box.h"

int message_read_head(void ** message,void * blob,int blob_size)
{
	struct message_box * msg_box;
	int ret;
	MESSAGE_HEAD * msg_head;
	BYTE * data;
	BYTE * buffer;
	int i,j;
	int record_size,expand_size;
	int head_size;
	int current_offset;
	int total_size;
	

	if(message==NULL)
		return -EINVAL;
        if(blob_size<sizeof(MESSAGE_HEAD))
        	return -EINVAL;
	if(blob==NULL)
		return -EINVAL;

	msg_box=message_init("MESG",0x00010001);
	if(msg_box==NULL)
		return -EINVAL;

	if(msg_box->box_state != MSG_BOX_INIT)
		return -EINVAL;

	head_size=blob_2_struct(blob,&(msg_box->head),msg_box->head_template);
	if(head_size<=0)
		return -EINVAL;
	msg_box->head_size=head_size;
        msg_box->current_offset=0;

	// check the head value
	if(msg_box->head.record_num<=0)
		return -EINVAL;
	if(msg_box->head.expand_num<0)
		return -EINVAL;
	if(msg_box->head.expand_num>MAX_EXPAND_NUM)
		return -EINVAL;

	if(strncmp(msg_box->head.tag,"MESG",4)!=0)
		return -EINVAL;
	if(msg_box->head.version!=0x00010001)
		return -EINVAL;

    	msg_box->box_state=MSG_BOX_LOADDATA;

   	total_size=msg_box->head.record_size+msg_box->head.expand_size;
	data=kmalloc(total_size,GFP_KERNEL);
	if(data==NULL)
		return -ENOMEM;
   	 msg_box->blob=data;
	 *message=msg_box;
   	 return sizeof(MESSAGE_HEAD);
}

int  message_read_data(void * message,void * blob,int data_size)
{
	struct message_box * msg_box;
	int ret;
	MESSAGE_HEAD * msg_head;
	BYTE * data;
	int new_size;
	int current_offset;
	int total_size;

	msg_box=(struct message_box *)message;
        if(msg_box->box_state!=MSG_BOX_LOADDATA)
                return -EINVAL;

	if(message==NULL)
		return -EINVAL;
	if(blob==NULL)
		return -EINVAL;
	
	if(data_size<=0)
		return -EINVAL;

	data=msg_box->blob;

        total_size=msg_box->head.record_size+msg_box->head.expand_size;

        if(data_size+msg_box->current_offset>=total_size)
	{
		new_size=total_size-msg_box->current_offset;
		memcpy(data+msg_box->current_offset,blob,new_size);
		msg_box->current_offset=0;
                current_offset=msg_box->head.record_size;
		int i;
		for(i=0;i<msg_box->head.expand_num;i++)
		{
			msg_box->expand[i]=data+current_offset;
			msg_box->expand_size[i]=*(int *)(data+current_offset);
			current_offset+=msg_box->expand_size[i];
		}
                __message_alloc_record_site(msg_box);
                msg_box->current_offset=0;
                msg_box->box_state=MSG_BOX_REBUILDING;
		return new_size;
	}	

	memcpy(data+msg_box->current_offset,blob,data_size);
	msg_box->current_offset+=data_size;
	return 0;
}

int message_read_from_blob(void ** message,void * blob, int data_size)
{
    const int buf_size=1024;
    int offset=0;
    int retval;
    int left_size;

    retval=message_read_head(message,blob,data_size);
    if(retval<0)
        return retval;
    offset=retval;
    left_size=data_size-retval;
    retval=message_read_data(*message,blob+offset,left_size);
    if(retval<0)
        return retval;
    return retval+offset;
}

int message_read_from_src(void ** message,void * src,
           int (*read_func)(void *,char *,int size))
{
    const int fixed_buf_size=4096;
    char readbuf[fixed_buf_size];
    struct message_box *message_box;
    MESSAGE_HEAD * message_head;
    int read_size;
    int seek_size;
    int offset=0;
    int ret;
    int retval;
    int message_size;


    seek_size=0;
    ret=read_func(src,readbuf,sizeof(MESSAGE_HEAD));
    if(ret<sizeof(MESSAGE_HEAD))
        return ret;

    retval=message_read_head(message,readbuf,ret);
    if(retval<0)
         return -EINVAL;
     seek_size=sizeof(MESSAGE_HEAD);
     message_head=get_message_head(*message);
   
     message_box=(struct message_box *)(*message);

    message_size = message_head->record_size+message_head->expand_size;

    while(offset<message_size)
    {
        read_size=message_size-offset;
        if(read_size>fixed_buf_size)
        {
            read_size=fixed_buf_size;
        }

        ret=read_func(src,readbuf,read_size);
        if(ret<=0)
        {
            message_free(*message);
            break;
        }
        retval=message_read_data(*message,readbuf,ret);
        if(retval<read_size)
            break;
        offset+=read_size;
        seek_size+=read_size;
    }
    return seek_size;
}

int message_load_record_template(void * message)
{

    struct message_box * msg_box;
    int ret;
    MESSAGE_HEAD * msg_head;
    BYTE * data;
    BYTE * buffer;
    int i,j;
    int record_size,expand_size;
    int head_size;

    struct struct_elem_attr * record_desc;

    msg_box=(struct message_box *)message;

    if(message==NULL)
        return -EINVAL;

    msg_box->record_template=load_record_template(msg_box->head.record_type);
    if(msg_box->record_template==NULL)
    {
        printf("error:invalid type!\n");
        return -EINVAL ;
    }

    return 0;
}


int message_load_record(void * message)
// this function read all the record from blob, and output these record
// struct to the precord point in the message_box
{

    struct message_box * msg_box;
    int ret;
    MESSAGE_HEAD * msg_head;
    BYTE * data;
    BYTE * buffer;
    int i,j;
    int record_size,expand_size;
    int head_size;
    int no;

    struct struct_elem_attr * record_desc;

    msg_box=(struct message_box *)message;

    if(message==NULL)
        return -EINVAL;

// if(msg_box->box_state!=MSG_BOX_REBUILDING)
//       return -EINVAL;

    // choose the record's type
    if(msg_box->record_template==NULL)
    {
        ret= message_load_record_template(message);
        if(ret!=0)
            return ret;
    }


//  __message_alloc_record_site(msg_box);
//  msg_box->current_offset=msg_box->head_size;

    no=0;
    while(msg_box->record[no]!=NULL)
    {
        if(no>=msg_box->head.record_num)
            return 0;
        no++;
    }

    for(;no<msg_box->head.record_num;no++)
    {
        void * record;
        msg_box->record[no]=(BYTE *)(msg_box->blob)+msg_box->current_offset;
        ret=message_get_record(msg_box,&record,no);
        if(ret<0)
            return ret;
        if(record==NULL)
            return -EINVAL;
        msg_box->current_offset+=msg_box->record_size[no];
    }

//	record_size=blob_2_struct((BYTE *)(msg_box->record[no]),record,msg_box->record_template);
    if(ret<0)
        return ret;
    msg_box->current_offset+=msg_box->record_size[no];
//	msg_box->record_size[no]=record_size;
    return 1;
}

int message_load_expand(void * message)
// this function read all the expand from blob, and output these expand 
// struct to the pexpand point in the message_box
{

    struct message_box * msg_box;
    int ret;
    MESSAGE_HEAD * msg_head;
    MESSAGE_EXPAND * expand;
    BYTE * data;
    BYTE * buffer;
    int i,j;
    int expand_size;
    int head_size;
    int no;
    int offset;
    void * struct_template;
    void * expand_struct;

    msg_box=(struct message_box *)message;

    if(message==NULL)
        return -EINVAL;

//    if(msg_box->box_state!=MSG_BOX_REBUILDING)
//        return -EINVAL;

    // choose the record's type

    no=0;
    offset=msg_box->head.record_size;

    data=msg_box->blob+offset;
    offset=0;
    for(;no<msg_box->head.expand_num;no++)
    {
	if(offset>=msg_box->head.expand_size)
		return -EINVAL;
	    
	expand=(MESSAGE_EXPAND *)data;
	if(expand->data_size<0)
		return -EINVAL;
	offset+=expand->data_size;
	struct_template=load_record_template(expand->tag);
	if(struct_template==NULL)
	{
		data+=expand->data_size;
		continue;
	}
	ret=alloc_struct(&buffer,struct_template);
	if(ret<0)
		return -EINVAL;
	if(buffer==NULL)
		return -EINVAL;
	ret=blob_2_struct(data,buffer,struct_template);
	if(ret!=expand->data_size)
	{
		free_struct(buffer,struct_template);
		return -EINVAL;
	}
	msg_box->pexpand[no]=buffer;
	data+=expand->data_size;

    }
    return offset;
}

int message_free_blob(void * message)
// this function read all the expand from blob, and output these expand 
// struct to the pexpand point in the message_box
{

    struct message_box * msg_box;
    int ret;
    MESSAGE_HEAD * msg_head;
    int i,j;
    int expand_size;
    int head_size;
    int no;
    int offset;
    void * struct_template;
    void * expand_struct;

    msg_box=(struct message_box *)message;

    if(message==NULL)
        return -EINVAL;

    if(msg_box->box_state!=MSG_BOX_REBUILDING)
        return -EINVAL;
    if(msg_box->blob!=NULL)
	    free(msg_box->blob);
    for(i=0;i<msg_box->head.record_num;i++)
    {
	    msg_box->record[i]=NULL;
	    msg_box->record_size[i]=0;
    }
    for(i=0;i<msg_box->head.expand_num;i++)
    {
	    msg_box->expand[i]=NULL;
	    msg_box->expand_size[i]=0;
    }
    msg_box->box_state = MSG_BOX_RECOVER;

    return 0;
}

int add_message_expand_forward(void * message,char * sender_uuid,char * sender_name,char * receiver_uuid, char * receiver_name)
{
	char * buffer;
	struct message_box * msg_box;
	int ret;
	MESSAGE_HEAD * msg_head;
	BYTE * data;
	int i;
	struct expand_data_forward * expand_data;
	void * struct_template;

	msg_box=(struct message_box *)message;

	if(message==NULL)
		return -EINVAL;

    msg_box->box_state=MSG_BOX_EXPAND;
	for(i=0;i<MAX_EXPAND_NUM;i++)
	{
		if(msg_box->expand[i]==NULL)
			break;
		MESSAGE_EXPAND *curr_expand=(MESSAGE_EXPAND *)(msg_box->expand[i]);	

		if(strncmp(msg_box->expand[i],"IDEE",4))
			continue;
		return -EEXIST;
	}
	if(i==MAX_EXPAND_NUM)
		return -EEXIST;
	
	expand_data=kmalloc(sizeof(struct expand_data_forward),GFP_KERNEL);
	if(expand_data==NULL)
		return -ENOMEM;

	memset(expand_data,0,sizeof(struct expand_data_forward));
	
	memcpy(expand_data->tag,"FORE",4);

	if(sender_uuid!=NULL)
		strncpy(expand_data->sender_uuid,sender_uuid,DIGEST_SIZE*2);
	expand_data->sender_name=dup_str(sender_name,256);
	if(receiver_uuid!=NULL)
		strncpy(expand_data->receiver_uuid,receiver_uuid,DIGEST_SIZE*2);
	expand_data->receiver_name=dup_str(receiver_name,256);

	buffer=kmalloc(4096,GFP_KERNEL);

	struct_template=load_record_template("FORE");
	ret=struct_2_blob(expand_data,buffer,struct_template);
	msg_box->expand[i]=kmalloc(ret,GFP_KERNEL);
	if(msg_box->expand[i]==NULL)
	{
		kfree(buffer);
		kfree(expand_data);
		return -ENOMEM;
	}

	msg_box->expand_size[i]=ret;
	memcpy(buffer,&ret,sizeof(int));

	memcpy(msg_box->expand[i],buffer,msg_box->expand_size[i]);
	msg_box->head.expand_num++;
	msg_box->head.expand_size+=ret;	

	kfree(buffer);
	kfree(expand_data);
	return 0;
}

int message_get_define_expand(void * message,void ** addr,char * type)
{
    	struct message_box * msg_box;
	int ret;
	MESSAGE_HEAD * msg_head;
	BYTE * data;
	int struct_size;
	int i;

	msg_box=(struct message_box *)message;

	if(message==NULL)
		return -EINVAL;
	*addr=NULL;

	for(i=0;i<MAX_EXPAND_NUM;i++)
	{
		if(msg_box->pexpand[i]==NULL)
			continue;
		MESSAGE_EXPAND *curr_expand=(MESSAGE_EXPAND *)(msg_box->pexpand[i]);	

		if(strncmp(curr_expand->tag,type,4))
			continue;
		break;
	}
	if(i==MAX_EXPAND_NUM)
		return 0;
	*addr=msg_box->pexpand[i];
	return i;
}

int message_replace_define_expand(void * message,void * addr,char * type)
{
    	struct message_box * msg_box;
	int ret;
	MESSAGE_HEAD * msg_head;
	BYTE * data;
	int struct_size;
	int i;

	msg_box=(struct message_box *)message;
	MESSAGE_EXPAND * temp_expand;

	if(message==NULL)
		return -EINVAL;

//:	msg_box->box_state=MSG_BOX_ADD_EXPAND;
	for(i=0;i<MAX_EXPAND_NUM;i++)
	{
		if(msg_box->pexpand[i]==NULL)
			return 0;
		MESSAGE_EXPAND *curr_expand=(MESSAGE_EXPAND *)(msg_box->pexpand[i]);	

		if(strncmp(curr_expand->tag,type,4))
			continue;
		break;
	}
	if(i==MAX_EXPAND_NUM)
		return 0;

	temp_expand=msg_box->pexpand[i];
	if(msg_box->expand[i]!=NULL)
	{
		msg_box->expand[i]=NULL;
		msg_box->expand_size[i]=0;
	}
	msg_box->pexpand[i]=addr;

	void * struct_template=load_record_template(temp_expand->tag);
	if(struct_template!=NULL)
	{
		free_struct(temp_expand,struct_template);
		free_struct_template(struct_template);
	}
	return i;
}


int message_add_define_expand(void * message, void * addr, void * struct_template)
{
	char * buffer;
    	struct message_box * msg_box;
	int ret;
	MESSAGE_HEAD * msg_head;
	BYTE * data;
	int struct_size;
	int i;

	msg_box=(struct message_box *)message;

	if(message==NULL)
		return -EINVAL;

    msg_box->box_state=MSG_BOX_EXPAND;
	for(i=0;i<MAX_EXPAND_NUM;i++)
	{
		if(msg_box->expand[i]==NULL)
			break;
		MESSAGE_EXPAND *curr_expand=(MESSAGE_EXPAND *)(msg_box->expand[i]);	

		if(strncmp(msg_box->expand[i],"type",4))
			continue;
		return -EEXIST;
	}
	if(i==MAX_EXPAND_NUM)
		return -EEXIST;
	
	buffer=kmalloc(4096,GFP_KERNEL);
	ret=struct_2_blob(addr,buffer,struct_template);
	msg_box->expand[i]=kmalloc(ret,GFP_KERNEL);
	if(msg_box->expand[i]==NULL)
	{
		kfree(buffer);
		return -ENOMEM;
	}

	msg_box->expand_size[i]=ret;
	memcpy(buffer,&ret,sizeof(int));

	memcpy(msg_box->expand[i],buffer,msg_box->expand_size[i]);
	msg_box->head.expand_num++;
	msg_box->head.expand_size+=ret;	

	kfree(buffer);
	return 0;
}

int add_message_expand_identify(void * message,char * user_name,int type,int blob_size,BYTE * blob)
{
	char * buffer;
	struct message_box * msg_box;
	int ret;
	MESSAGE_HEAD * msg_head;
	BYTE * data;
	int i;
	struct expand_data_identify * expand_data;
	void * struct_template;

	msg_box=(struct message_box *)message;

	if(message==NULL)
		return -EINVAL;

	if(blob==NULL)
	{	
		if(blob_size!=0)
			return -EINVAL;
	}

    if((msg_box->box_state!=MSG_BOX_ADD) &&
            (msg_box->box_state!=MSG_BOX_EXPAND))
	{
		return -EINVAL;
	}
    msg_box->box_state=MSG_BOX_EXPAND;
	for(i=0;i<MAX_EXPAND_NUM;i++)
	{
		if(msg_box->expand[i]==NULL)
			break;
		MESSAGE_EXPAND *curr_expand=(MESSAGE_EXPAND *)(msg_box->expand[i]);	

		if(strncmp(msg_box->expand[i],"IDEE",4))
			continue;
		return -EEXIST;
	}
	if(i==MAX_EXPAND_NUM)
		return -EEXIST;
	
	expand_data=kmalloc(sizeof(struct expand_data_identify),GFP_KERNEL);
	if(expand_data==NULL)
		return -ENOMEM;

	memset(expand_data,0,sizeof(struct expand_data_identify));
	

	memcpy(expand_data->tag,"IDEE",4);

	expand_data->user_name=dup_str(user_name,0);
	expand_data->type=type;
	expand_data->blob_size=blob_size;
	if(blob_size>0)
	{
		expand_data->blob=kmalloc(blob_size,GFP_KERNEL);
		memcpy(expand_data->blob,blob,blob_size);
	}
	
	buffer=kmalloc(blob_size+1024,GFP_KERNEL);

	struct_template=load_record_template("IDEE");

	ret=struct_2_blob(expand_data,buffer,struct_template);
	msg_box->expand[i]=kmalloc(ret,GFP_KERNEL);
	if(msg_box->expand[i]==NULL)
	{
		kfree(buffer);
		kfree(expand_data);
		return -ENOMEM;
	}

	msg_box->expand_size[i]=ret;
	memcpy(buffer,&ret,sizeof(int));

	memcpy(msg_box->expand[i],buffer,msg_box->expand_size[i]);
	msg_box->head.expand_num++;
	msg_box->head.expand_size+=ret;	

	kfree(buffer);
	kfree(expand_data);
	return 0;
}

int message_get_box_state(void * message)
{
	struct message_box * msg_box;

	msg_box=(struct message_box *)message;

	if((message==NULL) || IS_ERR(message))
		return -EINVAL;
	return msg_box->box_state;
}

int message_get_blob(void * message,void ** blob)
{
	struct message_box * msg_box;

	msg_box=(struct message_box *)message;

	if((message==NULL) || IS_ERR(message))
		return -EINVAL;
/*
    if((msg_box->box_state !=MSG_BOX_RECOVER)
        && (msg_box->box_state != MSG_BOX_CUT)
        && (msg_box->box_state != MSG_BOX_READ)
        && (msg_box->box_state != MSG_BOX_ADD)
        && (msg_box->box_state != MSG_BOX_EXPAND)
        && (msg_box->box_state != MSG_BOX_DEAL))
		return 0;
*/
    	if(msg_box->blob==NULL)
	{
		return 0;
	}
	*blob=msg_box->blob;
	return msg_box->head.record_size;
}

int message_set_blob(void * message, void * blob, int size)
{
	struct message_box * msg_box;

	msg_box=(struct message_box *)message;

	if((message==NULL) || IS_ERR(message))
		return NULL;
//	if(msg_box->blob!=NULL)
//		free(msg_box->blob);
	msg_box->blob=blob;
	msg_box->head.record_size=size;
	return size;
}


char * message_get_expand_type(void * message,int expand_no)
{
	struct message_box * msg_box;

	msg_box=(struct message_box *)message;

	if((message==NULL) || IS_ERR(message))
		return NULL;
	if(expand_no>=MAX_EXPAND_NUM)
		return NULL;
	if(msg_box->expand[expand_no]==NULL)
		return NULL;
	char * type=kmalloc(5,GFP_KERNEL);
	if(type==NULL)
		return NULL;
	memcpy(type,((MESSAGE_EXPAND *)(msg_box->expand[expand_no]))->tag,4);
	type[4]=0;
	return type;	
}

int message_get_expand(void * message, void ** msg_expand,int expand_no)
{
	struct message_box * msg_box;
	char type[5];
	int i;
	void * addr;
	int retval;
	*msg_expand=NULL;

	msg_box=(struct message_box *)message;

	if((message==NULL) || IS_ERR(message))
		return -EINVAL;
	if(expand_no>=MAX_EXPAND_NUM)
	{
		return 0;
	}
	if((msg_box->expand[expand_no]==NULL) && (msg_box->pexpand[expand_no]==NULL))
		return -EINVAL;

	if(msg_box->pexpand[expand_no]!=NULL)
	{
		*msg_expand = msg_box->pexpand[expand_no];
		return 0;
	}

	memcpy(type,((MESSAGE_EXPAND *)(msg_box->expand[expand_no]))->tag,4);
	type[4]=0;

	void * struct_template=load_record_template(type);
	if(struct_template==NULL)
		return -EINVAL;
	retval=alloc_struct(&addr,struct_template);
	if(retval<=0)
	{
		free_struct_template(struct_template);
		return -EINVAL;
	}
	retval=blob_2_struct(msg_box->expand[expand_no],addr,struct_template);	
	if(retval<0)
	{
		free_struct(addr,struct_template);
		free_struct_template(struct_template);
		return retval;
	}
	msg_box->pexpand[expand_no]=addr;
	*msg_expand=addr;
	
	return 0;	
}

int message_get_expand_byname(void * message, char * name,void ** msg_expand)
{
	struct message_box * msg_box;
	char type[5];
	int i;
	void * addr;
	int retval;
	*msg_expand=NULL;
	MESSAGE_EXPAND * expand_data;

	msg_box=(struct message_box *)message;

	if((message==NULL) || IS_ERR(message))
		return -EINVAL;

	for(i=0;i<MAX_EXPAND_NUM;i++)
	{
		retval=message_get_expand(message,&expand_data,i);
		if(retval<0)
			return retval;
		if(expand_data==NULL)
			return;
		if(strncmp(expand_data->tag,name,4)==0)
		{
			*msg_expand=expand_data;
			break;
		}
	}

	return 0;
}

int message_remove_indexed_expand(void * message, int expand_no,void **expand)
{
	struct message_box * msg_box;
	char type[5];
	int i,j;
	void * addr;
	int retval;
	MESSAGE_EXPAND * expand_data;
	MESSAGE_HEAD * msg_head;

	msg_box=(struct message_box *)message;

	if((message==NULL) || IS_ERR(message))
		return -EINVAL;
	if(expand_no>=MAX_EXPAND_NUM)
		return -EINVAL;
	msg_head=&(msg_box->head);

	*expand=msg_box->pexpand[expand_no];
/*	
	if(msg_box->expand[expand_no]!=NULL)
	{
		free(msg_box->expand[expand_no]);
	}
*/
	for(i=expand_no;i<msg_head->expand_num;i++)
	{
		msg_box->expand[i]=msg_box->expand[i+1];
		msg_box->pexpand[i]=msg_box->pexpand[i+1];
		msg_box->expand_size[i]=msg_box->expand_size[i+1];
	}
	msg_head->expand_num--;
	return 0;

}

int message_remove_expand(void * message, char * name,void ** expand)
{
	struct message_box * msg_box;
	char type[5];
	int i,j;
	void * addr;
	int retval;
	MESSAGE_EXPAND * expand_data;

	msg_box=(struct message_box *)message;

	if((message==NULL) || IS_ERR(message))
		return -EINVAL;
	*expand=NULL;

	for(i=0;i<MAX_EXPAND_NUM;i++)
	{
		retval=message_get_expand(message,&expand_data,i);
		if(retval<0)
			continue;
		if(expand_data==NULL)
			return 0;
		if(strncmp(expand_data->tag,name,4)==0)
		{
			retval=message_remove_indexed_expand(message,i,expand);
			break;
		}
	}

	return 0;
}



