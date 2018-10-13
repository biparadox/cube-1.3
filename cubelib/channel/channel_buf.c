#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../include/data_type.h"
#include "../include/list.h"
#include "../include/memfunc.h"
#include "../include/alloc.h"
#include "../include/json.h"
#include "../include/struct_deal.h"
#include "../include/basefunc.h"
#include "../include/memdb.h"
#include "../include/message.h"
#include "../include/channel.h"


typedef struct cube_channel_buf
{
	UINT16 bufsize;
	UINT16 offset;
	UINT16 last_read_tail;
	UINT16 last_write_tail;
}__attribute__((packed)) CHANNEL_BUF; 


void * channel_buf_create(int size)
{
	int ret;
	CHANNEL_BUF * new_buf;
	if(size<0)
		return NULL;
	if(size>32767)
		return NULL;

	new_buf=Dalloc0(size+sizeof(CHANNEL_BUF),NULL);
	if(new_buf==NULL)
		return NULL;
	new_buf->bufsize=size;		
	new_buf->offset=sizeof(CHANNEL_BUF);
	return new_buf;
}

void * channel_membuf_create(int size,void * buf)
{
	int ret;
	CHANNEL_BUF * new_buf;
	if(size<sizeof(CHANNEL_BUF))
		return NULL;
	if(size>32767)
		return NULL;

	new_buf=(CHANNEL_BUF *)buf;
	if(new_buf==NULL)
		return NULL;
	new_buf->bufsize=size-sizeof(new_buf);		
	new_buf->offset=sizeof(CHANNEL_BUF);

	return new_buf;
}

void channel_buf_free(void * buf)
{
	CHANNEL_BUF * old_buf=buf;
	Memset(buf,0,old_buf->bufsize);
	Free(old_buf);	
}

void channel_membuf_free(void * buf)
{
	CHANNEL_BUF * old_buf=buf;
	Free(old_buf);	
}

int channel_buf_read(void * buf,BYTE * data,int size)
{
	CHANNEL_BUF * curr_buf=buf;
	int left_data;
	int ret=0;
	if(curr_buf->last_read_tail==curr_buf->last_write_tail)
		return 0;

	if(curr_buf->last_read_tail<curr_buf->last_write_tail)
	{
		left_data=curr_buf->last_write_tail-curr_buf->last_read_tail;
		if(size<left_data)
		{
			Memcpy(data,((void *)curr_buf)+curr_buf->offset+curr_buf->last_read_tail,size);
			curr_buf->last_read_tail+=size;		
			ret=size;
		}
		else
		{
			Memcpy(data,((void *)curr_buf)+curr_buf->offset+curr_buf->last_read_tail,left_data);
			curr_buf->last_read_tail=curr_buf->last_write_tail;		
			ret=left_data;
		}

	}
	else
	{
		left_data=curr_buf->bufsize-curr_buf->last_read_tail+curr_buf->last_write_tail;
		int tail_block_size=curr_buf->bufsize-curr_buf->last_read_tail;
		if(size<tail_block_size)
		{
			Memcpy(data,((void *)curr_buf)+curr_buf->offset+curr_buf->last_read_tail,size);
			curr_buf->last_read_tail+=size;		
			ret=size;
		}
		else
		{
			Memcpy(data,((void *)curr_buf)+curr_buf->offset+curr_buf->last_read_tail,tail_block_size);
			if(size<left_data)
			{
				Memcpy(data+tail_block_size,((void *)curr_buf)+curr_buf->offset,size-tail_block_size);
				curr_buf->last_read_tail=size-tail_block_size;
				ret=size;	

			}
			else
			{
				Memcpy(data+tail_block_size,((void *)curr_buf)+curr_buf->offset,left_data-tail_block_size);
				curr_buf->last_read_tail=curr_buf->last_write_tail;		
				ret=left_data;	

			}
		}
	}
	return ret;	
}
 
int channel_buf_write(void * buf,BYTE * data,int size)
{
	CHANNEL_BUF * curr_buf=buf;
	int left_data;
	int ret=0;
	if(curr_buf->last_read_tail-1==curr_buf->last_write_tail)
		return 0;

	if(curr_buf->last_read_tail>curr_buf->last_write_tail)
	{
		left_data=curr_buf->last_read_tail-1-curr_buf->last_write_tail;
		if(size<left_data)
		{
			Memcpy(((void *)curr_buf)+curr_buf->offset+curr_buf->last_write_tail,data,size);
			curr_buf->last_write_tail+=size;		
			ret=size;
		}
		else
		{
			Memcpy(((void *)curr_buf)+curr_buf->offset+curr_buf->last_write_tail,data,left_data);
			curr_buf->last_write_tail=curr_buf->last_read_tail-1;		
			ret=left_data;
		}

	}
	else
	{
		left_data=curr_buf->bufsize-curr_buf->last_write_tail+curr_buf->last_read_tail-1;
		int tail_block_size=curr_buf->bufsize-curr_buf->last_write_tail;
		if(size<tail_block_size)
		{
			Memcpy(((void *)curr_buf)+curr_buf->offset+curr_buf->last_write_tail,data,size);
			curr_buf->last_write_tail+=size;		
			ret=size;
		}
		else
		{
			Memcpy(((void *)curr_buf)+curr_buf->offset+curr_buf->last_write_tail,data,tail_block_size);
			if(size<left_data)
			{
				Memcpy(((void *)curr_buf)+curr_buf->offset,data+tail_block_size,size-tail_block_size);
				curr_buf->last_write_tail=size-tail_block_size;
				ret=size;	

			}
			else
			{
				Memcpy(((void *)curr_buf)+curr_buf->offset,data+tail_block_size,left_data-tail_block_size);
				curr_buf->last_write_tail=curr_buf->last_read_tail-1;		
				ret=left_data;	

			}
		}
	}
	return ret;	
}
 
