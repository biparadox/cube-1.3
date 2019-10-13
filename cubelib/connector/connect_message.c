#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

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

void * hub_get_connector_byreceiver(void * hub, char * uuid, char * name, char * service)
{
	struct tcloud_connector * this_conn, *temp_conn;
	
	int new_fd;

	temp_conn=hub_get_first_connector(hub);

	while(temp_conn!=NULL)
	{
		this_conn=temp_conn;
		temp_conn=hub_get_next_connector(hub);
		
		if(this_conn->conn_type==CONN_CHANNEL)
		{
			struct connect_proc_info * connect_extern_info;
			connect_extern_info=(struct connect_proc_info *)(this_conn->conn_extern_info);
			if(connect_extern_info==NULL)
				continue;
			if(uuid!=NULL)
			{
				if(strncmp(uuid,connect_extern_info->uuid,64)!=0)
					continue;
			}
			if(name==NULL)
				return this_conn;
			if(strcmp(this_conn->conn_name,name)==0)
				return this_conn;		
		}
		else if(this_conn->conn_type==CONN_CLIENT)
		{
			struct connect_syn * connect_extern_info;
			connect_extern_info=(struct connect_syn *)(this_conn->conn_extern_info);
			if(connect_extern_info==NULL)
				continue;
			if(uuid!=NULL)
			{
				if(strncmp(uuid,connect_extern_info->uuid,DIGEST_SIZE)!=0)
					continue;
			}
			if(name!=NULL)
			{
				if(strncmp(name,connect_extern_info->server_name,DIGEST_SIZE)!=0)
					continue;
			}
			if(service==NULL)
				return this_conn;
			if(strcmp(connect_extern_info->service,service)==0)
				return this_conn;		

		}
		else if((this_conn->conn_type == CONN_P2P_BIND)
			||(this_conn->conn_type==CONN_P2P_RAND))
		{
			struct sysconn_peer_info * sys_peer=NULL;
			void * p2p_peer = af_inet_p2p_getfirstpeer(this_conn);	
			while(p2p_peer!=NULL)
			{
				sys_peer=af_inet_p2p_getpeerexterninfo(p2p_peer);
				p2p_peer=af_inet_p2p_getnextpeer(this_conn);
				if(sys_peer!=NULL)
				{
					if(uuid!=NULL)
					{
						if(Strncmp(uuid,sys_peer->machine_uuid,DIGEST_SIZE)!=0)
							continue;
					}
					if(name!=NULL)
					{
						if(Strncmp(name,sys_peer->proc_name,DIGEST_SIZE)!=0)
							continue;
					}
					if(service==NULL)
						return this_conn;
					if(Strncmp(sys_peer->user_name,service,DIGEST_SIZE)==0)
						return this_conn;		
				}
			}
			continue;
		}
	}
	return NULL;
}

int connector_inet_get_uuid(void * conn,BYTE * uuid)
{
	int ret;
	int i;
	TCLOUD_CONN * inet_conn=(TCLOUD_CONN *)conn;

	if(conn!=NULL)
	{	

		if(connector_get_type(conn)==CONN_CLIENT)
		{
			struct connect_syn * syn_info=(struct connect_syn *)(inet_conn->conn_extern_info);
			if(syn_info!=NULL)
			{
				comp_proc_uuid(syn_info->uuid,syn_info->server_name,uuid);
			}

		}
		else if(connector_get_type(conn)==CONN_CHANNEL)
		{
			struct connect_proc_info * channel_info=(struct connect_ack *)(inet_conn->conn_extern_info);
			if(channel_info!=NULL)
			{
				comp_proc_uuid(channel_info->uuid,channel_info->proc_name,uuid);
			}

		}
		else if((inet_conn->conn_type == CONN_P2P_BIND)
			||(inet_conn->conn_type==CONN_P2P_RAND))
		{
			struct sysconn_peer_info * sys_peer=NULL;
			sys_peer=af_inet_p2p_getpeerexterninfo(inet_conn);
			if(sys_peer!=NULL)
			{
				Memcpy(sys_peer->uuid,uuid,DIGEST_SIZE);
			}
		}
	}
	return 0;

}

