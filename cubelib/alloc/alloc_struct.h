#ifndef ALLOC_STRUCT_H
#define ALLOC_STRUCT_H

enum alloc_type
{
	ALLOC_TEMP=1,
	ALLOC_STATIC,
	ALLOC_CACHE,
	ALLOC_DYNAMIC	
};

typedef struct alloc_head
{
	BYTE type;
	BYTE flag;
	UINT16 size;
}__attribute__((packed)) POINTER_HEAD;

typedef struct pointer_list
{
	struct alloc_head pointer;
	struct pointer_list * next;
}POINTER_LIST;

struct alloc_struct
{
	int total_size;
	int occupy_size;
	int pointer_no;
	int unit_size;
	POINTER_LIST pointer_list;
	POINTER_LIST * tail;
};


#endif
