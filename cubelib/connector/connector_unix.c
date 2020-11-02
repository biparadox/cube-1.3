#ifdef USER_MODE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <memfunc.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "data_type.h"
#include "list.h"
#include "memfunc.h"
#include "alloc.h"
#include "json.h"
#include "struct_deal.h"
#include "basefunc.h"
#include "memdb.h"
#include "message.h"

#include "connector.h"

#define MAX_CHANNEL_SIZE	1024 
//struct connectorectod_ops connector_af_unix_info_ops;


struct connector_af_unix_info
{
	struct sockaddr_un adr_unix;
	int len_unix;
	void * sem_struct;
};

struct connector_af_unix_server_info
{
	int channel_num;
	Record_List channel_list;
	struct List_head * curr_channel;
};

struct connector_af_unix_channel_info
{
	void * server;
};

void * create_channel_af_unix_info(void * server_info)
{
	struct connector_af_unix_info * src_info;
	struct connector_af_unix_info * new_info;
	src_info=(struct connector_af_unix_info *)server_info;
	new_info=(struct connector_af_unix_info *)malloc(sizeof (struct connector_af_unix_info));
	if(new_info == NULL)
		return NULL;
	memset(new_info,0,sizeof (struct connector_af_unix_info));
	memcpy(&new_info->adr_unix,&src_info->adr_unix,src_info->len_unix);
	new_info->len_unix=src_info->len_unix;
	return new_info;
}


int  connector_af_unix_info_init (void * connector,char * addr)
{

	struct tcloud_connector * this_conn;
	struct connector_af_unix_info * base_info;
	char addrbuf[108];
	int len_unix;
	int retval;


	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_protocol!=AF_UNIX)
		return -EINVAL;
	if(this_conn->conn_type==CONN_CHANNEL)
		return 0;


	base_info=malloc(sizeof(struct connector_af_unix_info));
	if(base_info==NULL)
		return -ENOMEM;
	memset(base_info,0,sizeof(struct connector_af_unix_info));
	this_conn->conn_base_info=base_info;
	

    	this_conn->conn_fd  = socket(AF_UNIX,SOCK_STREAM,0);
	if(this_conn->conn_fd <0)
		return this_conn->conn_fd;
/*
* Form an AF_UNIX Address:
*/
	memset(&base_info->adr_unix,0,sizeof(base_info->adr_unix));

	base_info->adr_unix.sun_family = AF_UNIX;

	addrbuf[0]='Z';    // this is only a place holder charactor;
	strncpy(addrbuf+1,addr,sizeof(base_info->adr_unix.sun_path)-2);
	strncpy(base_info->adr_unix.sun_path,addrbuf,
		sizeof base_info->adr_unix.sun_path-1);

	base_info->adr_unix.sun_path[sizeof base_info->adr_unix.sun_path-1] = 0;

	base_info->len_unix = SUN_LEN(&base_info->adr_unix);

/* Now make first byte null */
	base_info->adr_unix.sun_path[0] = 0;

/*
* Now bind the address to the socket:
*/
	retval = bind(this_conn->conn_fd,
		(struct sockaddr *)&(base_info->adr_unix),
		base_info->len_unix);
	if ( retval == -1 )
		return -ENONET;

	return this_conn->conn_fd;
}


int  connector_af_unix_client_init (void * connector,char * name,char * addr)
{

	struct tcloud_connector * this_conn;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	this_conn->conn_name=malloc(strlen(name)+1);
	if(this_conn->conn_name==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_name,name);
	
	this_conn->conn_addr=malloc(strlen(addr)+1);
	if(this_conn->conn_addr==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_addr,addr);
	
	this_conn->conn_peeraddr=malloc(strlen(addr)+1);
	if(this_conn->conn_addr==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_peeraddr,addr);
	
	connector_af_unix_info_init (connector,addr);
	
	return 0;
};

int  connector_af_unix_server_init (void * connector,char * name,char * addr)
{

	struct tcloud_connector * this_conn;
	struct connector_af_unix_info * base_info;
	struct connector_af_unix_server_info * server_info;

	int retval;

	this_conn=(struct tcloud_connector *)connector;

	this_conn->conn_name=malloc(strlen(name)+1);
	if(this_conn->conn_name==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_name,name);
	
	this_conn->conn_addr=malloc(strlen(addr)+1);
	if(this_conn->conn_addr==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_addr,addr);

	this_conn->conn_peeraddr=NULL;
	
	connector_af_unix_info_init (connector,addr);
	base_info=(struct connector_af_unix_info *)(this_conn->conn_base_info);

	server_info=malloc(sizeof(struct connector_af_unix_server_info));
	if(server_info==NULL)
		return -ENOMEM;
	memset(server_info,0,sizeof(struct connector_af_unix_server_info));
	INIT_LIST_HEAD(&(server_info->channel_list.list));	
	server_info->curr_channel=&(server_info->channel_list.list);

	this_conn->conn_var_info=server_info;
	
	return 0;
}

int  connector_af_unix_listen (void * connector)
{

	struct tcloud_connector * this_conn;
	struct connector_af_unix_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_type!=CONN_SERVER)
		return -EINVAL;
	if(this_conn->conn_protocol!=AF_UNIX)
		return -EINVAL;
	base_info=(struct connector_af_unix_info * )this_conn->conn_base_info;
	return listen(this_conn->conn_fd,10);
}

