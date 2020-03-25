#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "json.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "basefunc.h"
#include "memdb.h"
#include "message.h"
#include "channel.h"
#include "connector.h"
#include "ex_module.h"
#include "connector_value.h"
#include "sys_func.h"

#include "websocket_channel.h"

#define MAX_LINE_LEN 1024
#define BUFFER_SIZE 1024
#define SHA_DIGEST_LENGTH 20
#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

static char * ws_addr;
static int ws_port;
static struct tcloud_connector_hub * conn_hub;
static CHANNEL * websocket_channel;


pthread_t send_pthread;


static BYTE Buf[DIGEST_SIZE*64];
static int index = 0;
static BYTE * ReadBuf=Buf+DIGEST_SIZE*32;
static int readbuf_len;

static void * default_conn = NULL;
struct tcloud_connector * server_conn;

typedef struct _frame_head {
    char fin;
    char opcode;
    char mask;
    unsigned long long payload_length;
    char masking_key[4];
}frame_head;

struct send_channel_para
{
	CHANNEL * inner_channel;	
	TCLOUD_CONN * ws_conn;
	frame_head head;	
}send_para;

struct default_conn_index
{
	UINT16 conn_no;
};

struct default_channel_head
{
	UINT16 conn_no;
	int size;
}__attribute__((packed));

static int conn_count=0;
int shakehands(void * conn);
int websocket_send_func(void * pointer);

int _channel_set_recv_conn(BYTE * Buf,void * conn,int size)
//本函数用于改写模块者实现一个为连接设置属性的函数。缺省属性设置为连接接入的次序
{
	struct default_conn_index * conn_index;
	TCLOUD_CONN * set_conn=conn;
	if(conn==NULL)
		return -EINVAL;
	if(set_conn->conn_extern_info==NULL)
	{
		conn_index=Dalloc0(sizeof(*conn_index),set_conn);
		if(conn_index==NULL)
			return -EINVAL;
		set_conn->conn_extern_info=conn_index;
		conn_count++;
		conn_index->conn_no=conn_count;
	}
	else
		conn_index=(struct default_conn_index *)set_conn->conn_extern_info;
	if(Buf!=NULL)
	{
		struct default_channel_head * channel_head;
		channel_head=(struct default_channel_head *)Buf;
		channel_head->conn_no=conn_index->conn_no;
		channel_head->size=size;
		return size+sizeof(*channel_head);
	}
	return 0;
}


int _channel_get_send_conn(BYTE * Buf,int length,void **conn)
//本函数用于改写模块者实现一个根据缓冲区确定写连接对象和连接长度的函数，缺省设置为
//连接编号和写入数据长度
{
	struct default_channel_head * channel_head;
	struct default_conn_index * conn_index;
	if(length<sizeof(*channel_head))
		return 0;
	channel_head=(struct defaule_channel_head *)Buf;
	if(channel_head->size>length-sizeof(*channel_head))
		return 0;

	TCLOUD_CONN * temp_conn;
	temp_conn=hub_get_first_connector(conn_hub);
	while(temp_conn!=NULL)
	{
		conn_index=temp_conn->conn_extern_info;
		if(conn_index!=NULL)
		{
			if(conn_index->conn_no==channel_head->conn_no)
			{
				break;
			}
		}
		temp_conn=hub_get_next_connector(conn_hub);
	}
	
	*conn=temp_conn;
	
	return channel_head->size;
}



int websocket_channel_init(void * sub_proc,void * para)
{
    struct ws_init_para * init_para=para;
    int ret;

    conn_hub = get_connector_hub();

    if(conn_hub==NULL)
	return -EINVAL;
    server_conn	= get_connector(CONN_SERVER,AF_INET);
    if((server_conn ==NULL) & IS_ERR(server_conn))
    {
         printf("get conn failed!\n");
         return -EINVAL;
    }
 
    Strcpy(Buf,init_para->ws_addr);
    Strcat(Buf,":");
    Itoa(init_para->ws_port,Buf+Strlen(Buf));

//  server_conn->conn_mode=1;

    ret=server_conn->conn_ops->init(server_conn,"websocket_channer_server",Buf);
    if(ret<0)
	return ret;
    conn_hub->hub_ops->add_connector(conn_hub,server_conn,NULL);

    ret=server_conn->conn_ops->listen(server_conn);

    fprintf(stdout,"test server listen,return value is %d!\n",ret);

    websocket_channel=channel_register(init_para->channel_name,CHANNEL_RDWR,sub_proc);
    if(websocket_channel==NULL)
	return -EINVAL;

    send_para.inner_channel=websocket_channel;
    send_para.ws_conn=NULL;

    return 0;
}

