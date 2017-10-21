#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../include/data_type.h"
#include "../include/list.h"
#include "../include/string.h"
#include "../include/alloc.h"
#include "../include/json.h"
#include "../include/struct_deal.h"
#include "../include/basefunc.h"
#include "../include/memdb.h"
#include "../include/message.h"
#include "../include/channel.h"

#define CHANNEL_BUF_SIZE 4000

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
