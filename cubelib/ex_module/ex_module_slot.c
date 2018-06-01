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
//#include "kernel_comp.h"
#include "list.h"
#include "attrlist.h"
#include "string.h"
#include "struct_deal.h"
#include "basefunc.h"
#include "memdb.h"
#include "message.h"

enum entity_type
{
	ENTITY_RECORD=1,
	ENTITY_MESSAGE
};



struct entity_slot_list
{
	struct List_head head;
	int   slot_num;
	Record_List * curr_slot;
};

typedef struct tagslot_pin
{
	int pin_num;
	enum entity_type entity_type;
	int type;
	int subtype;
}SLOT_PIN;

typedef struct tagslot_port
{
	BYTE name[DIGEST_SIZE];
	char slot_size;
	SLOT_PIN * pins;
}SLOT_PORT;


typedef struct tagslot_sock
{
	BYTE uuid[DIGEST_SIZE];
	SLOT_PORT * slot_port; 
	int entity_num;
	long long slot_mask;
	long long slot_flags;
	void ** entitys;
}SLOT_SOCK;

void * slot_list_init( )
{
	struct entity_slot_list * slot_list;
	slot_list=(struct entity_slot_list *)malloc(sizeof(*slot_list));
	if(slot_list==NULL)
		return NULL;
	Memset(slot_list,0,sizeof(*slot_list));
	INIT_LIST_HEAD(&(slot_list->head));
	return slot_list;
}

void * slot_port_init(char * name,int pin_num)
{
	SLOT_PORT * slot_port; 
	if(pin_num<0)
		return NULL;
	if(pin_num>64)
		return NULL;
	slot_port=(SLOT_PORT *)malloc(sizeof(*slot_port));
	if(slot_port==NULL)
		return NULL;
	Memset(slot_port,0,sizeof(*slot_port));
	Strncpy(slot_port->name,name,DIGEST_SIZE);
	slot_port->slot_size=(char)pin_num;
	if(pin_num>0)
	{
		slot_port->pins=(SLOT_PIN *)malloc(sizeof(SLOT_PIN)*pin_num);
		if(slot_port->pins==NULL)
		{
			free(slot_port);
			return NULL;
		}
		Memset(slot_port->pins,0,sizeof(SLOT_PIN)*pin_num);
	}
	return slot_port;
}

void slot_port_free(void * slot_port)
{
	SLOT_PORT * ports=slot_port;
	if(ports->pins!=NULL)
	{
		free(ports->pins);
	}
	free(slot_port);		
	return;
}

int slot_port_addpin(void * port,enum entity_type entity_type, int type,int subtype)
{
	SLOT_PORT * slot_port=port;
	SLOT_PIN * pin;
	if(slot_port==NULL)	
		return -EINVAL;
	if(slot_port->slot_size>64)
		return -EINVAL;

	pin=&slot_port->pins[slot_port->slot_size-1];

	if((slot_port->slot_size==0)||(pin->entity_type!=0))
	{
		if(slot_port->slot_size>=64)
			return -EINVAL;
		SLOT_PIN * new_pins=(SLOT_PIN *)malloc(sizeof(SLOT_PIN)*(slot_port->slot_size+1));
		if(new_pins==NULL)
			return -ENOMEM;
		if(slot_port->slot_size>0)
		{
			Memcpy(new_pins,slot_port->pins,sizeof(SLOT_PIN)*(slot_port->slot_size));
			free(slot_port->pins);	
		}	
		slot_port->pins=new_pins;
	
		pin=&slot_port->pins[slot_port->slot_size];
		pin->pin_num=slot_port->slot_size;
		slot_port->slot_size++;
	}	
	else
	{
		int i;
		for(i=0;i<slot_port->slot_size;i++)
		{
			if(slot_port->pins[i].type==0)
			{
				pin=&slot_port->pins[i];
				break;
			}
		}
		pin->pin_num=i;
	}
	pin->entity_type=entity_type;
	pin->type=type;
	pin->subtype=subtype;
	return pin->pin_num;
}

int slot_port_addrecordpin(void * port,int type,int subtype)
{
	return slot_port_addpin(port,ENTITY_RECORD,type,subtype);
}

int slot_port_addmessagepin(void * port,int type,int subtype)
{
	return slot_port_addpin(port,ENTITY_MESSAGE,type,subtype);
}


int slot_list_addslot(void * list,void * port)
{
	struct entity_slot_list * slot_list=list;
	SLOT_PORT * slot_port=port;
	Record_List * record_list=(Record_List *)malloc(sizeof(*record_list));
	if(record_list==NULL)
		return -ENOMEM;
	INIT_LIST_HEAD(&(record_list->list));
	record_list->record=port;
	List_add_tail(&record_list->list,&slot_list->head);
	return 0;
}

void * slot_list_getfirst(void * list)
{
	struct entity_slot_list * slot_list=list;
	slot_list->curr_slot=slot_list->head.next;
	if(slot_list->curr_slot==&(slot_list->head))
		return NULL;
	return slot_list->curr_slot->record;

}

void * slot_list_getnext(void * list)
{
	struct entity_slot_list * slot_list=list;
	slot_list->curr_slot=slot_list->curr_slot->list.next;
	if(slot_list->curr_slot==&(slot_list->head))
		return NULL;
	return slot_list->curr_slot->record;
}

void * slot_list_findport(void * list,char * name)
{
	struct entity_slot_list * slot_list=list;
	SLOT_PORT * slot_port;
	slot_port=slot_list_getfirst(list);
	while(slot_port!=NULL)
	{
		if(Strncmp(slot_port->name,name,DIGEST_SIZE)==0)
			return slot_port;
		slot_port=slot_list_getnext(list);
	}
	return NULL;	
}

