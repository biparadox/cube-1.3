#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
#include "sys_func.h"

#include "connector.h"

#define MAX_CHANNEL_SIZE	1024 
#define MAX_P2P_SIZE		1450 
//struct connectorectod_ops connector_af_inet_info_ops;
struct connector_af_inet_info
{
	struct sockaddr_in adr_inet;
	int len_inet;
	void * sem_struct;
};

enum p2p_read_state
{
	P2P_WAIT_PEER=0x01,
	P2P_READ_CONN,
	P2P_READ_BUF,
	P2P_READ_FIN
};

struct connector_af_inet_p2p_peer_info
{
//	BYTE uuid[DIGEST_SIZE];
//	BYTE peer_uuid[DIGEST_SIZE];
//	char proc_name[DIGEST_SIZE];
//	char user_name[DIGEST_SIZE];
	struct sockaddr_in adr_inet;
	int len_inet;
	int buf_size;
	void * buf;
	void * extern_info;
};

struct connector_af_inet_server_info
{
	int channel_num;
	Record_List channel_list;
	struct List_head * curr_channel;
};

struct connector_af_inet_channel_info
{
	void * server;
};

struct connector_af_inet_p2p_info
{
	int peer_num;
	enum connector_type type;
	enum p2p_read_state read_state;
	Record_List peer_list;
	struct List_head * curr_peer;
};


static void setnonblocking(int sockfd) {  
	int flag = fcntl(sockfd, F_GETFL, 0);  
	if (flag < 0) {  
        	print_cubeerr("fcntl F_GETFL fail");  
        	return;  
    	}  
    	if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0) {  
        	print_cubeerr("fcntl F_SETFL fail");  
    	}
	return;  
}
 
void * create_channel_af_inet_info(void * server_info)
{
	struct connector_af_inet_info * src_info;
	struct connector_af_inet_info * new_info;
	src_info=(struct connector_af_inet_info *)server_info;
	new_info=(struct connector_af_inet_info *)Talloc(sizeof (struct connector_af_inet_info));
	if(new_info == NULL)
		return NULL;
	memset(new_info,0,sizeof (struct connector_af_inet_info));
	memcpy(&new_info->adr_inet,&src_info->adr_inet,src_info->len_inet);
	new_info->len_inet=src_info->len_inet;
	return new_info;
}


int	af_inet_formaddr(struct sockaddr_in * adr_inet, int * len,char * addrstring)
{
	char addrbuf[108];
	int i,retval;
	int namelen;

	memset(adr_inet,0,sizeof(struct sockaddr_in));

	adr_inet->sin_family = AF_INET;

	namelen=strlen(addrstring);
	for(i=0;i<namelen;i++)
	{
		if(addrstring[i]==':')
			break;
		addrbuf[i]=addrstring[i];
	}
	if(i==namelen)
	{
		//printf("no port number!\n");
		return -EINVAL;
	}
		
	addrbuf[i++]=0;

	*len = sizeof (struct sockaddr_in);

	adr_inet->sin_addr.s_addr=inet_addr(addrbuf);
	if(adr_inet->sin_addr.s_addr==INADDR_NONE)
		return -EINVAL;

	memcpy(addrbuf,addrstring+i,namelen-i+1);

	adr_inet->sin_port=htons(atoi(addrbuf));
	
	return 0;
}


char *  af_inet_getaddrstring(struct sockaddr_in * adr_inet)
{
	char addrbuf[128];
	int i,retval;
	int namelen;
	char *cp;
	char * addrstring;

	cp=inet_ntoa(adr_inet->sin_addr);

	sprintf(addrbuf,":%d",adr_inet->sin_port);

	namelen=strlen(cp)+strlen(addrbuf);

	addrstring=Talloc(namelen+1);
	if(addrstring==NULL)
		return -ENOMEM;

	strcpy(addrstring,cp);
	strcat(addrstring,addrbuf);

	return addrstring;
}

int  af_inet_p2p_setpeerexterninfo(void * peer,void * extern_info)
{
	struct connector_af_inet_p2p_peer_info * peer_info=peer;
	if(peer==NULL)
		return -EINVAL;	
	peer_info->extern_info=extern_info;
}
void * af_inet_p2p_getpeerexterninfo(void * peer)
{
	struct connector_af_inet_p2p_peer_info * peer_info=peer;
	if(peer==NULL)
		return NULL;	
	return peer_info->extern_info;
}