int websocket_channel_start(void * sub_proc,void * para)
{
    int ret = 0, len = 0, i = 0, j = 0;
    int rc = 0;

    struct tcloud_connector *recv_conn;
    struct tcloud_connector *temp_conn;
    struct tcloud_connector * channel_conn=NULL;
    struct timeval conn_val;
    conn_val.tv_sec=time_val.tv_sec;
    conn_val.tv_usec=time_val.tv_usec;
    frame_head head;	

    ret=pthread_create(&(send_pthread),NULL,websocket_send_func,NULL);
    if(ret<0)
    {
	print_cubeerr("websocket_channel: create send pthread failed %d!\n",ret);
	return -EINVAL;
    }


    for (;;)
    {
	    ret = conn_hub->hub_ops->select(conn_hub, &conn_val);
	    usleep(conn_val.tv_usec);
	    conn_val.tv_usec = time_val.tv_usec;
	    if (ret > 0) {
		    do {
			    recv_conn = conn_hub->hub_ops->getactiveread(conn_hub);
			    if (recv_conn == NULL)
				    break;
			    if (connector_get_type(recv_conn) == CONN_SERVER)
			    {
				    char * peer_addr;
				    channel_conn = recv_conn->conn_ops->accept(recv_conn);
				    if(channel_conn == NULL)
				    {
					    printf("error: server connector accept error %p!\n", channel_conn);
					    continue;
				    }
				    connector_setstate(channel_conn, CONN_CHANNEL_ACCEPT);
				    printf("create a new channel %p!\n", channel_conn);
				    //		    channel_conn->conn_ops->write(channel_conn,"test",5);

				    conn_hub->hub_ops->add_connector(conn_hub, channel_conn, NULL);
				    // should add a start message
				    if(channel_conn->conn_ops->getpeeraddr!=NULL)
				    {
					    peer_addr=channel_conn->conn_ops->getpeeraddr(channel_conn);
					    if(peer_addr!=NULL)
						    printf("build channel to %s !\n",peer_addr);	
					    _channel_set_recv_conn(NULL,channel_conn,0);
				    }	
				    ret=shakehands(channel_conn);
				    if(ret<0)
					    return -EINVAL;				
				    send_para.ws_conn=channel_conn;

			    }
			
			    else if (connector_get_type(recv_conn) == CONN_CHANNEL)
			    {

				    rc=recv_frame_head(recv_conn,&head);
				    if(rc<0)
					    break;
				    memcpy(&send_para.head,&head,sizeof(head));
//				    printf("fin=%d\nopcode=0x%X\nmask=%d\npayload_len=%llu\n",head.fin,head.opcode,head.mask,head.payload_length);

				    do{
					    len=recv_conn->conn_ops->read(recv_conn,Buf,BUFFER_SIZE);
					    if(len>0)
					    {
						    ws_umask(Buf,len,head.masking_key);
						    //printf("read Data %s!\n",Buf);
						    ret=channel_inner_write(websocket_channel,Buf,len);	
							print_cubeaudit("websocket_channel write %d Data!",len);
					    }
					    if(ret<len)
					    {
						    printf(" read Buffer overflow!\n");
						    return -EINVAL;	
					    }	

				    }while(len>0);
			    }
		    } while (1);
	    }


    }
    return 0;
}


struct tcloud_connector * getConnectorByIp(struct tcloud_connector_hub *hub, char *ip)
{
	struct tcloud_connector * conn =  hub_get_first_connector(hub);

