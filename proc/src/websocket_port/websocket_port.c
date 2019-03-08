#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include <libwebsockets.h>
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "basetype.h"
#include "message.h"
#include "ex_module.h"
#include "websocket_port.h"

static char * websocketserver_addr;
static int websocket_port;
extern struct timeval time_val={0,50*1000};

int isconnected=0;
struct websocket_server_context
{
       void * server_context;  //interface's hub
       void * callback_context;
       void * callback_interface;
       void * syn_template;
       void * connect_syn;
       char * websocket_message;
       int message_len;
       BYTE *read_buf;
       int  readlen;
       BYTE *write_buf;
       int  writelen;
};


static struct websocket_server_context * ws_context;  

struct connect_syn
{
	char uuid[DIGEST_SIZE];
	char * server_name;
	char * service;
	char * server_addr;
	int  flags;
	char nonce[DIGEST_SIZE];
}__attribute__((packed));

/*
static struct struct_elem_attr connect_syn_desc[]=
{
	{"uuid",OS210_TYPE_STRING,DIGEST_SIZE*2,NULL},
	{"server_name",OS210_TYPE_ESTRING,256,NULL},
	{"service",OS210_TYPE_ESTRING,64,NULL},
	{"server_addr",OS210_TYPE_ESTRING,256,NULL},
	{"flags",OS210_TYPE_INT,sizeof(UINT32),NULL},
	{"nonce",OS210_TYPE_STRING,DIGEST_SIZE,NULL},
	{NULL,OS210_TYPE_ENDDATA,0,NULL}
};
*/

static int callback_http(	
				struct lws * wsi,
				enum lws_callback_reasons reason,
				void * user,void * in,size_t len)
{
	return 0;
}
		
static int callback_cube_wsport(
				struct lws * wsi,
				enum lws_callback_reasons reason,
				void * user,void * in,size_t len)
{
	struct lws_context * this=lws_get_context(wsi);
	int i;
	switch(reason) {
		case LWS_CALLBACK_ESTABLISHED:
			ws_context->callback_interface=wsi;
			ws_context->callback_context=this;
			print_cubeaudit("connection established\n");
			BYTE * buf= (unsigned char *)malloc(
				LWS_SEND_BUFFER_PRE_PADDING+
				ws_context->message_len+
				LWS_SEND_BUFFER_POST_PADDING);
			if(buf==NULL)
				return -EINVAL;			
			Memcpy(&buf[LWS_SEND_BUFFER_PRE_PADDING],
				ws_context->websocket_message,
				ws_context->message_len);
			lws_write(wsi,
				&buf[LWS_SEND_BUFFER_PRE_PADDING],
				ws_context->message_len,LWS_WRITE_TEXT);
			free(buf);
			isconnected=1;
			break;
		case LWS_CALLBACK_RECEIVE:
		{
			ws_context->read_buf = (unsigned char *)malloc(len);
			if(ws_context->read_buf==NULL)
				return -EINVAL;
			ws_context->readlen=len;
			Memcpy(ws_context->read_buf,in,len);
			break;
		}
		default:
			break;
	}
	return 0;
}

static struct lws_protocols protocols[] = {
	{
		"http_only",
		callback_http,
		0	
	},
	{
		"cube-wsport",
		callback_cube_wsport,
		0
	},
	{
		NULL,NULL,0
	}
};