void * af_inet_p2p_getfirstpeer(void * conn)
{
	struct tcloud_connector * this_conn=conn;
	struct connector_af_inet_p2p_info * p2p_info;
	struct connector_af_inet_p2p_peer_info * peer_info=NULL;
	Record_List * record_elem;
	p2p_info=this_conn->conn_var_info;
	p2p_info->curr_peer=p2p_info->peer_list.list.next;
	if(p2p_info->curr_peer==&p2p_info->peer_list.list)
		return NULL;
	record_elem=p2p_info->curr_peer;
	peer_info=record_elem->record;
	
	return peer_info;
}

void * af_inet_p2p_getnextpeer(void * conn)
{
	struct tcloud_connector * this_conn=conn;
	struct connector_af_inet_p2p_info * p2p_info;
	struct connector_af_inet_p2p_peer_info * peer_info=NULL;
	Record_List * record_elem;

	p2p_info=this_conn->conn_var_info;
	if(p2p_info->curr_peer==p2p_info->peer_list.list.prev)
		return NULL;
	p2p_info->curr_peer=p2p_info->curr_peer->next;
	record_elem=p2p_info->curr_peer;
	peer_info=record_elem->record;

	return peer_info;
}

void * af_inet_p2p_getcurrpeer(void * conn)
{
	struct tcloud_connector * this_conn=conn;
	struct connector_af_inet_p2p_info * p2p_info;
	struct connector_af_inet_p2p_peer_info * peer_info=NULL;
	Record_List * record_elem;

	p2p_info=this_conn->conn_var_info;
	if(p2p_info->curr_peer==&p2p_info->peer_list.list)
		return NULL;
	record_elem=p2p_info->curr_peer;
	peer_info=record_elem->record;

	return peer_info;
}

int af_inet_p2p_islastpeer(void * recv_conn)
{
	struct tcloud_connector * this_conn=recv_conn;
	struct connector_af_inet_p2p_info * p2p_info;
	struct connector_af_inet_p2p_peer_info * peer_info=NULL;
	Record_List * record_elem;

	p2p_info=this_conn->conn_var_info;
	if(p2p_info->curr_peer==p2p_info->peer_list.list.prev)
		return 1;
	return 0;
}

int af_inet_p2p_read_refresh(void * recv_conn)
{
	struct tcloud_connector * this_conn=recv_conn;
	struct connector_af_inet_p2p_info * p2p_info;
	p2p_info=this_conn->conn_var_info;
	p2p_info->read_state=P2P_READ_CONN;
	return 0;
}
int af_inet_p2p_read_hold(void * recv_conn)
{
	struct tcloud_connector * this_conn=recv_conn;
	struct connector_af_inet_p2p_info * p2p_info;
	p2p_info=this_conn->conn_var_info;
	p2p_info->read_state=P2P_READ_BUF;
	return 0;
}
int af_inet_p2p_read_fin(void * recv_conn)
{
	struct tcloud_connector * this_conn=recv_conn;
	struct connector_af_inet_p2p_info * p2p_info;
	p2p_info=this_conn->conn_var_info;
	p2p_info->read_state=P2P_READ_BUF;
	return 0;
}

int af_inet_p2p_setcurrpeer(void * recv_conn,void * peer_info)
{
	struct tcloud_connector * this_conn=recv_conn;
	struct connector_af_inet_p2p_info * p2p_info;
	Record_List * record_elem;

	p2p_info=this_conn->conn_var_info;
	p2p_info->curr_peer=p2p_info->peer_list.list.next;
	while(p2p_info->curr_peer!=&p2p_info->peer_list.list)
	{	
		record_elem=p2p_info->curr_peer;
		if(record_elem->record==peer_info)
			return 0;
		p2p_info->curr_peer=p2p_info->curr_peer->next;
	}
	
	return -EINVAL;
}
void * af_inet_p2p_findpeer(void * recv_conn,void * from_addr,int from_len)
{
	struct tcloud_connector * this_conn=recv_conn;
	struct connector_af_inet_p2p_peer_info * peer_info;

	peer_info=af_inet_p2p_getfirstpeer(recv_conn);
	while(peer_info!=NULL)
	{
		if(from_len==peer_info->len_inet)
		{
			if(Memcmp(from_addr,&peer_info->adr_inet,from_len)==0)
				break;		
		}
		peer_info=af_inet_p2p_getnextpeer(recv_conn);
	}
	
	return peer_info;
}