void * hub_get_connector_bypeeruuid(void * hub,char * uuid)
{
	int ret;
	int i;
	TCLOUD_CONN * conn;
	BYTE conn_uuid[DIGEST_SIZE];

	conn=hub_get_first_connector(hub);
	
	while(conn!=NULL)
	{	

		if(connector_get_type(conn)==CONN_CLIENT)
		{
			struct connect_syn * syn_info=(struct connect_syn *)(conn->conn_extern_info);
			if(syn_info!=NULL)
			{
				comp_proc_uuid(syn_info->uuid,syn_info->server_name,conn_uuid);
				if(strncmp(conn_uuid,uuid,DIGEST_SIZE)==0)
					break;
			}

		}
		else if(connector_get_type(conn)==CONN_CHANNEL)
		{
			struct connect_proc_info * channel_info=(struct connect_ack *)(conn->conn_extern_info);
			if(channel_info!=NULL)
			{
				comp_proc_uuid(channel_info->uuid,channel_info->proc_name,conn_uuid);
				if(strncmp(conn_uuid,uuid,DIGEST_SIZE)==0)
					break;
			}

		}
		else if((conn->conn_type == CONN_P2P_BIND)
			||(conn->conn_type==CONN_P2P_RAND))
		{
			struct sysconn_peer_info * sys_peer=NULL;
			void * p2p_peer = af_inet_p2p_getfirstpeer(conn);	
			while(p2p_peer!=NULL)
			{
				sys_peer=af_inet_p2p_getpeerexterninfo(p2p_peer);
				if(sys_peer!=NULL)
				{
					if(Strncmp(uuid,sys_peer->uuid,DIGEST_SIZE)==0)
						return conn;
				}
				p2p_peer=af_inet_p2p_getnextpeer(conn);
			}
		}
		conn=hub_get_next_connector(hub);
	}
	return conn;

}

void * build_server_syn_message(char * service,char * local_uuid,char * proc_name)
{
	void * message_box;
	struct connect_syn * server_syn;
	MSG_HEAD * message_head;
	void * syn_template;
	BYTE * blob;
	int record_size;
	int retval;

	message_box=message_create(DTYPE_MESSAGE,SUBTYPE_CONN_SYNI,NULL); // SYNI
	if(message_box==NULL)
		return -EINVAL;
	if(IS_ERR(message_box))
		return -EINVAL;
	server_syn=Dalloc(sizeof(struct connect_syn),message_box);
	if(server_syn == NULL)
	{
		message_free(message_box);
		return -ENOMEM;
	}

	Memset(server_syn,0,sizeof(struct connect_syn));
	
	Memcpy(server_syn->uuid,local_uuid,DIGEST_SIZE);
	server_syn->server_name=dup_str(proc_name,0);

	if(service!=NULL)
	{
		server_syn->service=dup_str(service,0);
	}
	retval=message_add_record(message_box,server_syn);

//	message_head->state=MSG_FLOW_INIT;
	message_set_state(message_box,MSG_FLOW_INIT);
	//printf("init message success!\n");
	return message_box;

}

