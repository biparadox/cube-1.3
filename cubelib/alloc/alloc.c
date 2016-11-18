#include <stdlib.h>
#include <errno.h>

#include "../include/data_type.h"
#include "../include/string.h"

struct alloc_struct
{
	int Tmem_size;
	int Gmem_size;
	int Cmem_size;	
};

void * Talloc(int size)
{
	return malloc(size);
}

void * Talloc0(int size)
{
	char * mem;
	mem=malloc(size);
	if(mem!=NULL)
	{
		Memset(mem,0,size);
	}
	return mem;
}

void * Calloc(int size)
{
	return malloc(size);
}

void * Calloc0(int size)
{
	char * mem;
	mem=malloc(size);
	if(mem!=NULL)
	{
		Memset(mem,0,size);
	}
	return mem;
}

int Galloc(void ** pointer,int size)
{
	*pointer= malloc(size);
	if(*pointer==NULL)
		return -ENOMEM;
	return size;
}

int Galloc0(void ** pointer,int size)
{
	*pointer= malloc(size);
	if(*pointer==NULL)
		return -ENOMEM;
	Memset(*pointer,0,size);
	return size;
}

int Palloc(void ** pointer,int size)
{
	*pointer= malloc(size);
	if(*pointer==NULL)
		return -ENOMEM;
	return size;
}

int Palloc0(void ** pointer,int size)
{
	*pointer= malloc(size);
	if(*pointer==NULL)
		return -ENOMEM;
	Memset(*pointer,0,size);
	return size;
}

int Free(void * pointer)
{
	free(pointer);
	return 0;
}
int Free0(void * pointer)
{
	free(pointer);
	return 0;
}