void * af_inet_p2p_findpeerbyuuid(void * recv_conn,BYTE * uuid)
{
	struct tcloud_connector * this_conn=recv_conn;
	struct connector_af_inet_p2p_peer_info * peer_info;
	struct sysconn_peer_info * sys_peer_info;

	peer_info=af_inet_p2p_getfirstpeer(recv_conn);
	while(peer_info!=NULL)
	{
		sys_peer_info=af_inet_p2p_getpeerexterninfo(peer_info);
		if(sys_peer_info!=NULL)
		{
			if(Memcmp(sys_peer_info->uuid,uuid,DIGEST_SIZE)==0)
				break;		
		}
		peer_info=af_inet_p2p_getnextpeer(recv_conn);
	}
	
	return peer_info;
}
void * af_inet_p2p_addpeer(void * recv_conn,void * from_addr,int from_len)
{
	struct tcloud_connector * this_conn=recv_conn;
	struct connector_af_inet_p2p_peer_info * peer_info;
	struct connector_af_inet_p2p_info * p2p_info;
	Record_List * record_elem;

	peer_info=af_inet_p2p_findpeer(recv_conn,from_addr,from_len);
	if(peer_info!=NULL)
		return NULL;

	record_elem = Calloc(sizeof(Record_List));
	if(record_elem==NULL)
		return NULL;
	INIT_LIST_HEAD(&(record_elem->list));

	peer_info = Dalloc0(sizeof(*peer_info),this_conn);
	if(peer_info==NULL)
		return -EINVAL;
	peer_info->len_inet=from_len;
	Memcpy(&peer_info->adr_inet,from_addr,from_len);
	record_elem->record=peer_info;

	p2p_info=this_conn->conn_var_info;
	List_add_tail(&record_elem->list,&p2p_info->peer_list.list);
	p2p_info->curr_peer=p2p_info->peer_list.list.prev;
	
	return peer_info;
}