void * build_peer_ack_message(void * message_box,char * local_uuid,char * proc_name,void * conn)
{
	MSG_HEAD * message_head;
	struct connect_ack  * client_ack;
	struct connect_syn  * server_syn;
	int retval;
	void * ack_template;
	int record_size;
	void * blob;
	struct tcloud_connector * temp_conn=conn;
	void * new_msg;
	void * p2p_peer_info;
	BYTE conn_uuid[DIGEST_SIZE];	

	new_msg=message_create(DTYPE_MESSAGE,SUBTYPE_CONN_ACKI,message_box); //ACKI
	if(new_msg==NULL)
		return -EINVAL;

	client_ack=Dalloc(sizeof(struct connect_ack),new_msg);
	if(client_ack==NULL)
	{
		message_free(new_msg);
		return -ENOMEM;
	}

	p2p_peer_info=af_inet_p2p_getcurrpeer(conn);
	if(p2p_peer_info==NULL)
		return -EINVAL;

	Memset(client_ack,0,sizeof(struct connect_ack));
		// monitor send a new image message
	retval=message_get_record(message_box,&server_syn,0);

	if(retval<0)
	{
		message_free(new_msg);
		return -EINVAL;
	}
	if(server_syn==NULL)
	{
		message_free(new_msg);
		return -EINVAL;
	}
	
	struct sysconn_peer_info * peer_info=Talloc0(sizeof(*peer_info));
	if(peer_info==NULL)
		return -ENOMEM;
	comp_proc_uuid(server_syn->uuid,server_syn->server_name,conn_uuid);

	Memcpy(peer_info->uuid,conn_uuid,DIGEST_SIZE);
	Memcpy(peer_info->machine_uuid,server_syn->uuid,DIGEST_SIZE);
	Strncpy(peer_info->proc_name,server_syn->server_name,DIGEST_SIZE);
//	peer_info->peer_addr=dup_str(channel_conn->conn_peeraddr,DIGEST_SIZE*4);		
	memdb_store(peer_info,DTYPE_SYS_CONN,SUBTYPE_PEER_INFO,connector_getname(conn));
	af_inet_p2p_setpeerexterninfo(p2p_peer_info,peer_info);

	Memcpy(client_ack->uuid,local_uuid,DIGEST_SIZE);
//	client_ack->client_name=dup_str("unknown machine",0);
	client_ack->client_name=dup_str(proc_name,0);
	client_ack->client_process=dup_str(proc_name,0);
	client_ack->client_addr=dup_str("unknown addr",0);

	Memcpy(client_ack->server_uuid,server_syn->uuid,DIGEST_SIZE);
	client_ack->server_name=dup_str(server_syn->server_name,0);
	client_ack->service=dup_str(server_syn->service,0);
	client_ack->server_addr=dup_str(server_syn->server_addr,0);
	client_ack->flags=server_syn->flags;
	strncpy(client_ack->nonce,server_syn->nonce,DIGEST_SIZE);

	message_set_state(new_msg,MSG_FLOW_INIT);
	retval=message_add_record(new_msg,client_ack);
	return new_msg;
}
void * build_client_ack_message(void * message_box,char * local_uuid,char * proc_name,void * conn)
{
	MSG_HEAD * message_head;
	struct connect_ack  * client_ack;
	struct connect_syn  * server_syn;
	int retval;
	void * ack_template;
	int record_size;
	void * blob;
	struct tcloud_connector * temp_conn=conn;
	void * new_msg;

	new_msg=message_create(DTYPE_MESSAGE,SUBTYPE_CONN_ACKI,message_box); //ACKI
	if(new_msg==NULL)
		return -EINVAL;

	client_ack=Dalloc(sizeof(struct connect_ack),new_msg);
	if(client_ack==NULL)
	{
		message_free(new_msg);
		return -ENOMEM;
	}
//	server_syn=malloc(sizeof(struct connect_syn));
//	if(server_syn==NULL)
//		return -ENOMEM;

	Memset(client_ack,0,sizeof(struct connect_ack));
		// monitor send a new image message
	retval=message_get_record(message_box,&server_syn,0);

	if(retval<0)
	{
		message_free(new_msg);
		return -EINVAL;
	}
	if(server_syn==NULL)
	{
		message_free(new_msg);
		return -EINVAL;
	}
	temp_conn->conn_extern_info=server_syn;

	Memcpy(client_ack->uuid,local_uuid,DIGEST_SIZE);
//	client_ack->client_name=dup_str("unknown machine",0);
	client_ack->client_name=dup_str(proc_name,0);
	client_ack->client_process=dup_str(proc_name,0);
	client_ack->client_addr=dup_str("unknown addr",0);

	Memcpy(client_ack->server_uuid,server_syn->uuid,DIGEST_SIZE);
	client_ack->server_name=dup_str(server_syn->server_name,0);
	client_ack->service=dup_str(server_syn->service,0);
	client_ack->server_addr=dup_str(server_syn->server_addr,0);
	client_ack->flags=server_syn->flags;
	strncpy(client_ack->nonce,server_syn->nonce,DIGEST_SIZE);

	message_set_state(new_msg,MSG_FLOW_INIT);
	retval=message_add_record(new_msg,client_ack);
	return new_msg;
}

