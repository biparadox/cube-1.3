/**
 * File: basefunc.c
 *
 * Created on: Jun 5, 2015
 * Author: Hu Jun (algorist@bjut.edu.cn)
 */

#include "../include/errno.h"
#include "../include/data_type.h"
#include "../include/alloc.h"
#include "../include/memfunc.h"
#include "../include/list.h"
#include "../include/basefunc.h"
#include "../include/attrlist.h"

const int db_order=10;
const int hash_num=1024;  // 2^db_order

const int subdb_order=8;
const int hash_subnum=256;

//#if db_order>8
static inline unsigned int get_hash_index(char * uuid)
{
	int ret = *((unsigned int *)uuid);
	return ret&(0xffff>>(16-db_order));
}
//#elif db_order<=8
//inline unsigned int get_hash_index(char * uuid)
//{
//	return ((unsigned char)uuid[0])>>(8-db_order);
//}
//#endif

//#if subdb_order>8
//inline unsigned int get_hash_subindex(char * uuid)
//{
//	return ((unsigned char)uuid[0])<<(subdb_order-8)+uuid[1]>>(16-subdb_order);
//}
//#elif subdb_order<=8
static inline unsigned int get_hash_subindex(char * uuid)
{
	int ret= *((unsigned char *)uuid);
	return ret &(0xff>>(8-subdb_order));
}
//#endif

typedef struct taguuid_hashlist
{
	int hash_num;
	void * desc;
	Record_List * hash_table;
	int curr_index;
	struct List_head * curr_head;
}UUID_LIST;



void * init_hash_list(int order,int type,int subtype)
{
	int i;
	if(order<0)
		return NULL;
	if(order>10)
		return NULL;
	UUID_LIST * hash_head;
	hash_head=Salloc0(sizeof(UUID_LIST));
	if(hash_head==NULL)
		return NULL;
	hash_head->hash_num=1<<order;
	hash_head->desc=NULL;
	hash_head->curr_index=0;
	hash_head->hash_table=Salloc(sizeof(Record_List)*hash_head->hash_num);

	if(hash_head->hash_table==NULL)
		return NULL;
	for(i=0;i<hash_head->hash_num;i++)
	{
		INIT_LIST_HEAD(&(hash_head->hash_table[i].list));
		hash_head->hash_table[i].record=NULL;
	}
	return hash_head;
}


void * hashlist_get_desc(void * hashlist)
{
	UUID_LIST * uuid_list= (UUID_LIST *)hashlist;
	
	return uuid_list->desc;
}

int hashlist_set_desc(void * hashlist,void * desc)
{
	UUID_LIST * uuid_list= (UUID_LIST *)hashlist;
	uuid_list->desc=desc;
	return 0;
}

int hashlist_add_elem(void * hashlist,void * elem)
{
	Record_List * new_record;
	UUID_LIST * uuid_list= (UUID_LIST *)hashlist;
	int hindex;
	new_record=Calloc(sizeof(Record_List));
	if(new_record==NULL)
		return -ENOMEM;
	INIT_LIST_HEAD(&(new_record->list));
	new_record->record=elem;

	if(uuid_list->hash_num==256)
		hindex=get_hash_subindex(elem);
	else if(uuid_list->hash_num==1024)
		hindex=get_hash_index(elem);
	else
		return -EINVAL;
	
	List_add_tail(&new_record->list,&uuid_list->hash_table[hindex].list);
	return 0;
}

static __inline__ int comp_uuid(void * src,void * desc)
{
	int i;
	unsigned int * src_array=(unsigned int *)src;
	unsigned int * desc_array = (unsigned int *)desc;

	for(i=0;i<DIGEST_SIZE/sizeof(int);i++)
	{
		if(src_array[i]>desc_array[i])
			return 1;
		if(src_array[i]<desc_array[i])
			return -1;
	}
	return 0;
}

static __inline__ int comp_name(void * src,void * desc)
{
	return Strncmp(src,desc,DIGEST_SIZE);
}

Record_List * list_find_nameelem(void * list,char  * name)
{
	Record_List * head = list;
	Record_List * curr = (Record_List *)head->list.next;
	UUID_HEAD * record_head;
	
	while(curr!=head)
	{
		record_head=(UUID_HEAD *)curr->record;
		if(comp_name(record_head->name,name)==0)
			return curr;
		curr= (Record_List *)curr->list.next;
	}
	return NULL;
}

Record_List * list_find_uuidelem(void * list,void * uuid)
{
	Record_List * head = list;
	Record_List * curr = (Record_List *)head->list.next;
	
	while(curr!=head)
	{
		if(comp_uuid(curr->record,uuid)==0)
			return curr;
		curr= (Record_List *)curr->list.next;
	}
	return NULL;
}

