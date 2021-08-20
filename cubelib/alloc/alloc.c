#include <stdlib.h>
#include <errno.h>


#include "../include/data_type.h"
#include "../include/memfunc.h"

#include "alloc_struct.h"

struct alloc_struct tempmem_struct;
struct alloc_struct static_struct;
struct alloc_struct cache_struct[10];
struct alloc_struct dynamic_struct;

int alloc_init()
{
	Memset(&tempmem_struct,0,sizeof(tempmem_struct));	
	Memset(&static_struct,0,sizeof(static_struct));	
	Memset(cache_struct,0,sizeof(cache_struct));	
	Memset(&dynamic_struct,0,sizeof(dynamic_struct));	
	return 0;
}

void * general_alloc( int type, int flag,struct alloc_struct * mem_struct,int size)
{

	int realsize = size+sizeof(POINTER_LIST);
	POINTER_LIST * mem_list=malloc(realsize);
	POINTER_HEAD * mem_head=&(mem_list->pointer);
	if(mem_head==NULL)
		return NULL;
	mem_head->type=type;
	mem_head->flag=flag;
	mem_head->size=size;
	mem_list->next=NULL;
	mem_struct->total_size+=size;
	mem_struct->occupy_size+=realsize;
	mem_struct->pointer_no++;
	if(mem_struct->tail==NULL)
	{
		mem_struct->pointer_list.next=mem_list;
		mem_struct->tail=mem_list;
	}
	else
	{
		mem_struct->tail->next=mem_list;
		mem_struct->tail=mem_list;	
	}
	return ((void *)mem_list) + sizeof(*mem_list);
}
	
int alloc_pointer_type(void * pointer)
{
	POINTER_HEAD * mem_head=(POINTER_HEAD *)(pointer-sizeof(POINTER_LIST));
	return mem_head->type;
}

int general_free(void * pointer,struct alloc_struct * mem_struct)
{

	POINTER_LIST * mem_list=(POINTER_LIST *)(pointer-sizeof(*mem_list));
	POINTER_HEAD * mem_head=&(mem_list->pointer);
	mem_struct->total_size-=mem_head->size;
	mem_struct->occupy_size-=mem_head->size+sizeof(*mem_list);
	mem_struct->pointer_no--;


	if(mem_list==mem_struct->pointer_list.next)
	{
		mem_struct->pointer_list.next=mem_list;	
		if(mem_list==mem_struct->tail)
			mem_struct->tail=NULL;	
	}
	else
	{
		POINTER_LIST * priv_list=mem_struct->pointer_list.next;
		do
		{
			if(priv_list->next==NULL)
				return -EINVAL;
			if(priv_list->next==mem_list)
			{
				priv_list->next=mem_list->next;
				if(mem_list==mem_struct->tail)
					mem_struct->tail=priv_list;	
				break;
			}
			priv_list=priv_list->next;
		}while(1);
	}

	free(mem_list);
	return 0;
}

void * general_alloc0( int type, int flag,struct alloc_struct * mem_struct,int size)
{
	void * pointer=general_alloc(type,flag,mem_struct,size);
	if(pointer!=NULL)
	{
		Memset(pointer,0,size);
	}
	return pointer; 
}

void * Talloc(int size)
{

    void * pointer;
    pointer=malloc(size+sizeof(UINT16));
    if(pointer==NULL)
        return NULL;
    *(UINT16 *)pointer=size;
    *(UINT16 *)pointer|=ALLOC_TEMP<<12;

    return pointer+sizeof(UINT16);   

//	return general_alloc(ALLOC_TEMP,0,&tempmem_struct,size);
}

void * Talloc0(int size)
{
	void * temp=Talloc(size);
	if(temp==NULL)
		return NULL;
	Memset(temp,0,size);
	return temp;
//	return general_alloc0(ALLOC_TEMP,0,&tempmem_struct,size);
}

int TFree(void * pointer)
{
	free(pointer-sizeof(UINT16));
	return 0;
//	return general_free(pointer,&tempmem_struct);
}

void * Salloc(int size)
{
    void * pointer;
    pointer=malloc(size+sizeof(UINT16));
    if(pointer==NULL)
    *(UINT16 *)pointer=size;
    *(UINT16 *)pointer|=ALLOC_STATIC<<12;

    return pointer+sizeof(UINT16);   
//	return general_alloc(ALLOC_STATIC,0,&static_struct,size);
}