void * connector_af_unix_accept (void * connector)
{

	struct tcloud_connector * this_conn;
	struct tcloud_connector * channel_conn;
	struct connector_af_unix_info * base_info;
	struct connector_af_unix_server_info * server_info;
	struct connector_af_unix_info * channel_base_info;
	struct connector_af_unix_channel_info * channel_info;
	int retval;
	int accept_fd;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_type!=CONN_SERVER)
		return -EINVAL;
	if(this_conn->conn_protocol!=AF_UNIX)
		return -EINVAL;
	base_info=(struct connector_af_unix_info * )this_conn->conn_base_info;
	server_info=(struct connector_af_unix_server_info * )this_conn->conn_var_info;

	accept_fd= accept(this_conn->conn_fd,NULL,0);
	if(accept_fd<=0)
		return NULL;

	channel_conn=get_connector(CONN_CHANNEL,AF_UNIX);
	if(channel_conn==NULL)
		return NULL;
	channel_base_info = create_channel_af_unix_info(base_info);
	channel_conn->conn_fd=accept_fd;

	channel_conn->conn_ops->setname(channel_conn,this_conn->conn_ops->getname(this_conn));

	channel_info=malloc(sizeof(struct connector_af_unix_channel_info));
	if(channel_info==NULL)
		return NULL;
	channel_info->server=this_conn;
	channel_conn->conn_base_info=channel_base_info;
	channel_conn->conn_var_info=channel_info;

	struct List_head * head, *currlib;
	Record_List * record_elem;

	record_elem = malloc(sizeof(Record_List));
	if(record_elem==NULL)
		return -ENOMEM;
	INIT_LIST_HEAD(&(record_elem->list));
	record_elem->record=channel_conn;

	head = &(server_info->channel_list.list);
	List_add_tail(&(record_elem->list),head);

	server_info->channel_num++;

	return channel_conn;
}

int  connector_af_unix_connect (void * connector)
{

	struct tcloud_connector * this_conn;
	struct connector_af_unix_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_protocol!=AF_UNIX)
		return -EINVAL;
	base_info=(struct connector_af_unix_info * )this_conn->conn_base_info;
	return connect(this_conn->conn_fd,&base_info->adr_unix,base_info->len_unix);
}

int  connector_af_unix_read (void * connector,void * buf, size_t count)
{

	struct tcloud_connector * this_conn;
	struct connector_af_unix_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	base_info=(struct connector_af_unix_info * )this_conn->conn_base_info;

	printf("test read fd %d!\n",this_conn->conn_fd);
	return read(this_conn->conn_fd,buf,count);
}

int  connector_af_unix_write (void * connector,void * buf,size_t count)
{

	struct tcloud_connector * this_conn;
	struct connector_af_unix_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	base_info=(struct connector_af_unix_info * )this_conn->conn_base_info;
	printf("test write fd %d!\n",this_conn->conn_fd);
	return write(this_conn->conn_fd,buf,count);
}

int  connector_af_unix_wait (void * connector,struct timeval * timeout)
{

	struct tcloud_connector * this_conn;
	struct connector_af_unix_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_protocol!=AF_UNIX)
		return -EINVAL;
	base_info=(struct connector_af_unix_info * )this_conn->conn_base_info;
	return 0;
}

int  connector_af_unix_disconnect (void * connector)
{

	struct tcloud_connector * this_conn;
	struct connector_af_unix_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_protocol!=AF_UNIX)
		return -EINVAL;
	base_info=(struct connector_af_unix_info * )this_conn->conn_base_info;
	return 0;
}

void * connector_af_unix_get_server(void * connector)
{

	struct tcloud_connector * this_conn;
	struct connector_af_unix_channel_info * channel_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;
	if(this_conn->conn_protocol!=AF_UNIX)
		return -EINVAL;
	if(this_conn->conn_type !=CONN_CHANNEL)
		return NULL;

	channel_info=(struct connector_af_unix_channel_info *)(this_conn->conn_var_info);
	return channel_info->server;

}


struct connector_ops connector_af_unix_server_ops = 
{
	.conn_type = CONN_SERVER,
	.init=connector_af_unix_server_init,
	.ioctl=NULL,	
	.getname=connector_getname,	
	.getaddr=connector_getaddr,	
	.getpeeraddr=connector_getpeeraddr,	
	.setname=connector_setname,	
	.listen=connector_af_unix_listen,	
	.accept=connector_af_unix_accept,	
//	.connect=connector_af_unix_connect,	
	.read=connector_af_unix_read,	
	.write=connector_af_unix_write,	
	.getfd=connector_getfd,	
	.wait=connector_af_unix_wait,	
	.disconnect=connector_af_unix_disconnect,	
//	.select = connector_af_unix_select,
//	.isset = connector_af_unix_isset
};

struct connector_ops connector_af_unix_client_ops = 
{
	.conn_type = CONN_CLIENT,
	.init=connector_af_unix_client_init,
	.ioctl=NULL,	
	.getname=connector_getname,	
	.getaddr=connector_getaddr,	
	.getpeeraddr=connector_getpeeraddr,	
	.setname=connector_setname,	
	.connect=connector_af_unix_connect,	
	.read=connector_af_unix_read,	
	.write=connector_af_unix_write,	
	.getfd=connector_getfd,	
	.wait=connector_af_unix_wait,	
	.disconnect=connector_af_unix_disconnect	
};

struct connector_ops connector_af_unix_channel_ops = 
{
	.conn_type = CONN_CHANNEL,
	.init=NULL,
	.ioctl=NULL,	
	.getname=connector_getname,	
	.getaddr=connector_getaddr,	
	.getpeeraddr=connector_getpeeraddr,	
	.setname=connector_setname,	
	.connect=connector_af_unix_connect,	
	.read=connector_af_unix_read,	
	.write=connector_af_unix_write,	
	.getfd=connector_getfd,	
	.wait=connector_af_unix_wait,	
	.getserver=connector_af_unix_get_server,
	.disconnect=connector_af_unix_disconnect	
};