void * af_inet_p2p_delpeer(void * recv_conn,void * peer)
{
	struct tcloud_connector * this_conn=recv_conn;
	struct connector_af_inet_p2p_info * p2p_info;
	struct connector_af_inet_p2p_peer_info * peer_info;
	Record_List * record_elem;

	p2p_info=this_conn->conn_var_info;
	record_elem=p2p_info->peer_list.list.next;
	while(record_elem!=&p2p_info->peer_list.list)
	{
	
		if(record_elem->record==peer)
		{
			if(p2p_info->curr_peer==record_elem)
			{
				p2p_info->curr_peer=record_elem->list.next;
			}
			List_del(&record_elem->list);
			Free(record_elem);
			return peer;
		
		}
		record_elem=(Record_List *)record_elem->list.next;
	}

	return NULL;
}
int  connector_af_inet_info_init (void * connector,char * addr)
{

	// name format: X.X.X.X:port

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	char addrbuf[108];
	int len_inet;
	int retval;
	int i;
	int namelen;
	struct timeval timeout ={0,1000};   // we hope each read only delay 10 microsecond
	
	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_protocol!=AF_INET)
		return -EINVAL;
	if(this_conn->conn_type==CONN_CHANNEL)
		return 0;


	base_info=Dalloc(sizeof(struct connector_af_inet_info),this_conn);
	if(base_info==NULL)
		return -ENOMEM;
	Memset(base_info,0,sizeof(struct connector_af_inet_info));
	this_conn->conn_base_info=base_info;
	
	switch(this_conn->conn_type)
	{

		case CONN_SERVER:
		case CONN_CLIENT:
		{
    			this_conn->conn_fd  = socket(AF_INET,SOCK_STREAM,0);
			if(this_conn->conn_fd <0)
				return this_conn->conn_fd;

				int yes=1;
				if(setsockopt(this_conn->conn_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1)
				{
					//printf("setsockopt reuseaddr error!\n");
					return -EINVAL;
				}
			if(this_conn->conn_mode != 1)
			{
				if(setsockopt(this_conn->conn_fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==-1)
				{
					//printf("setsockopt timeout error!\n");
					return -EINVAL;
				}
			}
/*
* Form an AF_INET Address:
*/

			af_inet_formaddr(&base_info->adr_inet,&base_info->len_inet,addr);

/*
* Now bind the address to the socket:
*/
			if(this_conn->conn_type==CONN_SERVER)
			{
				retval = bind(this_conn->conn_fd,
					(struct sockaddr *)&(base_info->adr_inet),
					base_info->len_inet);
				if ( retval == -1 )
					return -ENONET;
			}
			break;
		}
		case CONN_P2P_BIND:	
		{
    			this_conn->conn_fd  = socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);
			if(this_conn->conn_fd <0)
				return this_conn->conn_fd;
			af_inet_formaddr(&base_info->adr_inet,&base_info->len_inet,addr);
			retval = bind(this_conn->conn_fd,(struct sockaddr *)&(base_info->adr_inet),base_info->len_inet);
			if(retval==-1)
				return -ENONET;
			if(setsockopt(this_conn->conn_fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==-1)
			{
				//printf("setsockopt timeout error!\n");
				return -EINVAL;
			}
			//setnonblocking(this_conn->conn_fd);
	
			break;
		}
		case CONN_P2P_RAND:	
		{
    			this_conn->conn_fd  = socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);
			if(this_conn->conn_fd <0)
				return this_conn->conn_fd;

			
//			af_inet_formaddr(&base_info->adr_inet,&base_info->len_inet,"0.0.0.0:1440");
//			retval = bind(this_conn->conn_fd,(struct sockaddr *)&(base_info->adr_inet),base_info->len_inet);
//			if(retval==-1)
//				return -ENONET;
			if(Strncmp(addr,"(null)",6)!=0)
			{
				struct connector_af_inet_p2p_peer_info * peer_info;
				struct connector_af_inet_p2p_info * rand_info;
				Record_List * record_elem;
				int ret;
    				int so_broadcast = 1;  

				rand_info=this_conn->conn_var_info;
				peer_info = Dalloc0(sizeof(*peer_info),this_conn);
				if(peer_info==NULL)
					return -EINVAL;
				ret=af_inet_formaddr(&peer_info->adr_inet,&peer_info->len_inet,addr);
				if(ret<0)
					return ret;
    				//默认的套接字描述符sock是不支持广播，必须设置套接字描述符以支持广播  
    				ret = setsockopt(this_conn->conn_fd, SOL_SOCKET, SO_BROADCAST, &so_broadcast,  
            				sizeof(so_broadcast));  

				record_elem = Calloc(sizeof(Record_List));
				if(record_elem==NULL)
					return NULL;
				INIT_LIST_HEAD(&(record_elem->list));
				record_elem->record=peer_info;
					
				List_add_tail(&record_elem->list,&rand_info->peer_list);
			//  RAND need to broadcast to find peer
				if(setsockopt(this_conn->conn_fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==-1)
				{
					//printf("setsockopt timeout error!\n");
					return -EINVAL;
				}
				//setnonblocking(this_conn->conn_fd);
			}
				
				
			break;
		}
		default:
			return -EINVAL;
	}

	return this_conn->conn_fd;
}


int  connector_af_inet_client_init (void * connector,char * name,char * addr)
{

	struct tcloud_connector * this_conn;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	this_conn->conn_name=Dalloc(strlen(name)+1,this_conn);
	if(this_conn->conn_name==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_name,name);

	this_conn->conn_addr=Dalloc(strlen(addr)+1,this_conn);
	if(this_conn->conn_addr==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_addr,addr);
	
	connector_af_inet_info_init (connector,addr);
	
	return 0;
};

int  connector_af_inet_p2p_bind_init (void * connector,char * name,char * addr)
{

	struct tcloud_connector * this_conn;
	int retval;
	struct connector_af_inet_p2p_info * bind_info;

	this_conn=(struct tcloud_connector *)connector;

	this_conn->conn_name=Dalloc(strlen(name)+1,this_conn);
	if(this_conn->conn_name==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_name,name);

	this_conn->conn_addr=Dalloc(strlen(addr)+1,this_conn);
	if(this_conn->conn_addr==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_addr,addr);
	

	bind_info=Dalloc0(sizeof(struct connector_af_inet_p2p_info),this_conn);
	if(bind_info==NULL)
		return -ENOMEM;

	INIT_LIST_HEAD(&(bind_info->peer_list.list));	
	bind_info->curr_peer=&(bind_info->peer_list.list);

	this_conn->conn_var_info=bind_info;
	connector_af_inet_info_init (connector,addr);
	
	return 0;
};

int  connector_af_inet_p2p_rand_init (void * connector,char * name,char * addr)
{

	struct tcloud_connector * this_conn;
	int retval;
	struct connector_af_inet_p2p_info * rand_info;
	struct connector_af_inet_p2p_peer_info * peer_info;

	this_conn=(struct tcloud_connector *)connector;

	this_conn->conn_name=Dalloc(strlen(name)+1,this_conn);
	if(this_conn->conn_name==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_name,name);

	this_conn->conn_addr=Dalloc(strlen(addr)+1,this_conn);
	if(this_conn->conn_addr==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_addr,addr);
	
	rand_info=Dalloc0(sizeof(struct connector_af_inet_p2p_info),this_conn);
	if(rand_info==NULL)
		return -ENOMEM;

	INIT_LIST_HEAD(&(rand_info->peer_list.list));	
	rand_info->curr_peer=&(rand_info->peer_list.list);

	this_conn->conn_var_info=rand_info;
	connector_af_inet_info_init (connector,addr);
	
	return 0;
};

int  connector_af_inet_server_init (void * connector,char * name,char * addr)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	struct connector_af_inet_server_info * server_info;

	int retval;

	this_conn=(struct tcloud_connector *)connector;

	this_conn->conn_name=Dalloc(strlen(name)+1,this_conn);
	if(this_conn->conn_name==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_name,name);
	
	this_conn->conn_addr=Dalloc(strlen(addr)+1,this_conn);
	if(this_conn->conn_addr==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_addr,addr);
	
	connector_af_inet_info_init (connector,addr);
	base_info=(struct connector_af_inet_info *)(this_conn->conn_base_info);

	server_info=Dalloc(sizeof(struct connector_af_inet_server_info),this_conn);
	if(server_info==NULL)
		return -ENOMEM;
	memset(server_info,0,sizeof(struct connector_af_inet_server_info));

	INIT_LIST_HEAD(&(server_info->channel_list.list));	
	server_info->curr_channel=&(server_info->channel_list.list);

	this_conn->conn_var_info=server_info;
	
	return 0;
}