	// find Ip's conn
	while (conn != NULL)
	{
		if (connector_get_type(conn) == CONN_CHANNEL)
		{
			// printf("conn peeraddr %s\n", conn->conn_peeraddr);
			if (conn->conn_peeraddr != NULL && !Strncmp(conn->conn_peeraddr, ip, Strlen(ip)))
				return conn;
		}
		conn = hub_get_next_connector(hub);
	}
	return NULL;
}

/*
 * debug in use
 *
 */
void printAllConnect(struct tcloud_connector_hub *hub)
{
	printf("All Connector Channel\n");
	struct tcloud_connector * conn =  hub_get_first_connector(hub);

	// find Ip's conn
	while (conn != NULL)
	{
		if (connector_get_type(conn) == CONN_CHANNEL)
		{
			printf("conn peeraddr %s\n", conn->conn_peeraddr);
		}
		conn = hub_get_next_connector(hub);
	}
}

int _readline(char* allbuf,int level,char* linebuf)
{
	int len = strlen(allbuf);
	for (;level<len;++level)
	{
		if(allbuf[level]=='\r' && allbuf[level+1]=='\n')
			return level+2;
		else
			*(linebuf++) = allbuf[level];
	}
	return -1;
}



int shakehands(void * conn)
{
	//next line's point num
	int level = 0;
	//all request data
	char buffer[BUFFER_SIZE];
	//a line data
	char linebuf[256];
	//Sec-WebSocket-Accept
	char sec_accept[32];
	//sha1 data
	unsigned char sha1_data[SHA_DIGEST_LENGTH+1]={0};
	//reponse head buffer
	char head[BUFFER_SIZE] = {0};
	struct tcloud_connector *channel_conn=conn;
	int len;

	len = channel_conn->conn_ops->read(channel_conn, buffer,BUFFER_SIZE);
	if (len < 0) {
		perror("read error");
		conn_hub->hub_ops->del_connector(conn_hub, channel_conn);
		return -EINVAL;
	} 
	else if (len == 0) {
		perror("peer close\n");
		conn_hub->hub_ops->del_connector(conn_hub, channel_conn);
		return -EINVAL;
	}	
//	printf("request\n");
//	printf("%s\n",buffer);

	do {
		memset(linebuf,0,sizeof(linebuf));
		level = _readline(buffer,level,linebuf);
		//printf("line:%s\n",linebuf);

		if (strstr(linebuf,"Sec-WebSocket-Key")!=NULL)
		{
			strcat(linebuf,GUID);
			//          printf("key:%s\nlen=%d\n",linebuf+19,strlen(linebuf+19));
			calculate_context_sha1((unsigned char*)&linebuf+19,strlen(linebuf+19),&sha1_data);
			//          SHA1((unsigned char*)&linebuf+19,strlen(linebuf+19),(unsigned char*)&sha1_data);

			bin_to_radix64(sec_accept,SHA_DIGEST_LENGTH,sha1_data);
			strcat(sec_accept,"=");	
			//printf("radix64:%s\n",sec_accept);
			//          base64_encode(sha1_data,SHA_DIGEST_LENGTH,sec_accept);
			//          printf("base64:%s\n",sec_accept);
			/* write the response */
			sprintf(head, "HTTP/1.1 101 Switching Protocols\r\n" \
					"Upgrade: websocket\r\n" \
					"Connection: Upgrade\r\n" \
					"Sec-WebSocket-Accept: %s\r\n" \
					"\r\n",sec_accept);

			printf("response\n");
			//printf("%s",head);
			len=channel_conn->conn_ops->write(channel_conn,head,strlen(head));
			if(len<0)
			{
				perror("write");
			}
			break;
		}
	}while((buffer[level]!='\r' || buffer[level+1]!='\n') && level!=-1);
	return 0;
}

void ws_umask(char *data,int len,char *mask)
{
    int i;
    for (i=0;i<len;++i)
        *(data+i) ^= *(mask+(i%4));
}

