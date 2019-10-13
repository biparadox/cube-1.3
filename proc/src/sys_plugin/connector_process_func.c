#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>  
#include <netinet/in.h>  
#include <netdb.h>  
#include <sys/ioctl.h>  
#include <net/if.h>  
#include <sys/socket.h>
  
//#include <sys/un.h>

#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "basefunc.h"
#include "memdb.h"
#include "message.h"
#include "connector.h"
#include "ex_module.h"
#include "connector_value.h"
#include "connector_process_func.h"
#include "sys_func.h"
//#include "conn_struct.h"

struct  connector_config
{
	char name[DIGEST_SIZE*2];
	int  family;
	int  type;	
	char  *  address;
	int  port;
	int  attr;
}__attribute__((packed));

static NAME2VALUE connector_family_valuelist[] = 
{
	{"AF_INET",AF_INET},
	{"AF_UNIX",AF_UNIX},
	{NULL,0}
};

enum  conn_config_attr
{
	CONN_ATTR_DEFAULT=0x01,
	CONN_ATTR_STOP=0x8000,
};

static NAME2VALUE connector_attr_valuelist[] = 
{
	{"DEFAULT",CONN_ATTR_DEFAULT},
	{"STOP",CONN_ATTR_STOP},
	{NULL,0}
};
static struct struct_elem_attr connector_config_desc[] =
{
    {"name",CUBE_TYPE_STRING,DIGEST_SIZE*2,NULL,NULL},
    {"family",CUBE_TYPE_ENUM,sizeof(int),&connector_family_valuelist,NULL},
    {"type",CUBE_TYPE_ENUM,sizeof(int),&connector_type_valuelist,NULL},
    {"address",CUBE_TYPE_ESTRING,DIGEST_SIZE*2,NULL,NULL},
    {"port",CUBE_TYPE_INT,sizeof(int),NULL,NULL},
    {"attr",CUBE_TYPE_ENUM,sizeof(int),&connector_attr_valuelist,NULL},
    {NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};
/*
struct connect_ack
{
	char uuid[DIGEST_SIZE];    //client's uuid
	char * client_name;	     // this client's name
	char * client_process;       // this client's process
	char * client_addr;          // client's address
	char server_uuid[DIGEST_SIZE];  //server's uuid
	char * server_name;               //server's name
	char * service;
	char * server_addr;              // server's addr
	int flags;
	char nonce[DIGEST_SIZE];
} __attribute__((packed));

struct connect_syn
{
	char uuid[DIGEST_SIZE];
	char * server_name;
	char * service;
	char * server_addr;
	int  flags;
	char nonce[DIGEST_SIZE];
}__attribute__((packed));
*/
static void * default_conn=NULL;
 
static void * conn_cfg_template=NULL;

int message_read_from_conn(void ** message,void * conn)
{
	const int fixed_buf_size=4096;
	char readbuf[fixed_buf_size];
	void * message_box;
	MSG_HEAD * message_head;
	int offset=0;
	int ret;
	int retval;
	int flag;
	struct tcloud_connector * temp_conn=conn;
	int message_size;

	ret=message_read_from_src(message,conn,temp_conn->conn_ops->read);
	if(ret<=0)
		return ret;
	offset=ret;
	flag=message_get_flag(*message);
	if(!(flag&MSG_FLAG_CRYPT))
	{
		ret=message_load_record(*message);
		if(ret<0)
		{
			print_cubeerr("load record failed in message_read_from_conn! use bin format\n");
		}
	}

	ret=message_load_expand(*message);
	
	return offset;           // all the message's data are read
}

int message_send(void * message,void * conn)
{
	int retval;
	BYTE * blob;
	struct tcloud_connector * temp_conn=conn;
	int record_size=message_output_blob(message,&blob);	
	if(record_size<=0)
		return record_size;

	retval=temp_conn->conn_ops->write(temp_conn,blob,record_size);
	print_cubeaudit("send %d data to conn %s!\n",record_size,connector_getname(temp_conn));
	return retval;
}

#define MAX_LINE_LEN 1024
int read_conn_cfg_buffer(FILE * stream, char * buf, int size)
    /*  Read text data from config file,
     *  ignore the ^# line and remove the \n character
     *  stream: the config file stream
     *  buf: the buffer to store the cfg data
     *  size: read data size
     *
     *  return value: read data size,
     *  negative value if it has special error
     *  */
{
    long offset=0;
    long curr_offset;
    char buffer[MAX_LINE_LEN];
    char * retptr;
    int len;

    while(offset<size)
    {
        curr_offset=ftell(stream);
        retptr=fgets(buffer,MAX_LINE_LEN,stream);

        // end of the file
        if(retptr==NULL)
            break;
        len=strlen(buffer);
        if(len==0)
            break;
        // commet line
        if(buffer[0]=='#')
            continue;
        while((buffer[len-1]=='\r')||(buffer[len-1]=='\n'))
        {
            len--;
            if(len==0)
                continue;
            buffer[len]==0;
        }
        // this line is too long to read
        if(len>size)
            return -EINVAL;

        // out of the bound
        if(len+offset>size)
        {
            fseek(stream,curr_offset,SEEK_SET);
            break;
        }
        Memcpy(buf+offset,buffer,len);
        offset+=len;
    }
    return offset;
}

int read_one_connector(void ** connector,void * json_node)
{
    void * conn_cfg_node;
    void * temp_node;
    char buffer[1024];
    int ret;
    struct connector_config * temp_cfg;


    struct tcloud_connector * conn=NULL;


    if(json_node!=NULL)
    {
        temp_cfg=Talloc(sizeof(struct connector_config));
        ret=json_2_struct(json_node,temp_cfg,conn_cfg_template);
        if(ret<0)
            return -EINVAL;
	conn=get_connector(temp_cfg->type,temp_cfg->family);
	if(conn==NULL)
		return -EINVAL;


	switch(temp_cfg->family){
		case AF_INET:
			sprintf(buffer,"%s:%d",temp_cfg->address,temp_cfg->port);
			break;
		default:
			return -EINVAL;
	}

  	ret=conn->conn_ops->init(conn,temp_cfg->name,buffer);
	if(ret<0)
	{
		print_cubeerr("init conn %s failed!\n",temp_cfg->name);
      		return -EINVAL;
	}

    }

    // read the router policy
    // first,read the main router policy

    *connector=conn;
    if(temp_cfg->attr==CONN_ATTR_DEFAULT)
    {
	    if(default_conn!=NULL)
	    {
		    print_cubeerr("not unique default conn!\n");
		    return -EINVAL;
	    }
	    default_conn=conn;
    }
    return 0;
}

int connector_read_cfg(char * filename,void * hub)
{
    const int bufsize=4096;
    char buffer[bufsize];
    int read_offset;
    int solve_offset;
    int buffer_left=0;
    int conn_num=0;
    void * conn;
    int ret;
    void * root;
    struct tcloud_connector_hub * conn_hub=(struct tcloud_connector_hub *)hub;
    int i;

    FILE * fp = fopen(filename,"r");
    if(fp==NULL)
        return -EINVAL;

    do {

        // when the file reading is not finished, we should read new data to the buffer
        if(fp != NULL)
        {
            read_offset=read_conn_cfg_buffer(fp,buffer+buffer_left,bufsize-buffer_left);
            if(read_offset<0)
                return -EINVAL;
            else if(read_offset<bufsize-buffer_left)
            {
                fclose(fp);
                fp=NULL;
            }
        }
//      print_cubeaudit("conn %d is %s\n",conn_num+1,buffer);

        solve_offset=json_solve_str(&root,buffer);
        if(solve_offset<=0)
	{
		if(conn_num>0)
			return conn_num;
           	return -EINVAL;
	}

        ret=read_one_connector(&conn,root);

        if(ret<0)
            return -EINVAL;
        conn_num++;
	conn_hub->hub_ops->add_connector(conn_hub,conn,NULL);
        buffer_left=read_offset-solve_offset;
        if(buffer_left>0)
	{
            Memcpy(buffer,buffer+solve_offset,buffer_left);
	    buffer[buffer_left]=0;
	}
        else
        {
            if(fp==NULL)
                break;
        }
    }while(1);
    return conn_num;
}

struct connector_proc_pointer
{
	void * hub;
	void * default_local_conn;
	void * default_remote_conn;
};

int proc_conn_init(void * sub_proc,void * para)
{
	int ret;
        char * config_file ="./connector_config.cfg";
	struct connector_proc_pointer * sub_proc_pointer;
	struct conn_init_para * conn_init_para = (struct conn_init_para *)para;

        if(para!=NULL)
		config_file=para;

//	register_record_type("SYNI",connect_syn_desc);
//	register_record_type("ACKI",connect_ack_desc);
	struct tcloud_connector_hub * conn_hub;
 	conn_hub=get_connector_hub();


//	void * context;
//	ret=sec_subject_getcontext(sub_proc,&context);
//	if(ret<0)
//		return ret;
//    	conn_cfg_template=create_struct_template(&main_config_desc);
//	print_cubeaudit("main template create succeed!\n");
    	conn_cfg_template=create_struct_template(&connector_config_desc);
	sub_proc_pointer=Salloc(sizeof(struct connector_proc_pointer));
	if(sub_proc_pointer==NULL)
		return -ENOMEM;
	Memset(sub_proc_pointer,0,sizeof(struct connector_proc_pointer));
	sub_proc_pointer->hub=conn_hub;
	ret=ex_module_setpointer(sub_proc,sub_proc_pointer);
	if(ret<0)
		return ret;
	ret=connector_read_cfg(config_file,conn_hub);
	if(ret<0)
		return ret;
	print_cubeaudit("read %d connector!\n",ret);

	struct tcloud_connector * temp_conn;

	temp_conn=hub_get_first_connector(conn_hub);
	
	// start all the SERVER

	while(temp_conn!=NULL)
	{
		if(connector_get_type(temp_conn)==CONN_SERVER)
		{
  	 		ret=temp_conn->conn_ops->listen(temp_conn);
			if(ret<0)
			{
				print_cubeerr("conn server %s listen error!\n",connector_getname(temp_conn));
				return -EINVAL;
			}
			print_cubeaudit("conn server %s begin to listen!\n",connector_getname(temp_conn));

		}	
		else if(connector_get_type(temp_conn)==CONN_P2P_BIND)
		{
			ret=temp_conn->conn_ops->listen(temp_conn);
			if(ret<0)
			{
				print_cubeerr("conn p2p bind port %s bind error!\n",connector_getname(temp_conn));
				return -EINVAL;
			}
			print_cubeaudit("conn server %s begin to listen!\n",connector_getname(temp_conn));
	
		}
		temp_conn=hub_get_next_connector(conn_hub);
	}
	return 0;
}

int  conn_client_connect(void * hub)
{ 
	struct tcloud_connector * temp_conn;
	// start all the CLIENT
	if((hub==NULL) || IS_ERR(hub))
		return -EINVAL;
	int conn_num=0;
	int ret;

	struct tcloud_connector_hub * conn_hub = hub;
	
	temp_conn=hub_get_first_connector(hub);
	
	while(temp_conn!=NULL)
	{
		if(connector_getstate(temp_conn)!=CONN_CLIENT_CONNECT)
		{
			if(connector_get_type(temp_conn)==CONN_CLIENT)
			{
   				ret=temp_conn->conn_ops->connect(temp_conn);
				if(ret<0)
				{
					//print_cubeerr("client %s connect failed!\n",connector_getname(temp_conn));
				}
				else
				{
					print_cubeaudit("client %s connect succeed!\n",connector_getname(temp_conn));
					conn_num++;
				}
			}
			else if(connector_get_type(temp_conn)==CONN_P2P_RAND)
			{
   				ret=temp_conn->conn_ops->connect(temp_conn);
				if(ret<0)
				{
					//print_cubeerr("client %s connect failed!\n",connector_getname(temp_conn));
				}
				else
				{
					print_cubeaudit("client %s connect succeed!\n",connector_getname(temp_conn));
					conn_num++;
				}
			}
		}	
		temp_conn=hub_get_next_connector(hub);
	}
	return conn_num;
}
	

int proc_conn_start(void * sub_proc,void * para)
{
	int ret;
	int retval;
	void * message_box;
	MSG_HEAD * message_head;
	void * context;
	int i,j;
	struct tcloud_connector * temp_conn;
	struct tcloud_connector * recv_conn;
	struct tcloud_connector * send_conn;
	char local_uuid[DIGEST_SIZE*2+1];
	char proc_name[DIGEST_SIZE*2+1];
	struct connector_proc_pointer * sub_proc_pointer;
	
	ret=proc_share_data_getvalue("uuid",local_uuid);
	ret=proc_share_data_getvalue("proc_name",proc_name);


	struct timeval conn_val;
	conn_val.tv_sec=time_val.tv_sec;
	conn_val.tv_usec=time_val.tv_usec;

	sub_proc_pointer = ex_module_getpointer(sub_proc);
	if(sub_proc_pointer==NULL)
		return -EINVAL;

	int circle_count=0;
	const int conn_check_interval=100;

	struct tcloud_connector_hub * hub = sub_proc_pointer->hub;
	if((hub==NULL) || IS_ERR(hub))
		return -EINVAL;

	while(1)
	{

//		usleep(conn_val.tv_usec);

		if(circle_count++ % conn_check_interval ==0)
			 conn_client_connect(hub);
		// receive the remote message

		ret=hub->hub_ops->select(hub,&conn_val);
		usleep(conn_val.tv_usec);
		conn_val.tv_usec=time_val.tv_usec;
		if(ret>0) {

			do{
	
				recv_conn=hub->hub_ops->getactiveread(hub);
				if(recv_conn==NULL)
					break;

				if(connector_get_type(recv_conn)==CONN_SERVER)
				{
	
					struct tcloud_connector * channel_conn;
	
					channel_conn=recv_conn->conn_ops->accept(recv_conn);
					if(channel_conn==NULL)
					{
						print_cubeerr("error: server connector accept error %x!\n",channel_conn);
						continue;
					}
					connector_setstate(channel_conn,CONN_CHANNEL_ACCEPT);
					print_cubeaudit("create a new channel %x!\n",channel_conn);
 
					// build a server syn message with service name,uuid and proc_name
					message_box=build_server_syn_message("trust_server",local_uuid,proc_name);
					if((message_box == NULL) || IS_ERR(message_box))
					{
						print_cubeerr("local_server reply syn message error!\n");
						continue;
					}
			
					retval=message_send(message_box,channel_conn);
					if(retval<=0)
						continue;
					hub->hub_ops->add_connector(hub,channel_conn,NULL);
				}
				else if(connector_get_type(recv_conn)==CONN_CLIENT)
				{
					while((ret=message_read_from_conn(&message_box,recv_conn))>0)
					{
						print_cubeaudit("proc conn client receive %d data!\n",ret);

						
						message_head=message_get_head(message_box);

						if((message_head->record_type==DTYPE_MESSAGE)
							&&(message_head->record_subtype==SUBTYPE_CONN_SYNI))
						// do the handshake	
						{
							void * message=build_client_ack_message(message_box,local_uuid,proc_name,recv_conn);
							if((message == NULL) || IS_ERR(message))
								continue;
							send_conn=recv_conn;
							retval=message_send(message,send_conn);
							connector_setstate(send_conn,CONN_CLIENT_RESPONSE);
							print_cubeaudit("client %s send %d ack data to server !\n",connector_getname(send_conn),retval);
						
						}
						else
						{
							connector_inet_get_uuid(recv_conn,message_head->sender_uuid);	
						}
						ex_module_sendmsg(sub_proc,message_box);
					
						continue;		
					}

				}
				else if(connector_get_type(recv_conn)==CONN_CHANNEL)
				{	
					char * sender_uuid;
					char * sender_name;
					char * receiver_uuid;
					char * receiver_name;
					struct expand_data_forward * expand_forward;
			 		struct connect_proc_info * channel_extern_info;

					while(message_read_from_conn(&message_box,recv_conn)>0)
					{

						message_head=message_get_head(message_box);

						// first: finish the handshake
						if((message_head->record_type==DTYPE_MESSAGE)
							&&(message_head->record_subtype==SUBTYPE_CONN_ACKI))
						{
							ret=receive_local_client_ack(message_box,recv_conn,hub);
							ex_module_sendmsg(sub_proc,message_box);
							print_cubeaudit("channel set name %s!\n",connector_getname(recv_conn));
							continue;
						}
						// check if this message is for you or for others
						print_cubeaudit("channel receive (%d %d) message from conn %s!\n",message_head->record_type,
							message_head->record_subtype,connector_getname(recv_conn));
						connector_inet_get_uuid(recv_conn,message_head->sender_uuid);	
						ex_module_sendmsg(sub_proc,message_box);
						print_cubeaudit("client forward (%d %d) message to main proc!\n",message_head->record_type,
							message_head->record_subtype);
						continue;		

					}	
				}
				else if(connector_get_type(recv_conn)==CONN_P2P_BIND)
				{
	
					char *init_str="P2P_UDP_INIT";
					BYTE * head;
					BYTE * buffer;
    					struct sockaddr_in server_addr; //服务器端地址  
    					struct sockaddr_in from_addr; //客户端地址  
					int from_len;
					MSG_HEAD * msg_head;
					void * peer_info;
					int buffer_size;
		
					af_inet_p2p_read_refresh(recv_conn);

					peer_info=af_inet_p2p_getfirstpeer(recv_conn);
					do
					{
						buffer=Talloc0(4096);
					
						ret=recv_conn->conn_ops->read(recv_conn,buffer,4096);
						if(ret<0)
							return ret;
						if(ret==0)
							continue;
						buffer_size=ret;
						if(Strncmp(buffer,init_str,Strlen(init_str))==0)
						{
							message_box=build_server_syn_message("trust_server",local_uuid,proc_name);
							if((message_box == NULL) || IS_ERR(message_box))
							{
								print_cubeerr("local_server reply syn message error!\n");
								continue;
							}
			
							retval=message_send(message_box,recv_conn);
							if(retval<=0)
								continue;
						}
						else
						{
							int flag;
							int buf_off=0;
							buffer_size=ret;
							while(buffer_size>0)
							{

								ret=message_read_from_blob(&message_box,buffer+buf_off,buffer_size);
								if(ret<0)
									break;
								buf_off+=ret;
								buffer_size-=ret;
								flag=message_get_flag(message_box);
								if(!(flag&MSG_FLAG_CRYPT))
								{
									ret=message_load_record(message_box);
									if(ret<0)
									{
										print_cubeerr("load record failed in message_read_from_conn! use bin format\n");
									}
								}

								ret=message_load_expand(message_box);
	
								message_head=message_get_head(message_box);

							// first: finish the handshake
								if((message_head->record_type==DTYPE_MESSAGE)
									&&(message_head->record_subtype==SUBTYPE_CONN_ACKI))
								{
									ret=receive_local_peer_ack(message_box,recv_conn,hub);
									ex_module_sendmsg(sub_proc,message_box);
									print_cubeaudit("bind port set name %s!\n",connector_getname(recv_conn));
									continue;
								}
						// check if this message is for you or for others
								print_cubeaudit("bind port receive (%d %d) message from conn %s!\n",message_head->record_type,
								message_head->record_subtype,connector_getname(recv_conn));
								ex_module_sendmsg(sub_proc,message_box);
								print_cubeaudit("bind port forward (%d %d) message to main proc!\n",
									message_head->record_type,
									message_head->record_subtype);
								continue;		

							}
						}	
						
					}while((peer_info=af_inet_p2p_getnextpeer(recv_conn))!=NULL);

				}
				else if(connector_get_type(recv_conn)==CONN_P2P_RAND)
				{
	
					BYTE head[sizeof(MSG_HEAD)+2];
					BYTE * buffer;
    					struct sockaddr_in server_addr; //服务器端地址  
    					struct sockaddr_in from_addr; //客户端地址  
					int from_len=sizeof(from_addr);
					MSG_HEAD * msg_head;
					void * peer_info;

					af_inet_p2p_read_refresh(recv_conn);

					peer_info=af_inet_p2p_getfirstpeer(recv_conn);
					while(peer_info!=NULL)
					{
						while((ret=message_read_from_conn(&message_box,recv_conn))>0)
						{
							print_cubeaudit("proc conn rand port receive %d data!\n",ret);

						
							message_head=message_get_head(message_box);

							if((message_head->record_type==DTYPE_MESSAGE)
								&&(message_head->record_subtype==SUBTYPE_CONN_SYNI))
						// do the handshake	
							{
								void * message=build_peer_ack_message(message_box,local_uuid,proc_name,recv_conn);
								if((message == NULL) || IS_ERR(message))
									continue;
								send_conn=recv_conn;
								retval=message_send(message,send_conn);
								connector_setstate(send_conn,CONN_CLIENT_RESPONSE);
								print_cubeaudit("rand p2p port %s send %d ack data to server !\n",connector_getname(send_conn),retval);
						
							}	
							ex_module_sendmsg(sub_proc,message_box);
							continue;		
						}
						peer_info=af_inet_p2p_getnextpeer(recv_conn);
					}

				}
			}while(1);
		}

		// send message to the remote
		while(ex_module_recvmsg(sub_proc,&message_box)>=0)
		{
			if(message_box==NULL)
				break;
			message_head=message_get_head(message_box);
			if(message_head->flag & MSG_FLAG_LOCAL)
			{
				print_cubeerr("error local message in conn proc!\n");
				message_free(message_box);
				continue;
			}

			char buffer[DIGEST_SIZE*2+1];

			if(Isstrinuuid(message_head->receiver_uuid))
			{
				if(message_head->receiver_uuid[0]==':')
				{
					send_conn=hub_get_connector(sub_proc_pointer->hub,message_head->receiver_uuid+1);
					int type=connector_get_type(send_conn);
					if((type==CONN_P2P_BIND)||(type==CONN_P2P_RAND))
						af_inet_p2p_getfirstpeer(send_conn);
				}
				else
				{
					send_conn=hub_get_connector_byreceiver(sub_proc_pointer->hub,NULL,
						message_head->receiver_uuid,NULL);	
				}		
	
			}
			else
			{
				send_conn=hub_get_connector_bypeeruuid(sub_proc_pointer->hub,message_head->receiver_uuid);	
			}

			if(send_conn==NULL)
			{
				print_cubeerr("can't find target, send message to default connector!\n");
				send_conn=default_conn;
			}

			if(send_conn!=NULL)
			{
				ret=message_send(message_box,send_conn);
				print_cubeaudit("send (%d  %d)message %d to conn %s!\n",message_head->record_type,message_head->record_subtype,
					ret,connector_getname(send_conn));
			}
			else
				print_cubeerr("send (%d %d) message failed: no conn!\n",message_head->record_type,message_head->record_subtype);

		}	

	}


	return 0;
};

int proc_conn_accept(void * this_proc,void * msg,void * conn)
{

}

int proc_conn_sync(void * this_proc,void * msg,void * conn)
{

}

int proc_conn_acksend(void * this_proc,void * msg,void * conn)
{

}

int proc_conn_channelbuild(void * this_proc,void * msg,void * conn)
{

}