int  connector_af_inet_listen (void * connector)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_type!=CONN_SERVER)
		return -EINVAL;
	if(this_conn->conn_protocol!=AF_INET)
		return -EINVAL;
	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	this_conn->conn_state=CONN_SERVER_LISTEN;
	return listen(this_conn->conn_fd,10);
}

int  connector_af_inet_p2p_listen (void * connector)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_type!=CONN_P2P_BIND)
		return -EINVAL;
	if(this_conn->conn_protocol!=AF_INET)
		return -EINVAL;
	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	return 0;
}

void * connector_af_inet_accept (void * connector)
{

	struct tcloud_connector * this_conn;
	struct tcloud_connector * channel_conn;
	struct connector_af_inet_info * base_info;
	struct connector_af_inet_server_info * server_info;
	struct connector_af_inet_info * channel_base_info;
	struct connector_af_inet_channel_info * channel_info;
	int retval;
	int accept_fd;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_type!=CONN_SERVER)
		return NULL;
	if(this_conn->conn_protocol!=AF_INET)
		return NULL;
	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	server_info=(struct connector_af_inet_server_info * )this_conn->conn_var_info;

	accept_fd= accept(this_conn->conn_fd,NULL,0);
	if(accept_fd<=0)
		return NULL;

	channel_conn=get_connector(CONN_CHANNEL,AF_INET);
	if(channel_conn==NULL)
		return NULL;
	channel_base_info = create_channel_af_inet_info(base_info);
	channel_conn->conn_fd=accept_fd;

	channel_info=Dalloc(sizeof(struct connector_af_inet_channel_info),channel_conn);
	if(channel_info==NULL)
		return NULL;
	channel_info->server=this_conn;
	channel_conn->conn_base_info=channel_base_info;
	channel_conn->conn_var_info=channel_info;