int recv_frame_head(void * conn,frame_head* head)
{
	char one_char;
	int len;
	struct tcloud_connector *channel_conn=conn;
	/*read fin and op code*/
	len=channel_conn->conn_ops->read(channel_conn,&one_char,1);	
	if (len<0)
	{
		perror("read fin");
		return -1;
	}
	if(len==0)
	{
		return 0;
	}
	head->fin = (one_char & 0x80) == 0x80;
	head->opcode = one_char & 0x0F;
	/*read mask and payload*/
	len=channel_conn->conn_ops->read(channel_conn,&one_char,1);	
	if (len<=0)
	{
		perror("read mask");
		return -1;
	}
	head->mask = (one_char & 0x80) == 0X80;

	/*get payload length*/
	head->payload_length = one_char & 0x7F;

	if (head->payload_length == 126)
	{
		char extern_len[2];
		len=channel_conn->conn_ops->read(channel_conn,extern_len,2);	
		if (len<=0)
		{
			perror("read extern len");
			return -1;
		}
		head->payload_length = (extern_len[0]&0xFF) << 8 | (extern_len[1]&0xFF);
	}
	else if (head->payload_length == 127)
	{
		char extern_len[8],temp;
		int i;
		len=channel_conn->conn_ops->read(channel_conn,extern_len,8);	
		if (len<=0)
		{
			perror("read extern len");
			return -1;
		}
		for(i=0;i<4;i++)
		{
			temp = extern_len[i];
			extern_len[i] = extern_len[7-i];
			extern_len[7-i] = temp;
		}
		memcpy(&(head->payload_length),extern_len,8);
	}

	/*read masking-key*/
	len=channel_conn->conn_ops->read(channel_conn,head->masking_key,4);	
	if (len<=0)
	{
		perror("read masking-key");
		return -1;
	}
	return 0;
}

int send_frame_head(void * conn,frame_head* head)
{
	char *response_head;
	int head_length = 0;
	int len;
	struct tcloud_connector *channel_conn=conn;
	/*read fin and op code*/
    if(head->payload_length<126)
    {
        response_head = (char*)malloc(2);
        response_head[0] = 0x81;
        response_head[1] = head->payload_length;
        head_length = 2;
    }
    else if (head->payload_length<0xFFFF)
    {
        response_head = (char*)malloc(4);
        response_head[0] = 0x81;
        response_head[1] = 126;
        response_head[2] = (head->payload_length >> 8 & 0xFF);
        response_head[3] = (head->payload_length & 0xFF);
        head_length = 4;
    }
    else
    {
        //no code
        response_head = (char*)malloc(12);
//        response_head[0] = 0x81;
//        response_head[1] = 127;
//        response_head[2] = (head->payload_length >> 8 & 0xFF);
//        response_head[3] = (head->payload_length & 0xFF);
        head_length = 12;
    }
    len=channel_conn->conn_ops->write(channel_conn,response_head,head_length);	
    if (len<=0)
    {
        perror("write head");
        return -1;
    }
	print_cubeaudit("websocket_channel read %d Data!",len);

    free(response_head);
    return 0;
}

int websocket_send_func(void * pointer)
{
    frame_head head;	
    struct timeval conn_val;
    conn_val.tv_sec=time_val.tv_sec;
    conn_val.tv_usec=time_val.tv_usec;

    int len=0; 
    int ret;   

    while(1){
	 usleep(conn_val.tv_usec);
	 if(send_para.ws_conn==NULL)
		continue;
	 memcpy(&head,&send_para.head,sizeof(head));	 	

    len=channel_inner_read(send_para.inner_channel,ReadBuf,1024);
    if(len<0)
		return -EINVAL;
	 if(len >0)
	 {
		  print_cubeaudit("websocket_channel : Get %d data from websocekt_channel！",len);
	      head.payload_length=len;
	 //   printf("send fin=%d\nopcode=0x%X\nmask=%d\npayload_len=%llu\n",head.fin,head.opcode,head.mask,head.payload_length);
	      send_frame_head(send_para.ws_conn,&head);	
	      //printf("send Data %s!\n",ReadBuf);
	      ret=send_para.ws_conn->conn_ops->write(send_para.ws_conn,ReadBuf,len);
	      if(ret<len)
			return -EINVAL;
	      len=ret+sizeof(struct default_channel_head);
          }			
      }
      return 0;
}