int websocket_port_init(void * sub_proc,void * para)
{

    struct websocket_init_para * init_para=para;

    int ret;
    struct lws_context * context;
    struct lws_context_creation_info info;
		

    struct message_box * msg_box;
    struct message_box * new_msg;

    MSG_HEAD * msg_head;

    char local_uuid[DIGEST_SIZE];
    char proc_name[DIGEST_SIZE];
 	
    if(init_para==NULL)
    {
	   print_cubeerr("can't find websocket port address!\n");
	   return -EINVAL;
    }	
    websocketserver_addr=dup_str(init_para->ws_addr,0);				
    websocket_port=init_para->ws_port;				


    ret=proc_share_data_getvalue("uuid",local_uuid);
    if(ret<0)
	    return ret;
    ret=proc_share_data_getvalue("proc_name",proc_name);

    ws_context=Talloc(sizeof(struct websocket_server_context));
    if(ws_context==NULL)
	return -ENOMEM;
    Memset(ws_context,0,sizeof(struct websocket_server_context));

    Memset(&info,0,sizeof(info));
    info.port=websocket_port;
//    info.iface=NULL;
    info.iface=websocketserver_addr;
    info.protocols=protocols;
  //info.extensions=lws_get_internal_extensions();
    info.extensions=NULL;
    info.ssl_cert_filepath=NULL;
    info.ssl_private_key_filepath=NULL;
    info.gid=-1;
    info.uid=-1;
    info.options=0;

    // parameter deal with
    char * server_name="websocket_server";
    char * server_uuid=local_uuid;
    char * service=ex_module_getname(sub_proc);
    char * server_addr=websocketserver_addr;
    char * nonce="AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

    struct connect_syn * syn_info;
    syn_info=Talloc(sizeof(struct connect_syn));
    if(syn_info==NULL)
        return -ENOMEM;
    Memset(syn_info,0,sizeof(struct connect_syn));

    char buffer[1024];
    Memset(buffer,0,1024);

    int stroffset=0;

    msg_box=build_server_syn_message(service,local_uuid,proc_name);  
    stroffset=message_output_json(msg_box,buffer);
    print_cubeaudit("websocket message size is %d\n",stroffset);
    print_cubeaudit("websocket message:%s\n",buffer);

    ret=json_2_message(buffer,&new_msg);
    if(ret<0)
        return -EINVAL;

    ws_context->websocket_message=dup_str(buffer,0);
    ws_context->message_len=strlen(buffer);

    context = lws_create_context(&info);
    if(context==NULL)
    {
	print_cubeerr(" wsport context create error!\n");
	return -EINVAL;
    }
    ws_context->server_context=context;

    return 0;
}

int websocket_port_start(void * sub_proc,void * para)
{
    int ret;
    int retval;
    void * message_box;
    void * context;
    int i;
    struct timeval conn_val;
    conn_val.tv_usec=time_val.tv_usec;

    char local_uuid[DIGEST_SIZE];
    char proc_name[DIGEST_SIZE];
    char buffer[4096];
    memset(buffer,0,4096);
    int stroffset;
    int str_size;
	
    print_cubeaudit("begin websocket server process!\n");
    ret=proc_share_data_getvalue("uuid",local_uuid);
    if(ret<0)
        return ret;
    ret=proc_share_data_getvalue("proc_name",proc_name);

    if(ret<0)
	return ret;

    print_cubeaudit("starting wsport server ...\n");

    while(1)
    {
	 lws_service(ws_context->server_context,50);
	 // check if there is something to read
	 if(ws_context->readlen>0)
	{
		int offset=0;
		do {
	    	 	void * message;
			MSG_HEAD * msg_head;
			ret=json_2_message(ws_context->read_buf+offset,&message);
		   	if(ret>=0)
		    	{
				str_size=ret;
				if(message_get_state(message)==0)
					message_set_state(message,MSG_FLOW_INIT);
				msg_head=message_get_head(message);
				if(Isstrinuuid(msg_head->nonce))
				{
					ret=get_random_uuid(msg_head->nonce);
					if(ret<0)
						break; 
				}	
				message_set_sender(message,local_uuid);
	    	    		ex_module_sendmsg(sub_proc,message);	
				offset+=str_size;
				if(ws_context->readlen-offset<sizeof(MSG_HEAD))
				{
					ws_context->readlen=0;
					break;
				}
		    	}
			else
			{
				print_cubeerr("resolve websocket message failed!\n");
				ws_context->readlen=0;
				break;
			}
		}while(1);
	}
	// send message to the remote

	if(isconnected==0)
		continue;

	while(ex_module_recvmsg(sub_proc,&message_box)>=0)
	{
		if(message_box==NULL)
			break;
    		stroffset=message_output_json(message_box,buffer+LWS_SEND_BUFFER_PRE_PADDING);
		if(stroffset>0)
			lws_write(ws_context->callback_interface,
				&buffer[LWS_SEND_BUFFER_PRE_PADDING],
				stroffset,LWS_WRITE_TEXT);

	}

    }
    return 0;
}