//      get channel's peer addr begin
	struct sockaddr_in peeraddr;
	int len_peeraddr=128;
	getpeername(accept_fd,&peeraddr,&len_peeraddr);

	channel_conn->conn_peeraddr=af_inet_getaddrstring(&peeraddr);

//      get channel's peer addr end

	struct List_head * head, *currlib;
	Record_List * record_elem;

	record_elem = Calloc(sizeof(Record_List));
	if(record_elem==NULL)
		return NULL;
	INIT_LIST_HEAD(&(record_elem->list));
	record_elem->record=channel_conn;

	server_info->channel_num++;
	channel_conn->conn_state=CONN_CHANNEL_ACCEPT;

	return channel_conn;
}

int connector_af_inet_close_channel(void * connector,void * channel)
{
	struct tcloud_connector * this_conn;
	struct tcloud_connector * channel_conn;
	struct connector_af_inet_info * base_info;
	struct connector_af_inet_server_info * server_info;
	struct connector_af_inet_info * channel_base_info;
	struct connector_af_inet_channel_info * channel_info;
	int retval;
	int accept_fd;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_type!=CONN_SERVER)
		return -EINVAL;
	if(this_conn->conn_protocol!=AF_INET)
		return -EINVAL;
	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	server_info=(struct connector_af_inet_server_info * )this_conn->conn_var_info;

	channel_conn= (struct tcloud_connector *)channel;

	channel_base_info = channel_conn->conn_base_info;

	struct List_head * head, *curr;
	Record_List * record_elem;
	Record_List * conn_list;


	conn_list=(Record_List *)&server_info->channel_list;
	head = &(conn_list->list);

	curr=head->next;
	while(curr!=head)
	{
		record_elem=(Record_List *)curr;
		if(record_elem->record ==channel)
			break;
		curr=curr->next;
	}

	if(curr==head)
		return -EINVAL;

	if(server_info->curr_channel==curr)
		server_info->curr_channel=curr->next;
	List_del(curr);
	free(record_elem);
	channel_conn->conn_state=CONN_CHANNEL_CLOSE;
	return 0;
}

int  connector_af_inet_connect (void * connector)
{
	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_type!=CONN_CLIENT)
		return -EINVAL;

	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	retval = connect(this_conn->conn_fd,&base_info->adr_inet,base_info->len_inet);
	if(retval<0)
		return retval;
	this_conn->conn_state=CONN_CLIENT_CONNECT;
	return retval;
}

int  connector_af_inet_p2p_rand_connect (void * connector)
{
	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	struct connector_af_inet_p2p_info * rand_info;
	struct connector_af_inet_p2p_peer_info * peer_info;
	int retval;
	Record_List * record_elem;
	char * conn_str="P2P_UDP_INIT"; 

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_type!=CONN_P2P_RAND)
		return -EINVAL;

	rand_info=(struct connector_af_inet_p2p_info * )this_conn->conn_var_info;
	if(rand_info==NULL)
		return -EINVAL;
	
	record_elem=rand_info->peer_list.list.next;
	if(record_elem ==NULL)
		return -EINVAL;
	peer_info=record_elem->record;
	
	retval = sendto(this_conn->conn_fd,conn_str,Strlen(conn_str),0,
		&peer_info->adr_inet,peer_info->len_inet);
	if(retval<0)
		return retval;
	return retval;
}

int  connector_af_inet_read (void * connector,void * buf, size_t count)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;

	retval= read(this_conn->conn_fd,buf,count);
	if(retval<0)
		return retval;
	if((this_conn->conn_type == CONN_CLIENT) &&(this_conn->conn_type == CONN_CLIENT_CONNECT))
		this_conn->conn_state=CONN_CLIENT_RESPONSE;
	return retval;

}

int  connector_af_inet_write (void * connector,void * buf,size_t count)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	return write(this_conn->conn_fd,buf,count);
}

int  connector_af_inet_wait (void * connector,struct timeval * timeout)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_protocol!=AF_INET)
		return -EINVAL;
	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	return 0;
}

int  connector_af_inet_disconnect (void * connector)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_protocol!=AF_INET)
		return -EINVAL;
	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	shutdown(this_conn->conn_fd,SHUT_RDWR);
	close (this_conn->conn_fd);

	this_conn->conn_state=0x1000;
	return 0;
}