void * slot_list_findsock(void * list,BYTE * uuid)
{
	struct entity_slot_list * slot_list=list;
	SLOT_SOCK * slot_sock;
	slot_sock=slot_list_getfirst(list);
	while(slot_sock!=NULL)
	{
		if(Memcmp(slot_sock->uuid,uuid,DIGEST_SIZE)==0)
			return slot_sock;
		slot_sock=slot_list_getnext(list);
	}
	return NULL;	
}

void * slot_list_removesock(void * list,BYTE * uuid)
{
	SLOT_SOCK * slot_sock;
	Record_List * sock_head;
	struct entity_slot_list * slot_list=list;
	slot_list->curr_slot=slot_list->head.next;

	while(slot_list->curr_slot!=&(slot_list->head))
	{
		sock_head=slot_list->curr_slot;
		slot_sock=sock_head->record;
		if(slot_sock==NULL)
			return -EINVAL;
		slot_list->curr_slot=(Record_List *)slot_list->curr_slot->list.next;
		if(Memcmp(slot_sock->uuid,uuid,DIGEST_SIZE)==0)
		{
			List_del(&sock_head->list);
			free(sock_head);	
			return slot_sock;
		}
	}
	return NULL;	
}
void * slot_create_sock(void * port,BYTE * uuid)
{
	int i;
	SLOT_SOCK * slot_sock;
	SLOT_PORT * slot_port=port;
	slot_sock=(SLOT_SOCK *)malloc(sizeof(*slot_sock));
	if(slot_sock==NULL)
		return NULL;
	Memcpy(slot_sock->uuid,uuid,DIGEST_SIZE);
	slot_sock->slot_port=slot_port;	
	slot_sock->entity_num=slot_port->slot_size;
	slot_sock->slot_mask=(1<<slot_sock->entity_num)-1;
	slot_sock->slot_flags=0;
	slot_sock->entitys=(void **)malloc(sizeof(void *)*slot_sock->entity_num);
	Memset(slot_sock->entitys,0,sizeof(void *)*slot_sock->entity_num);
	return slot_sock;
}

/*
void * ex_module_findslot(void * ex_mod,BYTE * uuid)
{
	struct entity_slot_list * slot_list;
	slot_list=slot_list_getslots(slot_list);
	if(slot_list==NULL)
		return NULL;
}
*/
int _slot_sock_addentity(void * sock,enum entity_type entity_type,int type,int subtype, void * entity)
{
	int ret;
	int i; 
	SLOT_SOCK * slot_sock=sock;
	SLOT_PORT * slot_port=slot_sock->slot_port;
	SLOT_PIN * pin;
	long long flag;
	for(i=0;i<slot_port->slot_size;i++)
	{
		pin=&slot_port->pins[i];
		if(pin->entity_type ==entity_type)
		{
			if((pin->type == type) &&(pin->subtype==subtype))
			{
				flag=1<<i;
				if(flag & slot_sock->slot_flags)
					continue;
				slot_sock->slot_flags|=flag;
				slot_sock->entitys[i]=entity;		
				break;
			}
		}
	}
	
	if(slot_sock->slot_mask==slot_sock->slot_flags)
		return 1;
	return 0; 	
}

int slot_sock_addrecord(void * sock,void * record)
{
	DB_RECORD * db_record=(DB_RECORD *)record;
	if(record==NULL)
		return -EINVAL;
	return _slot_sock_addentity(sock,ENTITY_RECORD,db_record->head.type,db_record->head.subtype,record);
}
int slot_sock_addrecorddata(void * sock,int type,int subtype, void * recorddata)
{
	return _slot_sock_addentity(sock,ENTITY_RECORD,type,subtype,recorddata);
}

int slot_sock_addmsg(void * sock, void * message)
{
	int type;
	int subtype;
	type=message_get_type(message);
	if(type<0)
		return type;
	subtype=message_get_subtype(message);
	if(subtype<0)
		return subtype;
	return _slot_sock_addentity(sock,ENTITY_MESSAGE,type,subtype,message);
} 

int slot_sock_isactive(void * sock)
{
	SLOT_SOCK * slot_sock=sock;
	if(slot_sock->slot_mask==slot_sock->slot_flags)
		return 1;
	return 0; 	
}

int slot_sock_isempty(void * sock)
{
	SLOT_SOCK * slot_sock=sock;
	if(slot_sock->slot_flags==0)
		return 1;
	return 0; 	
}

void * _slot_sock_removeentity(void * sock,enum entity_type entity_type,int type,int subtype)
{

	int ret;
	int i; 
	SLOT_SOCK * slot_sock=sock;
	SLOT_PORT * slot_port=slot_sock->slot_port;
	SLOT_PIN * pin;
	long long flag;
	void * entity=NULL;
	for(i=0;i<slot_port->slot_size;i++)
	{
		pin=&slot_port->pins[i];
		if(pin->entity_type ==entity_type)
		{
			if((pin->type == type) &&(pin->subtype==subtype))
			{
				flag=1<<i;
				if(!(flag & slot_sock->slot_flags))
					continue;
				slot_sock->slot_flags&=~flag;
				entity=slot_sock->entitys[i];
				slot_sock->entitys[i]=NULL;		
				break;
			}
		}
	}
	return entity; 	
}

void * slot_sock_removerecord(void * sock,int type,int subtype)
{
	return _slot_sock_removeentity(sock,ENTITY_RECORD,type,subtype);
}

void * slot_sock_removemessage(void * sock,int type,int subtype)
{
	return _slot_sock_removeentity(sock,ENTITY_MESSAGE,type,subtype);
}
