#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../include/data_type.h"
#include "../include/list.h"
#include "../include/attrlist.h"
#include "../include/alloc.h"
#include "../include/string.h"
#include "../include/string.h"
#include "../include/json.h"
#include "../include/struct_deal.h"
#include "../include/basefunc.h"
#include "../include/memdb.h"
#include "../include/message.h"
#include "../include/channel.h"


static struct tagchannel_list
{
	Record_List head;
	int state;
	int channel_num;
	void * curr;
} channel_list;


void  channel_init( )
{
        int ret;
        Memset(&channel_list,0,sizeof(channel_list));
        INIT_LIST_HEAD(&(channel_list.head.list));
        channel_list.head.record=NULL;
        channel_list.channel_num=0;
        return ;
}


void * channel_create(char * name,int type)
{
	int ret;
	CHANNEL * new_channel;
	if(type<=0)
		return NULL;
	if(type>CHANNEL_SHUTDOWN)
		return NULL;
	new_channel=Salloc0(sizeof(CHANNEL));
	if(new_channel==NULL)
		return NULL;
	Strncpy(new_channel->name,name,DIGEST_SIZE);
	new_channel->state=CHANNEL_INIT;
	new_channel->type=type;

	if(type&CHANNEL_FIXMEM)
		return -EINVAL;
	if(type&CHANNEL_READ)
	{
		new_channel->read_buf=channel_buf_create(CHANNEL_BUF_SIZE);
		if(new_channel->read_buf==NULL)
		{
			Free(new_channel);
			return NULL;
		}
	}
	if(type&CHANNEL_WRITE)
	{
		new_channel->write_buf=channel_buf_create(CHANNEL_BUF_SIZE);
		if(new_channel->write_buf==NULL)
		{
			if(new_channel->read_buf!=NULL)
				channel_buf_free(new_channel->read_buf);
			Free(new_channel);
			return NULL;
		}
	}
	return new_channel;
}

void * channel_create_fixmem(char * name,int type,int size,void * readaddr,void * writeaddr)
{
	int ret;
	CHANNEL * new_channel;
	if(type<=0)
		return NULL;
	if(type>CHANNEL_SHUTDOWN)
		return NULL;
	new_channel=Salloc0(sizeof(CHANNEL));
	if(new_channel==NULL)
		return NULL;
	Strncpy(new_channel->name,name,DIGEST_SIZE);
	new_channel->state=CHANNEL_INIT;
	new_channel->type=type;

	if(!(type&CHANNEL_FIXMEM))
		return -EINVAL;
	if(type&CHANNEL_READ)
	{
		new_channel->read_buf=channel_membuf_create(size,readaddr);
		if(new_channel->read_buf==NULL)
		{
			Free(new_channel);
			return NULL;
		}
	}
	if(type&CHANNEL_WRITE)
	{
		new_channel->write_buf=channel_membuf_create(size,writeaddr);
		if(new_channel->write_buf==NULL)
		{
			if(new_channel->read_buf!=NULL)
				channel_membuf_free(new_channel->read_buf);
			Free(new_channel);
			return NULL;
		}
	}
	return new_channel;
}

void * channel_register(char * name,int type,void * module)
{
	CHANNEL * new_channel;
	new_channel=channel_create(name,type);
	if(new_channel==NULL)
		return NULL;
	Record_List * channel_head=Dalloc0(sizeof(*channel_head),module);
	if(channel_head==NULL)
		return NULL;
			
        INIT_LIST_HEAD(&(channel_list.head.list));

	channel_head->record=new_channel;
	List_add_tail(&channel_head->list,&(channel_list.head.list));
	
	return new_channel;
}

void * channel_register_fixmem(char * name,int type,void * module,int size,void * readaddr,void * writeaddr)
{
	CHANNEL * new_channel;
	if(!(type&CHANNEL_FIXMEM))
		return -EINVAL;
	new_channel=channel_create_fixmem(name,type,size,readaddr,writeaddr);
	if(new_channel==NULL)
		return NULL;
	Record_List * channel_head=Dalloc0(sizeof(*channel_head),module);
	if(channel_head==NULL)
		return NULL;
			
        INIT_LIST_HEAD(&(channel_list.head.list));

	channel_head->record=new_channel;
	List_add_tail(&channel_head->list,&(channel_list.head.list));
	
	return new_channel;
}

int _channel_comp_name(void * List_head, void * name)
{
        struct List_head * head;
        CHANNEL * comp_channel;
        head=(struct List_head *)List_head;
        if(head==NULL)
                return -EINVAL;
        Record_List * record;
        char * string;
        string=(char *)name;
        record = List_entry(head,Record_List,list);
        comp_channel = (CHANNEL *) record->record;
        if(comp_channel == NULL)
                return -EINVAL;
        if(comp_channel->name==NULL)
                return -EINVAL;
        return Strncmp(comp_channel->name,string,DIGEST_SIZE);
}

void * channel_find(char * name)
{
	struct List_head * curr_head;
	Record_List * record_elem;
	curr_head = find_elem_with_tag(&channel_list.head.list,
		_channel_comp_name,name);
	if(curr_head==NULL)
		return NULL;
        record_elem=List_entry(curr_head,Record_List,list);
	return record_elem->record;	
}

void channel_free(void * channel)
{
	CHANNEL * old_channel=channel;
	if(old_channel->read_buf!=NULL)
		channel_buf_free(old_channel->read_buf);
	if(old_channel->write_buf!=NULL)
		channel_buf_free(old_channel->write_buf);
	Free(old_channel);
}

int channel_read(void * channel,BYTE * data,int size)
{
	
	CHANNEL * curr_channel=channel;
	if(!(curr_channel->type & CHANNEL_READ))
		return -EINVAL;
	return channel_buf_read(curr_channel->read_buf,data,size);
}

int channel_write(void * channel,BYTE * data,int size)
{
	
	CHANNEL * curr_channel=channel;
	if(!(curr_channel->type & CHANNEL_WRITE))
		return -EINVAL;
	return channel_buf_write(curr_channel->write_buf,data,size);
}

int channel_inner_read(void * channel,BYTE * data,int size)
{
	
	CHANNEL * curr_channel=channel;
	if(!(curr_channel->type & CHANNEL_WRITE))
		return -EINVAL;
	return channel_buf_read(curr_channel->write_buf,data,size);
}

int channel_inner_write(void * channel,BYTE * data,int size)
{
	
	CHANNEL * curr_channel=channel;
	if(!(curr_channel->type & CHANNEL_READ))
		return -EINVAL;
	return channel_buf_write(curr_channel->read_buf,data,size);
}