Record_List * _hashlist_find_elem(void * hashlist,void * elem)
{
	UUID_LIST * uuid_list= (UUID_LIST *)hashlist;
	int hindex;

	if(uuid_list->hash_num==1<<subdb_order)
		hindex=get_hash_subindex(elem);
	else if(uuid_list->hash_num==1<<db_order)
		hindex=get_hash_index(elem);
	else
		return NULL;
	return list_find_uuidelem(&uuid_list->hash_table[hindex],elem);
}

void * hashlist_find_elem(void * hashlist,void * elem)
{
	Record_List * curr_elem;
	curr_elem=_hashlist_find_elem(hashlist,elem);
	if(curr_elem==NULL)
		return NULL;
	return curr_elem->record;
} 

void * hashlist_find_elem_byname(void * hashlist,char * name)
{
	UUID_LIST * uuid_list= (UUID_LIST *)hashlist;
	int i;
	Record_List * curr_elem;

	for(i=0;i<uuid_list->hash_num;i++)
	{
		curr_elem = list_find_nameelem(&uuid_list->hash_table[i],name);			if(curr_elem!=NULL)
			break;
	}
	if(curr_elem==NULL)
		return NULL;
	return curr_elem->record;
}


Record_List  * _hashlist_remove_elem(void * hashlist,void * elem)
{
	Record_List * curr_elem;
	curr_elem=_hashlist_find_elem(hashlist,elem);
	if(curr_elem==NULL)
		return NULL;
	List_del(&curr_elem->list);
	return curr_elem;		
}

void * hashlist_remove_elem(void * hashlist,void * elem)
{
	Record_List * curr_elem;
	void * temp;
	curr_elem=_hashlist_remove_elem(hashlist,elem);
	if(curr_elem==NULL)
		return curr_elem;
	temp=curr_elem->record;
	Free(curr_elem);
	return temp;	
}

void * hashlist_get_first(void * hashlist)
{
	UUID_LIST * uuid_list = hashlist;
	Record_List * temp=NULL;
	int i;
	for(i=0;i<uuid_list->hash_num;i++)
	{
		temp=(Record_List *)(uuid_list->hash_table[i].list.next);
		if(temp==&uuid_list->hash_table[i])
			continue;
		uuid_list->curr_index=i;
		uuid_list->curr_head=&temp->list;
		return temp->record;
	}
	uuid_list->curr_index=0;
	uuid_list->curr_head=NULL;
	return NULL;
}

void * hashlist_get_next(void * hashlist)
{
	UUID_LIST * uuid_list = hashlist;
	Record_List * temp=NULL;
	int i;
	if(uuid_list->curr_head==NULL)
		return NULL;
	temp=(Record_List *)uuid_list->curr_head;
	for(i=uuid_list->curr_index;i<uuid_list->hash_num;i++)
	{
		temp=(Record_List *)(temp->list.next);
		if(temp==&uuid_list->hash_table[i])
		{
			temp=&uuid_list->hash_table[i+1];
			continue;
		}
		uuid_list->curr_index=i;
		uuid_list->curr_head=&temp->list;
		return temp->record;
	}
	uuid_list->curr_index=0;
	uuid_list->curr_head=NULL;
	return NULL;
}



typedef struct tagpointer_stack
{
	void ** top;
	void ** curr; 
	int size;
}POINTER_STACK;


void * init_pointer_stack(int size)
{
	POINTER_STACK * stack;
	BYTE * buffer;
	buffer=Talloc(sizeof(POINTER_STACK)+sizeof(void *)*size);
	if(buffer==NULL)
		return NULL;
	stack=(POINTER_STACK *)buffer;
	stack->top=(void **)(buffer+sizeof(POINTER_STACK));
	stack->curr=stack->top;
	stack->size=size;
	return stack;
}
void free_pointer_stack(void * stack)
{	
	Free(stack);
	return;
}

int pointer_stack_push(void * pointer_stack,void * pointer)
{
	POINTER_STACK * stack;
	stack=(POINTER_STACK *)pointer_stack;
	if(stack->curr+1>=stack->top+stack->size)
		return -ENOSPC;
	*(stack->curr)=pointer;
	stack->curr++;
	return 0;
}


void * pointer_stack_pop(void * pointer_stack)
{
	POINTER_STACK * stack;
	stack=(POINTER_STACK *)pointer_stack;
	if(--stack->curr<stack->top)
		return NULL;
	return *(stack->curr);
}

typedef struct tagpointer_queue
{
	void ** buffer;
	int size;
	int head;
	int tail;
}POINTER_QUEUE;

