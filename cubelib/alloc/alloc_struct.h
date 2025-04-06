#ifndef ALLOC_STRUCT_H
#define ALLOC_STRUCT_H

enum alloc_type
{
	ALLOC_TEMP=1,
	ALLOC_STATIC,
	ALLOC_CACHE,
	ALLOC_DYNAMIC,
    ALLOC_PAGE,
    ALLOC_ERR
};

#define Pointer_Type_Mask  0xF0000000
#define Pointer_Size_Mask  0x0FFFFFFF
#define Pointer_Size_bits  28
#define Pointer_Desc(pointer)   (*(UINT32 *)((BYTE *)pointer -4))
#define Pointer_Type(pointer)   ((Pointer_Desc(pointer)&Pointer_Type_Mask)>>Pointer_Size_bits)
#define Pointer_Size(pointer)   (Pointer_Desc(pointer)&Pointer_Size_Mask)

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
