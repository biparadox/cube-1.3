#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

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

/*
struct connectorector_af_inet
{
	struct sockaddr_in adr_inet;
	int fd;
	int len_inet;
};*/


void connector_initvalue(void * connector)
{

	struct tcloud_connector * this_conn;
	this_conn=(struct tcloud_connector *)connector;
	if(this_conn==NULL)
		return;
	memset(this_conn,0,sizeof(struct tcloud_connector));
	this_conn->conn_type=CONN_INVALID;
	this_conn->conn_protocol=-1;

//	this_conn->conn_name=NULL;
//	this_conn->conn_ops=NULL;
//	this_conn->conn_base_info=NULL;
//	this_conn->conn_var_info=NULL;
	return;
}

char *  connector_getname (void * connector)
{

	struct tcloud_connector * this_conn;
//	struct connector_af_unix_info * base_info;
	int retval;
	this_conn=(struct tcloud_connector *)connector;
	return this_conn->conn_name;
}

char *  connector_getaddr (void * connector)
{

	struct tcloud_connector * this_conn;
//	struct connector_af_unix_info * base_info;
	int retval;
	this_conn=(struct tcloud_connector *)connector;
	return this_conn->conn_addr;
}

char *  connector_getpeeraddr (void * connector)
{

	struct tcloud_connector * this_conn;
//	struct connector_af_unix_info * base_info;
	int retval;
	this_conn=(struct tcloud_connector *)connector;
	return this_conn->conn_peeraddr;
}

int connector_getstate (void * connector)
{

	struct tcloud_connector * this_conn;
	int retval;
	this_conn=(struct tcloud_connector *)connector;
	return this_conn->conn_state;
}

int connector_setstate (void * connector,int state)
{

	struct tcloud_connector * this_conn;
	int retval;
	this_conn=(struct tcloud_connector *)connector;
	return this_conn->conn_state=state;
}


int   connector_setname (void * connector,char * name)
{

	struct tcloud_connector * this_conn;
//	struct connector_af_unix_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;
	if(this_conn->conn_name != NULL)
	free(this_conn->conn_name);
	this_conn->conn_name=Dalloc(strlen(name)+1,this_conn);
		
	if(this_conn->conn_name==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_name,name);

	return 0;
}

int  connector_getfd (void * connector)
{

	struct tcloud_connector * this_conn;
//	struct connector_af_unix_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	return this_conn->conn_fd;
}

void * connector_get_server (void * connector)
{

	struct tcloud_connector * this_conn;
//	struct connector_af_unix_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;
	if(this_conn->conn_type !=CONN_CHANNEL)
		return NULL;

	if(this_conn->conn_ops->getserver==NULL)
		return -EINVAL;
	return this_conn->conn_ops->getserver(this_conn);

}

int  connector_get_type (void * connector)
{

	struct tcloud_connector * this_conn;
//	struct connector_af_unix_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	return this_conn->conn_type;
}

int  connector_get_protocol (void * connector)
{

	struct tcloud_connector * this_conn;
//	struct connector_af_unix_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	return this_conn->conn_protocol;
}


void * get_connector(int type,int protocol)
{
	struct tcloud_connector * connector;
	connector=Calloc(sizeof(struct tcloud_connector));
	if(connector==NULL)
		return -ENOMEM;

	connector_initvalue(connector);
/*	
	if(protocol==AF_UNIX)
	{
			connector->conn_protocol=AF_UNIX;
		switch(type)
		{
			case CONN_CLIENT:
				connector->conn_type=type;
				connector->conn_ops= &connector_af_unix_client_ops;
				connector->conn_state=CONN_CLIENT_INIT;
				break;
			case CONN_SERVER:
				connector->conn_type=type;
				connector->conn_ops= &connector_af_unix_server_ops;
				connector->conn_state=CONN_SERVER_INIT;
				break;
			case CONN_CHANNEL:
				connector->conn_type=type;
				connector->conn_ops= &connector_af_unix_channel_ops;
				connector->conn_state=CONN_CHANNEL_INIT;
				break;
			default:
				return -EINVAL;
		}
	}
	else */
	if(protocol==AF_INET)
	{
		connector->conn_protocol=AF_INET;
		switch(type)
		{
			case CONN_CLIENT:
				connector->conn_type=type;
				connector->conn_ops= &connector_af_inet_client_ops;
				connector->conn_state=CONN_CLIENT_INIT;
				break;
			case CONN_SERVER:
				connector->conn_type=type;
				connector->conn_ops= &connector_af_inet_server_ops;
				connector->conn_state=CONN_SERVER_INIT;
				break;
			case CONN_CHANNEL:
				connector->conn_type=type;
				connector->conn_ops= &connector_af_inet_channel_ops;
				connector->conn_state=CONN_CHANNEL_INIT;
				break;
			case CONN_P2P_BIND:
				connector->conn_type=type;
				connector->conn_ops= &connector_af_inet_p2p_ops;
				connector->conn_state=CONN_CHANNEL_INIT;
				break;
			case CONN_P2P_RAND:
				connector->conn_type=type;
				connector->conn_ops= &connector_af_inet_p2p_cli_ops;
				connector->conn_state=CONN_CHANNEL_INIT;
				break;
			default:
				return -EINVAL;
		}
	}
	else
		return 	-EINVAL;
	return connector;
}