void * init_pointer_queue(int size)
{
	POINTER_QUEUE * queue;
	BYTE * buffer;
	buffer=Talloc(sizeof(POINTER_QUEUE)+sizeof(void *)*size);
	if(buffer==NULL)
		return NULL;
	queue=(POINTER_QUEUE *)buffer;
	Memset(queue,0,sizeof(POINTER_QUEUE)+sizeof(void *)*size);
	queue->buffer=(void **)(buffer+sizeof(POINTER_QUEUE));
	queue->size=size;
	queue->head=-1;
	return queue;
}

void free_pointer_queue(void * queue)
{	
	Free(queue);
	return;
}

int pointer_queue_put(void * pointer_queue,void * pointer)
{
	POINTER_QUEUE * queue;
	queue=(POINTER_QUEUE *)pointer_queue;
	if(queue->head==-1)
		queue->head=0;
	else if(queue->head ==queue->size-1)
	{
		if(queue->tail==0)
			return -ENOSPC;
		queue->head=0;
	}
	else
	{
		if(queue->head+1==queue->tail)
			return -ENOSPC;
		queue->head++;
	}
	queue->buffer[queue->head]=pointer;
	return 0;
}


int pointer_queue_get(void * pointer_queue,void **pointer)
{
	POINTER_QUEUE * queue;
	queue=(POINTER_QUEUE *)pointer_queue;
	if(queue->head==-1)
		return -EINVAL;
	*pointer=queue->buffer[queue->tail];
	if(queue->tail==queue->head)
	{
		queue->head=-1;
		queue->tail=0;
		return 0;
	}
	if(queue->tail ==queue->size-1)
		queue->tail=0;
	else
		queue->tail++;
	return 0;
}

typedef struct taglist_queue
{
	Record_List queue_list;
	int record_num;
	Record_List * curr;
}LIST_QUEUE;

void * init_list_queue()
{
	LIST_QUEUE * queue;
	queue=Calloc0(sizeof(LIST_QUEUE));
	if(queue==NULL)
		return NULL;
	INIT_LIST_HEAD(&queue->queue_list.list);
	queue->curr=NULL;
	return queue;
}

void free_list_queue(void * queue)
{	
	
	Free(queue);
	return;
}

int list_queue_put(void * list_queue,void * record)
{
	LIST_QUEUE * queue;
	Record_List * slot;
	queue=(LIST_QUEUE *)list_queue;
	
	slot=Calloc0(sizeof(Record_List));
	if(slot==NULL)
		return -EINVAL;
	INIT_LIST_HEAD(&slot->list);
	slot->record=record;
	List_add_tail(&slot->list,&queue->queue_list.list);
	queue->record_num++;
	return queue->record_num;
}
	

int list_queue_get(void * list_queue,void ** record)
{
	LIST_QUEUE * queue;
	Record_List * slot;
	queue=(LIST_QUEUE *)list_queue;
	
	if(queue->record_num==0)
	{
		*record=NULL;
		return 0;
	}
	

	slot=(Record_List *) (queue->queue_list.list.next);

	*record=slot->record;

	queue->record_num--;
	
	List_del(&slot->list);
	Free(slot);
	return queue->record_num;
}

void * list_queue_getfirst(void * list_queue)
{
	LIST_QUEUE * queue;
	Record_List * slot;
	queue=(LIST_QUEUE *)list_queue;
	
	if(queue->record_num==0)
	{
		return NULL;
	}
	
	slot = (Record_List *)queue->queue_list.list.next;
	queue->curr=slot;
	return slot->record;
}

void * list_queue_getnext(void * list_queue)
{
	LIST_QUEUE * queue;
	Record_List * slot;
	queue=(LIST_QUEUE *)list_queue;
	
	if(queue->record_num==0)
	{
		return NULL;
	}
	slot=(Record_List *)queue->curr;
	if(slot==NULL)
		return NULL;

	slot=(Record_List *)slot->list.next;
	if(slot==&queue->queue_list)
	{
		queue->curr=NULL;
		return NULL;
	}
	queue->curr=slot;
	return slot->record;
}

void * list_queue_removecurr(void * list_queue)
{
	LIST_QUEUE * queue;
	Record_List * slot;
	queue=(LIST_QUEUE *)list_queue;
	void * record;
	
	if(queue->record_num==0)
	{
		return NULL;
	}
	slot=(Record_List *)queue->curr;
	if(slot==NULL)
		return NULL;

	record=slot->record;

	if(slot->list.next==&queue->queue_list.list)
	{
		queue->curr=NULL;
	}
	queue->curr=(Record_List *)slot->list.next;
	Free(slot);
	return record;
}

