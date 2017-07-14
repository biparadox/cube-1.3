/*
 *  * Copyright (C) 2003 xh-sos v0.1.0
 *    install.h
 *    */
#ifndef _LIST_H
#define _LIST_H       

struct List_head
{
 struct List_head *next;
 struct List_head *prev;
};

#define INIT_LIST_HEAD(ptr) do { \
	        (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

#define List_entry(ptr,type,member) \
	((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))
		
static __inline__ void __List_add(struct List_head * new,
				struct List_head * prev,
				struct List_head * next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static __inline__ void List_cat(struct List_head * newlist,
				struct List_head * prev)
{
	struct List_head * newtail;
	newtail=newlist->prev;
	
	newlist->prev=prev;
	newtail->next=prev->next;
	
	prev->next->prev=newtail;
	prev->next=newlist;
}

/**
 * List_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static __inline__ void List_add(struct List_head *new, struct List_head *head)
{
	__List_add(new, head, head->next);
}

/**
 *  * List_add_tail - add a new entry
 *   * @new: new entry to be added
 *    * @head: list head to add it before
 *     *
 *      * Insert a new entry before the specified head.
 *       * This is useful for implementing queues.
 *        */
static __inline__ void List_add_tail(struct List_head *new, struct List_head *head)
{
	        __List_add(new, head->prev, head);
}
static __inline__ void List_del(struct List_head * head)
{
		if(head->next==head)
				return;
		head->next->prev=head->prev;
		head->prev->next=head->next;
		head->prev=head;
		head->next=head;
		return;

}
#endif
