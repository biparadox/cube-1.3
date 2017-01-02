/*************************************************
*       project:        973 trust demo, zhongan secure os 
*                       and trust standard verify
*	name:		data_type.h
*	write date:    	2011-08-04
*	auther:    	Hu jun
*       content:        this file describe some basic data type 
*       changelog:       
*************************************************/
#ifndef _OS210_KERNEL_COMP_H
#define _OS210_KERNEL_COMP_H
/*
struct list_head
{
 struct list_head *next;
 struct list_head *prev;
};
*/
#define GFP_KERNEL 0
#define GFP_ATOMIC 1
#define GFP_USER 2

static __inline__ void * kmalloc(int len, int type)
{
    return malloc(len);
}
static __inline__ void kfree( void * pointer)
{
	free(pointer);
}
#define printk printf
#define PAGE_SIZE 4096

struct page 
{
	void * addr;
	int index;
	void * beginaddr;
};

static inline void * alloc_page(int gfp_mask) 
{
	struct page * page;
	int addr;
	page =malloc(sizeof(struct page));
	if(page == NULL)
		return NULL;
	page->beginaddr = malloc(PAGE_SIZE*2);
	if(page->beginaddr == NULL)
	{
		free(page);
		return NULL;
	}
	addr = (int)(page->beginaddr);
        addr &= 0xFFFFF000;
        addr+=PAGE_SIZE;
	page->addr = addr;
	return page;	
}

static inline void free_page(struct page * page) 
{
	if(page != NULL)	
	free(page->beginaddr);
	free(page);
	return ;	
}

#define  kmap(page) (page->addr)

#endif