int receive_local_client_ack(void * message_box,void * conn,void * hub)
{
	MSG_HEAD * message_head;
	struct connect_ack  * client_ack;
	int retval;
	struct tcloud_connector * channel_conn=conn;
	void * ack_template;
	int record_size;
	void * blob;
	struct connect_proc_info * channel_info;
	char buf[DIGEST_SIZE*4];
	int addr_len;

	client_ack=Talloc(sizeof(struct connect_ack));
	if(client_ack==NULL)
		return -ENOMEM;
	Memset(client_ack,0,sizeof(struct connect_ack));


	channel_info=Dalloc(sizeof(struct connect_proc_info),conn);
	if(channel_info==NULL)
		return -ENOMEM;
	Memset(channel_info,0,sizeof(struct connect_proc_info));
//	channel_info->channel_state=PROC_CHANNEL_RECVACK;
	channel_conn->conn_extern_info=channel_info;

	retval=message_get_record(message_box,&client_ack,0);

	if(retval<0)
		return -EINVAL;

	channel_conn->conn_ops->setname(channel_conn,client_ack->client_name);

	BYTE conn_uuid[DIGEST_SIZE];

	comp_proc_uuid(client_ack->uuid,client_ack->client_process,conn_uuid);

	TCLOUD_CONN * temp_conn=hub_get_connector_bypeeruuid(hub,conn_uuid);
	if(temp_conn!=NULL)
	{
		((TCLOUD_CONN_HUB *)hub)->hub_ops->del_connector(hub,temp_conn);
		temp_conn->conn_ops->disconnect(temp_conn);
	}
	
	Memcpy(channel_info->uuid,client_ack->uuid,DIGEST_SIZE);
	channel_info->proc_name=dup_str(client_ack->client_process,0);
	channel_info->channel_name=NULL;
	channel_info->islocal=1;
	
	connector_setstate(channel_conn,CONN_CHANNEL_HANDSHAKE);
	struct sysconn_peer_info * peer_info=Talloc0(sizeof(*peer_info));
	if(peer_info==NULL)
		return -ENOMEM;

	Memcpy(peer_info->uuid,conn_uuid,DIGEST_SIZE);
	Memcpy(peer_info->machine_uuid,client_ack->uuid,DIGEST_SIZE);
	Strncpy(peer_info->proc_name,client_ack->client_process,DIGEST_SIZE);
	peer_info->peer_addr=dup_str(channel_conn->conn_peeraddr,DIGEST_SIZE*4);		
	memdb_store(peer_info,DTYPE_SYS_CONN,SUBTYPE_PEER_INFO,channel_info->channel_name);
	return 0;
}

int receive_local_peer_ack(void * message_box,void * conn,void * hub)
{
	MSG_HEAD * message_head;
	struct connect_ack  * client_ack;
	int retval;
	struct tcloud_connector * peer_conn=conn;
	void * ack_template;
	int record_size;
	void * blob;
	char buf[DIGEST_SIZE*4];
	int addr_len;
	void * p2p_peer_info;
	void * old_peer_info;

//	client_ack=Talloc(sizeof(struct connect_ack));
//	if(client_ack==NULL)
//		return -ENOMEM;
//	Memset(client_ack,0,sizeof(struct connect_ack));

	p2p_peer_info=af_inet_p2p_getcurrpeer(conn);
	if(p2p_peer_info==NULL)
		return -EINVAL;

	retval=message_get_record(message_box,&client_ack,0);

	if(retval<0)
		return -EINVAL;


	BYTE conn_uuid[DIGEST_SIZE];

	comp_proc_uuid(client_ack->uuid,client_ack->client_process,conn_uuid);

	old_peer_info = af_inet_p2p_findpeerbyuuid(conn,conn_uuid);
	if(old_peer_info!=NULL)
	{
		if(old_peer_info!=p2p_peer_info)
		{
			af_inet_p2p_delpeer(conn,old_peer_info);
		}
		else
		{
			void * extern_info=af_inet_p2p_getpeerexterninfo(old_peer_info);
			if(extern_info!=NULL)
				Free(extern_info);	
		}
	}
	
//	Memcpy(p2p_peer_info->uuid,conn_uuid,DIGEST_SIZE);
//	Memcpy(p2p_peer_info->peer_uuid,client_ack->uuid,DIGEST_SIZE);
//	Strncpy(p2p_peer_info->proc_name,client_ack->client_process,DIGEST_SIZE);
	
	struct sysconn_peer_info * peer_info=Talloc0(sizeof(*peer_info));
	if(peer_info==NULL)
		return -ENOMEM;

	Memcpy(peer_info->uuid,conn_uuid,DIGEST_SIZE);
	Memcpy(peer_info->machine_uuid,client_ack->uuid,DIGEST_SIZE);
	Strncpy(peer_info->proc_name,client_ack->client_process,DIGEST_SIZE);
//	peer_info->peer_addr=dup_str(channel_conn->conn_peeraddr,DIGEST_SIZE*4);		
	memdb_store(peer_info,DTYPE_SYS_CONN,SUBTYPE_PEER_INFO,connector_getname(conn));
	af_inet_p2p_setpeerexterninfo(p2p_peer_info,peer_info);
	

	return 0;
}