void * connector_af_inet_get_server(void * connector)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_channel_info * channel_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;
	if(this_conn->conn_protocol!=AF_INET)
		return -EINVAL;
	if(this_conn->conn_type !=CONN_CHANNEL)
		return NULL;

	channel_info=(struct connector_af_inet_channel_info *)(this_conn->conn_var_info);
	return channel_info->server;

}


int  connector_af_inet_p2p_read (void * connector,void * buf, size_t count)
{
	struct tcloud_connector * this_conn;
	struct connector_af_inet_p2p_info * p2p_info;
	struct connector_af_inet_p2p_peer_info * peer_info;
	struct connector_af_inet_p2p_peer_info * curr_peer_info;
	int retval;
	struct sockaddr_in adr_inet;
	int len_inet;
	char buffer[MAX_P2P_SIZE];
	char * temp_buf;
	int i;
	int offset;
	Record_List * record_elem;
	int total_read;

	len_inet=sizeof(adr_inet);
	this_conn=(struct tcloud_connector *)connector;
	p2p_info=this_conn->conn_var_info;

	curr_peer_info=af_inet_p2p_getcurrpeer(this_conn);

	
	Memset(&adr_inet,0,sizeof(adr_inet));

	i=0;
	offset=0;
	
	total_read=0;
	if((p2p_info->read_state==0)||(p2p_info->read_state==P2P_READ_FIN))
	{
		p2p_info->read_state=P2P_READ_CONN;
	}
	if(p2p_info->read_state==P2P_READ_CONN)
	{
	
		while((retval=recvfrom(this_conn->conn_fd,buffer,MAX_P2P_SIZE,0,&adr_inet,&len_inet))>0)
		{
			print_cubeaudit("p2p read %d data !\n",retval);
			peer_info=af_inet_p2p_findpeer(this_conn,&adr_inet,len_inet);
			if(peer_info==NULL)
			{
				peer_info=af_inet_p2p_addpeer(this_conn,&adr_inet,len_inet);
			}
			if(peer_info->buf==NULL)
			{
				peer_info->buf=Dalloc0(retval,this_conn);
				if(peer_info->buf==NULL)
					return -ENOMEM;
				peer_info->buf_size=retval;
				Memcpy(peer_info->buf,buffer,peer_info->buf_size);
			}
			else
			{
				temp_buf=Dalloc0(peer_info->buf_size+retval,this_conn);
				if(temp_buf==NULL)
					return -ENOMEM;
				Memcpy(temp_buf,peer_info->buf,peer_info->buf_size);
				Memcpy(temp_buf+peer_info->buf_size,buffer,retval);
				Free(peer_info->buf);
				peer_info->buf=temp_buf;
				peer_info->buf_size+=retval;
			}
//			total_read+=retval;
		}
		af_inet_p2p_read_hold(this_conn);
	}	
	if(curr_peer_info==NULL)
	{
		curr_peer_info=af_inet_p2p_getfirstpeer(this_conn);
		if(curr_peer_info==NULL)
			return 0;
	}
	else
	{
		af_inet_p2p_setcurrpeer(this_conn,curr_peer_info);
	}
	peer_info=curr_peer_info;
	if((peer_info->buf_size<=count)&&(peer_info->buf_size>0))
	{
		retval=peer_info->buf_size;
		Memcpy(buf,peer_info->buf,peer_info->buf_size);	
		peer_info->buf_size=0;
		Free(peer_info->buf);
		peer_info->buf=NULL;		
		if(af_inet_p2p_islastpeer(this_conn))
			af_inet_p2p_read_fin(this_conn);
	}
	else if(peer_info->buf_size>count)
	{
		retval=count;
		Memcpy(buf,peer_info->buf,retval);
		temp_buf=Dalloc0(peer_info->buf_size-retval,this_conn);
		if(temp_buf==NULL)
			return -ENOMEM;
		peer_info->buf_size-=retval;
		Memcpy(temp_buf,peer_info->buf+count,peer_info->buf_size);
		Free(peer_info->buf);
		peer_info->buf=temp_buf;
	}
	else
	{
		retval=0;
	}
	return retval;
}

