#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>

#include "data_type.h"
#include "../include/kernel_comp.h"
#include "list.h"
#include "attrlist.h"
#include "string.h"
#include "struct_deal.h"
#include "basefunc.h"
#include "memdb.h"
#include "message.h"

typedef struct message_queue//审计信息结构体
{	
	Record_List head;
	pthread_rwlock_t rwlock;
	int state;
	int msg_num;
}MESSAGE_QUEUE;

int message_queue_init(void ** msg_queue)
{
	int ret;
	MESSAGE_QUEUE * queue;
	queue=(MESSAGE_QUEUE *)malloc(sizeof(MESSAGE_QUEUE));
	if(queue==NULL)
		return -ENOMEM;
	Memset(queue,0,sizeof(MESSAGE_QUEUE));
	INIT_LIST_HEAD(&(queue->head.list));
	queue->head.record=NULL;
	ret=pthread_rwlock_init(&(queue->rwlock),NULL);
	if(ret<0)
	{
		free(queue);
		return -EINVAL;
	}
	*msg_queue=queue;
	return 0;
}

int __message_queue_putmsg(void * msg_queue,void * msg)
{
	Record_List * recordhead;
	Record_List * newrecord;
	MESSAGE_QUEUE * queue =(MESSAGE_QUEUE *)msg_queue;

	recordhead = &(queue->head);
	if(recordhead==NULL)
		return -ENOMEM;

	newrecord = malloc(sizeof(Record_List));
	if(newrecord==NULL)
		return -ENOMEM;
	INIT_LIST_HEAD(&(newrecord->list));
	newrecord->record=msg;
	pthread_rwlock_wrlock(&(queue->rwlock));
	queue->msg_num++;
	List_add_tail(&(newrecord->list),recordhead);
	pthread_rwlock_unlock(&(queue->rwlock));
	return 0;
}

int message_queue_putmsg(void * msg_queue,void * msg)
{
	Record_List * recordhead;
	Record_List * newrecord;
	MESSAGE_QUEUE * queue =(MESSAGE_QUEUE *)msg_queue;

	if(queue->state<0)
		return -EINVAL;
	return __message_queue_putmsg(msg_queue,msg);
}

int __message_queue_getmsg(void * msg_queue,void ** msg)
{
	Record_List * recordhead;
	Record_List * newrecord;
	MESSAGE_QUEUE * queue =(MESSAGE_QUEUE *)msg_queue;

	*msg=NULL;
	recordhead = &(queue->head);
	if(recordhead==NULL)
		return -ENOMEM;

	pthread_rwlock_rdlock(&(queue->rwlock));
	if(queue->msg_num==0)
	{
		pthread_rwlock_unlock(&(queue->rwlock));
		return 0;
	}
	newrecord = (Record_List *)(recordhead->list.prev);
	queue->msg_num--;
	List_del(&(newrecord->list));
	pthread_rwlock_unlock(&(queue->rwlock));
	*msg=newrecord->record;
	free(newrecord);
	return 0;
}

int message_queue_getmsg(void * msg_queue,void ** msg)
{
	Record_List * recordhead;
	Record_List * newrecord;
	MESSAGE_QUEUE * queue =(MESSAGE_QUEUE *)msg_queue;

	if(queue->state<0)
		return -EINVAL;
	return __message_queue_getmsg(msg_queue,msg);
}

int message_queue_reset(void * msg_queue)
{
	int ret;
	void * msg;
	MESSAGE_QUEUE * queue =(MESSAGE_QUEUE *)msg_queue;
	if(queue->state<0)
		return -EINVAL;

	pthread_rwlock_wrlock(&(queue->rwlock));
	queue->state = -1;
	pthread_rwlock_unlock(&(queue->rwlock));

	while(queue->msg_num>0)
	{
		ret=message_queue_getmsg(msg_queue,&msg);
		message_free(msg);
		if(ret<=0)
			break;
	}
	return 0;
}


int message_queue_destroy(void * msg_queue)
{
	int ret;
	MESSAGE_QUEUE * queue=(MESSAGE_QUEUE *)msg_queue;
	message_queue_reset(queue);
	ret=pthread_rwlock_destroy(&(queue->rwlock));
	if(ret<0)
		return ret;
	free(queue);
	return 0;
}