void * Salloc0(int size)
{
	void * temp=Salloc(size);
	if(temp==NULL)
		return NULL;
	memset(temp,0,size);
	return temp;
//	return general_alloc0(ALLOC_STATIC,0,&static_struct,size);
}

int SFree(void * pointer)
{
	free(pointer-sizeof(UINT16));
	return 0;
}


void * Dalloc(int size,void * base)
{
    void * pointer;
    pointer=malloc(size+sizeof(UINT16)+sizeof(void *));
    if(pointer==NULL)
        return NULL;
    *(void **)pointer=base;
    *(UINT16 *)(pointer+sizeof(void *))=size;
    *(UINT16 *)(pointer+sizeof(void *))|=ALLOC_DYNAMIC<<12;

    return pointer+sizeof(UINT16)+sizeof(void *);   
}

void * Dalloc0(int size,void * base)
{
    
	void * temp=Dalloc(size,base);
	if(temp==NULL)
		return NULL;
	memset(temp,0,size);
	return temp;
}

void * Dpointer_set(void * pointer,void * base)
{
    if(base!=NULL)
        *(void **)base=pointer;

    *(void **)(pointer-sizeof(UINT16)-sizeof(void *))=base;
    return pointer; 
}

int DFree(void * pointer)
{
	free(pointer-sizeof(UINT16)-sizeof(void *));
	return 0;
}

void * Calloc(int size)
{
    void * pointer;
    pointer=malloc(size+sizeof(UINT16));
    if(pointer==NULL)
        return NULL;
    *(UINT16 *)pointer=size;
    *(UINT16 *)pointer|=ALLOC_CACHE<<12;

    return pointer+sizeof(UINT16);   
/*
*/
}

void * Calloc0(int size)
{
	void * temp=Calloc(size);
	if(temp==NULL)
		return NULL;
	memset(temp,0,size);
	return temp;
}

int CFree(void * pointer)
{
	free(pointer-sizeof(UINT16));
	return 0;
}

void * Palloc(int size,void * base)
{
	switch(	Pointer_Type(base))
	{
		case ALLOC_TEMP:
			return Talloc(size);
		case ALLOC_STATIC:
			return Salloc(size);
		case ALLOC_DYNAMIC:
			return Dalloc(size,base);
		case ALLOC_CACHE:
			return Dalloc(size,base);
		default:
			return NULL;
	}	
}

void * Palloc0(int size,void * base)
{
	void * mem=Palloc(size,base);
	if(mem!=NULL)
		Memset(mem,0,size);
	return mem;
}

int Free(void * pointer)
{
	switch(	Pointer_Type(pointer))
	{
		case ALLOC_TEMP:
			return TFree(pointer);
		case ALLOC_STATIC:
			return -EINVAL;
		case ALLOC_DYNAMIC:
			return DFree(pointer);
		case ALLOC_CACHE:
			return CFree(pointer);
		default:
			return -EINVAL;
	}	
	return 0;
/*
	struct alloc_head * mem_head=(struct alloc_head *)(pointer-sizeof(POINTER_LIST));
	switch(mem_head->type)
	{
		case ALLOC_TEMP:
			return TFree(pointer);
		case ALLOC_STATIC:
			return SFree(pointer);
		case ALLOC_DYNAMIC:
			return DFree(pointer);
		case ALLOC_CACHE:
			return CFree(pointer,mem_head->flag);
		default:
			return -EINVAL;
	}	
*/
}

int Free0(void * pointer)
{
    int size;
    size=Pointer_Size(pointer);
    Memset(pointer,0,size);
	Free(pointer);
	return 0;
/*
	struct alloc_head * mem_head=(struct alloc_head *)(pointer-sizeof(POINTER_LIST));
	Memset(pointer,0,mem_head->size);	
	return Free(pointer);
*/
}
int TmemReset()
{
	POINTER_LIST * priv_list=tempmem_struct.pointer_list.next;
	while(priv_list!=NULL)
	{
		POINTER_LIST * curr_list=priv_list;
		priv_list=priv_list->next;
	
		POINTER_HEAD * mem_head=&(curr_list->pointer);
		tempmem_struct.total_size-=mem_head->size;
		tempmem_struct.occupy_size-=mem_head->size+sizeof(POINTER_LIST);
		tempmem_struct.pointer_no--;
		free(curr_list);
	}
	tempmem_struct.pointer_list.next=NULL;
	tempmem_struct.tail=NULL;
	return 0;		
}