int  connector_af_inet_p2p_write (void * connector,void * buf,size_t count)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_p2p_info * p2p_info;
	struct connector_af_inet_p2p_peer_info * peer_info;
	int retval;
	Record_List * record_elem;

	this_conn=(struct tcloud_connector *)connector;

	p2p_info=(struct connector_af_inet_p2p_info * )this_conn->conn_var_info;
	record_elem=p2p_info->curr_peer;
	peer_info=record_elem->record;
	if(peer_info==NULL)
	{
		p2p_info->curr_peer=p2p_info->peer_list.list.next;
		peer_info=record_elem->record;
		if(peer_info==NULL)
			return -EINVAL;
	}

 	retval=sendto(this_conn->conn_fd,buf,count,0,&peer_info->adr_inet,peer_info->len_inet); 
	print_cubeaudit("p2p send %d data return value%d!\n",count,retval);
	return retval;
}

struct connector_ops connector_af_inet_server_ops = 
{
	.conn_type = CONN_SERVER,
	.init=connector_af_inet_server_init,
	.ioctl=NULL,	
	.getname=connector_getname,	
	.getaddr=connector_getaddr,	
	.getpeeraddr=connector_getpeeraddr,	
	.setname=connector_setname,	
	.listen=connector_af_inet_listen,	
	.accept=connector_af_inet_accept,	
//	.connect=connector_af_inet_connect,	
	.close_channel=connector_af_inet_close_channel,
	.read=connector_af_inet_read,	
	.write=connector_af_inet_write,	
	.getfd=connector_getfd,	
	.wait=connector_af_inet_wait,	
	.disconnect=connector_af_inet_disconnect,	
};

struct connector_ops connector_af_inet_client_ops = 
{
	.conn_type = CONN_CLIENT,
	.init=connector_af_inet_client_init,
	.ioctl=NULL,	
	.getname=connector_getname,	
	.getaddr=connector_getaddr,	
	.getpeeraddr=connector_getpeeraddr,	
	.setname=connector_setname,	
	.connect=connector_af_inet_connect,	
	.read=connector_af_inet_read,	
	.write=connector_af_inet_write,	
	.getfd=connector_getfd,	
	.wait=connector_af_inet_wait,	
	.disconnect=connector_af_inet_disconnect	
};

struct connector_ops connector_af_inet_channel_ops = 
{
	.conn_type = CONN_CHANNEL,
	.init=NULL,
	.ioctl=NULL,	
	.getname=connector_getname,	
	.getaddr=connector_getaddr,	
	.getpeeraddr=connector_getpeeraddr,	
	.setname=connector_setname,	
	.connect=connector_af_inet_connect,	
	.read=connector_af_inet_read,	
	.write=connector_af_inet_write,	
	.getfd=connector_getfd,	
	.wait=connector_af_inet_wait,	
	.getserver=connector_af_inet_get_server,
	.disconnect=connector_af_inet_disconnect	
};

struct connector_ops connector_af_inet_p2p_ops = 
{
	.conn_type = CONN_P2P_BIND,
	.init=connector_af_inet_p2p_bind_init,
	.ioctl=NULL,	
	.getname=connector_getname,	
	.getaddr=connector_getaddr,	
	.getpeeraddr=connector_getpeeraddr,	
	.setname=connector_setname,	
	.listen=connector_af_inet_p2p_listen,	
	.accept=NULL,	
//	.connect=connector_af_inet_connect,	
	.close_channel=connector_af_inet_close_channel,
	.read=connector_af_inet_p2p_read,	
	.write=connector_af_inet_p2p_write,	
	.getfd=connector_getfd,	
	.wait=connector_af_inet_wait,	
	.disconnect=connector_af_inet_disconnect,	
};

struct connector_ops connector_af_inet_p2p_cli_ops = 
{
	.conn_type = CONN_P2P_RAND,
	.init=connector_af_inet_p2p_rand_init,
	.ioctl=NULL,	
	.getname=connector_getname,	
	.getaddr=connector_getaddr,	
	.getpeeraddr=connector_getpeeraddr,	
	.setname=connector_setname,	
	.listen=NULL,	
	.accept=NULL,	
	.connect=connector_af_inet_p2p_rand_connect,	
	.close_channel=connector_af_inet_close_channel,
	.read=connector_af_inet_p2p_read,	
	.write=connector_af_inet_p2p_write,	
	.getfd=connector_getfd,	
	.wait=connector_af_inet_wait,	
	.disconnect=connector_af_inet_disconnect,	
};
