#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "data_type.h"
#include "list.h"
#include "attrlist.h"
#include "memfunc.h"
#include "alloc.h"
#include "json.h"
#include "struct_deal.h"
#include "basefunc.h"
#include "memdb.h"
#include "message.h"

#include "connector.h"

#define MAX_CHANNEL_SIZE	1024 
//struct connectorectod_ops connector_af_inet_info_ops;


int connector_hub_init(void * hub)
{
	struct tcloud_connector_hub * conn_hub;
	Record_List * conn_list;
	conn_hub = (struct tcloud_connector_hub *)hub;
	memset(conn_hub,0,sizeof(struct tcloud_connector_hub));

	conn_list=Dalloc(sizeof(Record_List),hub);
	if(conn_list == NULL)
		return -ENOMEM;
	conn_list->record=NULL;

	INIT_LIST_HEAD(&(conn_list->list));	
	conn_hub->connector_list=conn_list;	
	conn_hub->nfds=1;

	FD_ZERO(&conn_hub->readfds);
	FD_ZERO(&conn_hub->writefds);
	FD_ZERO(&conn_hub->exceptfds);

	conn_hub->curr_read = &(conn_list->list);
	conn_hub->curr_write = &(conn_list->list);
	conn_hub->curr_except = &(conn_list->list);

	conn_hub->curr_conn=NULL;

	return 0;
}

int connector_hub_reset(void * hub)
{
	struct tcloud_connector_hub * conn_hub;
	Record_List * conn_list;
	Record_List * record_elem;
	struct List_head * head, *curr;

	conn_hub = (struct tcloud_connector_hub *)hub;
	conn_list=(Record_List *)conn_hub->connector_list;

	FD_ZERO(&conn_hub->readfds);
	FD_ZERO(&conn_hub->writefds);
	FD_ZERO(&conn_hub->exceptfds);

	conn_hub->curr_read = &(conn_list->list);
	conn_hub->curr_write = &(conn_list->list);
	conn_hub->curr_except = &(conn_list->list);

	head = &(conn_list->list);

	curr=head->next;
	while(curr!=head)
	{
		record_elem=(Record_List *)curr;
		struct tcloud_connector * new_conn = record_elem->record;
		int new_fd;
		new_fd=new_conn->conn_ops->getfd(new_conn);

		if(new_fd<=0)
		return -EINVAL;

		FD_SET(new_fd,&conn_hub->readfds);
		if(conn_hub->nfds<new_fd+1)
		conn_hub->nfds=new_fd+1;
		curr=curr->next;
	}

	if(curr==head)
		return -EINVAL;
	return 0;
}

int  connector_hub_add_connector (void * hub,void * connector,void * attr)
{
	struct tcloud_connector_hub * conn_hub;

	struct List_head * head, *currlib;
	Record_List * record_elem;
	Record_List * conn_list;
	

	conn_hub = (struct tcloud_connector_hub *)hub;

	record_elem = Dalloc(sizeof(Record_List),hub);
	if(record_elem==NULL)
		return -ENOMEM;
	INIT_LIST_HEAD(&(record_elem->list));
	record_elem->record=connector;

	conn_list=(Record_List *)conn_hub->connector_list;

	head = &(conn_list->list);
	List_add_tail(&(record_elem->list),head);

	struct tcloud_connector * new_conn = connector;
	int new_fd;
	new_fd=new_conn->conn_ops->getfd(new_conn);

	if(new_fd<=0)
		return -EINVAL;

	FD_SET(new_fd,&conn_hub->readfds);
	if(conn_hub->nfds<new_fd+1)
		conn_hub->nfds=new_fd+1;
	return 0;
}

int  connector_hub_del_connector (void * hub,void * connector)
{
	struct tcloud_connector_hub * conn_hub;
	struct tcloud_connector * new_conn;

	struct List_head * head, *curr;
	Record_List * record_elem;
	Record_List * conn_list;
	int new_fd;

	conn_hub = (struct tcloud_connector_hub *)hub;

	conn_list=(Record_List *)conn_hub->connector_list;
	head = &(conn_list->list);

	curr=head->next;
	while(curr!=head)
	{
		record_elem=(Record_List *)curr;
		if(record_elem->record ==connector)
			break;
		curr=curr->next;
	}

	if(curr==head)
		return -EINVAL;

	if(conn_hub->curr_read==curr)
		conn_hub->curr_read=curr->next;
	if(conn_hub->curr_write==curr)
		conn_hub->curr_write=curr->next;
	if(conn_hub->curr_except==curr)
		conn_hub->curr_except=curr->next;
	List_del(curr);
	Free(record_elem);

	new_conn=(struct tcloud_connector *)connector;
	
	new_fd=new_conn->conn_ops->getfd(new_conn);

	if(new_fd<=0)
		return -EINVAL;

	FD_CLR(new_fd,&conn_hub->readfds);

	return 0;
}

int connector_hub_select (void * hub,struct timeval * timeout)
{


	struct tcloud_connector_hub * conn_hub;
	conn_hub=(struct tcloud_connector_hub *)hub;
	
	Record_List * conn_list;
	conn_list=(Record_List *)conn_hub->connector_list;

	connector_hub_reset(hub);

	return select(conn_hub->nfds,&conn_hub->readfds,&conn_hub->writefds,&conn_hub->exceptfds,timeout);
}

void * connector_hub_getactiveread (void * hub)
{
	struct tcloud_connector_hub * conn_hub;
	struct tcloud_connector * this_conn;
	struct List_head * head, *curr;
	Record_List * record_elem;
	Record_List * conn_list;
	int new_fd;

	conn_hub = (struct tcloud_connector_hub *)hub;

	conn_list=(Record_List *)conn_hub->connector_list;
	head = &(conn_list->list);

	curr=conn_hub->curr_read->next;
	this_conn=NULL;

	while(curr!=head)
	{
		record_elem=(Record_List *)curr;
		this_conn=(struct tcloud_connector *)record_elem->record;
		new_fd=this_conn->conn_ops->getfd(this_conn);

		if(new_fd<=0)
			return -EINVAL;
		if(FD_ISSET(new_fd,&conn_hub->readfds))
		{
			break;	
		}
		curr=curr->next;
	}

	conn_hub->curr_read=curr;
	if(curr==head)   //getactiveread meet the end 
	{
//		FD_ZERO(&conn_hub->readfds);
		return NULL;
	}

	return this_conn;
}

struct tcloud_connector * hub_get_connector(void * hub, char * name)
{
	struct tcloud_connector_hub * conn_hub;
	struct tcloud_connector * this_conn;
	struct List_head * head, *curr;
	Record_List * record_elem;
	Record_List * conn_list;

	int new_fd;

	struct connect_proc_info * connect_extern_info;

	conn_hub = (struct tcloud_connector_hub *)hub;

	conn_list=(Record_List *)conn_hub->connector_list;
	head = &(conn_list->list);

	//curr=conn_hub->curr_read->next;
	curr=head->next;
	this_conn=NULL;

	while(curr!=head)
	{
		record_elem=(Record_List *)curr;
		this_conn=(struct tcloud_connector *)record_elem->record;
		curr=curr->next;
		if(name!=NULL)
		{
			if(strcmp(this_conn->conn_name,name)!=0)
				continue;
		}
		return this_conn;		
	}
	return NULL;
}


struct tcloud_connector * general_hub_get_connector(void * hub, char * uuid, char * proc)
{
	struct tcloud_connector_hub * conn_hub;
	struct tcloud_connector * this_conn;
	struct List_head * head, *curr;
	Record_List * record_elem;
	Record_List * conn_list;
	int new_fd;
	struct connect_proc_info * connect_extern_info;

	conn_hub = (struct tcloud_connector_hub *)hub;

	conn_list=(Record_List *)conn_hub->connector_list;
	head = &(conn_list->list);

	//curr=conn_hub->curr_read->next;
	curr=head->next;
	this_conn=NULL;

	while(curr!=head)
	{
		record_elem=(Record_List *)curr;
		this_conn=(struct tcloud_connector *)record_elem->record;
		curr=curr->next;
		connect_extern_info=(struct connect_proc_info *)(this_conn->conn_extern_info);
		if(connect_extern_info==NULL)
			continue;
		if(uuid!=NULL)
		{
			if(strncmp(uuid,connect_extern_info->uuid,64)!=0)
				continue;
		}
		if(strcmp(this_conn->conn_name,proc)==0)
//		if(strcmp(proc,connect_extern_info->proc_name)==0)
			return this_conn;		
	}
	return NULL;
}

void * hub_get_first_connector(void * hub)
{
	struct tcloud_connector_hub * conn_hub;
	struct tcloud_connector * this_conn;
	struct List_head * head, *curr;
	Record_List * record_elem;
	Record_List * conn_list;

	conn_hub = (struct tcloud_connector_hub *)hub;
	if(hub==NULL)
		return -EINVAL;

	conn_list=(Record_List *)conn_hub->connector_list;
	head = &(conn_list->list);

	//curr=conn_hub->curr_read->next;
	curr=head->next;
	this_conn=NULL;
	conn_hub->curr_conn=curr;

	record_elem=(Record_List *)curr;
	this_conn=(struct tcloud_connector *)record_elem->record;
	return this_conn;
}

void * hub_get_next_connector(void * hub)
{
	struct tcloud_connector_hub * conn_hub;
	struct tcloud_connector * this_conn;
	struct List_head * head, *curr;
	Record_List * record_elem;
	Record_List * conn_list;

	conn_hub = (struct tcloud_connector_hub *)hub;
	if(hub==NULL)
		return -EINVAL;

	conn_list=(Record_List *)conn_hub->connector_list;
	head = &(conn_list->list);

	curr=(struct List_head *)(conn_hub->curr_conn);
	curr=curr->next;
	if(curr==head)
		return NULL;
	conn_hub->curr_conn=curr;

	record_elem=(Record_List *)curr;
	this_conn=(struct tcloud_connector *)record_elem->record;
	return this_conn;
}

#define MAX_LINE_LEN 1024

struct connector_hub_ops general_hub_ops = 
{
	.init=connector_hub_init,
	.ioctl=NULL,	
	.add_connector=connector_hub_add_connector,	
	.del_connector=connector_hub_del_connector,	
//	.connect=connector_af_inet_connect,	
	.select=connector_hub_select,	
	.getactiveread=connector_hub_getactiveread,	
	.getactivewrite=NULL,	
	.getactiveexcept=NULL	
};

void * get_connector_hub()
{
	struct tcloud_connector_hub * hub;
	hub=Calloc(sizeof(struct tcloud_connector_hub));
	if(hub==NULL)
		return -ENOMEM;

	connector_hub_init(hub);
	
	hub->hub_ops= &general_hub_ops;
	return hub;
}

