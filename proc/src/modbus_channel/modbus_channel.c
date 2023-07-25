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
#include "return_value.h"
#include "zuc.h"

#include "modbus_channel.h"
#include "modbus_tcp.h"
#include "modbus_state.h"
#include "modbus_cmd.h"

#define MAX_LINE_LEN 1024

static int mode=0; //0 means server, 1 means client 
static char * tcp_addr;
static int tcp_port;
static struct tcloud_connector_hub * conn_hub;
static CHANNEL * modbus_channel;

static BYTE Buf[DIGEST_SIZE*64];
static int index = 0;
static BYTE * ReadBuf=Buf+DIGEST_SIZE*32;
static int readbuf_len;

static void * default_conn = NULL;
struct tcloud_connector * server_conn = NULL;
struct tcloud_connector * client_conn = NULL;
struct tcloud_connector * channel_conn = NULL;

struct tcloud_connector * getConnByUnitaddr(struct tcloud_connector_hub *hub, int unit_addr);

RECORD(MODBUS_STATE,SERVER) * server_index;
int init_addr=0x1000;

int ip_transfer(char * ipstr, BYTE *ip)
{
    int  ret;
    int offset=0;
    int i;
    BYTE IFS[5]={"...:"};
    for(i=0;IFS[i]!=0;i++)
    {
        ret=Getfiledfromstr(Buf,ipstr+offset,IFS[i],4);
        if(ret<0)
            return -1;
        offset+=ret+1;
        ip[i]=Atoi(Buf,4);
    }    
    return Atoi(ipstr+offset,5);
}

int _channel_set_recv_conn(void * sub_proc,void * conn,char * peer_addr)
//本函数用于Modbus Server端与Client端建立连接时,为Server端连接的Channel绑定一个channel_State
//该channel_state将记录连接对象的编号与状态
{
	RECORD(MODBUS_STATE,CHANNEL) * channel_index;
	RECORD(MODBUS_STATE,SERVER) * server_index;
	TCLOUD_CONN * set_conn=conn;
	if(conn==NULL)
		return -EINVAL;
    server_index = (RECORD(MODBUS_STATE,SERVER)*) ex_module_getpointer(sub_proc);
	if(set_conn->conn_extern_info==NULL)
	{
		channel_index=Dalloc0(sizeof(*channel_index),set_conn);
		if(channel_index==NULL)
			return -ENOMEM;
	    channel_index->state_machine=MODBUS_START;
        channel_index->port = ip_transfer(peer_addr,channel_index->client_ip);
        if(channel_index->port==0)
            return -EINVAL;
        Memcpy(channel_index->client_key,server_index->server_key,DIGEST_SIZE);
		set_conn->conn_extern_info=channel_index;
	}
	else
    {
		channel_index=(RECORD(MODBUS_STATE,CHANNEL)*)set_conn->conn_extern_info;
        Memset(channel_index,0,sizeof(*channel_index));
	    channel_index->state_machine=MODBUS_START;
        Memcpy(channel_index->client_key,server_index->server_key,DIGEST_SIZE);
    }    
	return 0;
}

int modbus_channel_init(void * sub_proc,void * para)
{
    struct init_para * init_para=para;
    int ret;
    if(init_para->mode != NULL)
    {   
        if((Strcmp(init_para->mode,"client")==0)
            || (Strcmp(init_para->mode,"CLIENT")==0)
            || (Strcmp(init_para->mode,"Client")==0))
            mode =1;
    }
    if(mode == 1)
        modbus_channel_client_init(sub_proc,para);
    else
        modbus_channel_server_init(sub_proc,para);
    return 0;
}

int modbus_channel_server_init(void * sub_proc,void * para)
{
    struct init_para * init_para=para;
    RECORD(MODBUS_STATE,SERVER) * server_index;
    int ret;

    conn_hub = get_connector_hub();

    if(conn_hub==NULL)
	    return -EINVAL;
    server_conn	= get_connector(CONN_SERVER,AF_INET);
    if((server_conn ==NULL) & IS_ERR(server_conn))
    {
         print_cubeerr("get conn failed!\n");
         return -EINVAL;
    }
 
    Strcpy(Buf,init_para->tcp_addr);
    Strcat(Buf,":");
    Itoa(init_para->tcp_port,Buf+Strlen(Buf));

    ret=server_conn->conn_ops->init(server_conn,"tcp_channel_server",Buf);
    if(ret<0)
	    return ret;
    conn_hub->hub_ops->add_connector(conn_hub,server_conn,NULL);

    ret=server_conn->conn_ops->listen(server_conn);
    fprintf(stdout,"test server listen,return value is %d!\n",ret);

    server_index = Dalloc0(sizeof(*server_index),0);
    server_index->unit_addr = init_para->unit_addr; 

    DB_RECORD * db_record;
    RECORD(GENERAL_RETURN,UUID) * channel_key;
    char addr_str[4];
    ret=Itoa(server_index->unit_addr,addr_str);
    if(ret<0)
        return ret;
    db_record = memdb_find_first(TYPE_PAIR(GENERAL_RETURN,UUID),"name",addr_str);
    if(db_record == NULL)
        return -EINVAL;
    channel_key=db_record->record;
    if(channel_key==NULL)
        return -EINVAL;
    Memcpy(server_index->server_key,channel_key->return_value,DIGEST_SIZE);
    
    ex_module_setpointer(sub_proc,server_index);
    return 0;
}

int modbus_channel_client_init(void * sub_proc,void * para)
{
    int ret;
    struct init_para * init_para=para;
    RECORD(MODBUS_STATE,CLIENT) * client_index;
    conn_hub = get_connector_hub();
    if(conn_hub==NULL)
	    return -EINVAL;

    client_conn	= get_connector(CONN_CLIENT,AF_INET);
    if((client_conn ==NULL) & IS_ERR(client_conn))
    {
         print_cubeerr("get conn failed!\n");
         return -EINVAL;
    }
 
    Strcpy(Buf,init_para->tcp_addr);
    Strcat(Buf,":");
    Itoa(init_para->tcp_port,Buf+Strlen(Buf));

    ret=client_conn->conn_ops->init(client_conn,"tcp_channel_client",Buf);
    if(ret<0)
	    return ret;
    conn_hub->hub_ops->add_connector(conn_hub,client_conn,NULL);

    ret=client_conn->conn_ops->connect(client_conn);
    client_index = Dalloc0(sizeof(*client_index),0);
    client_index->unit_addr=init_para->unit_addr;
    client_index->state_machine = MODBUS_START; 

    // get client key
    DB_RECORD * db_record;
    RECORD(GENERAL_RETURN,UUID) * channel_key;
    char addr_str[4];
    ret=Itoa(client_index->unit_addr,addr_str);
    if(ret<0)
        return ret;
    db_record = memdb_find_first(TYPE_PAIR(GENERAL_RETURN,UUID),"name",addr_str);
    if(db_record == NULL)
        return -EINVAL;
    channel_key=db_record->record;
    if(channel_key==NULL)
        return -EINVAL;
    Memcpy(client_index->client_key,channel_key->return_value,DIGEST_SIZE);
    
    ex_module_setpointer(sub_proc,client_index);
    return 0;
}

int tcp_channel_server_accept(void * sub_proc,TCLOUD_CONN * recv_conn)
{
	char * peer_addr;
    channel_conn = recv_conn->conn_ops->accept(recv_conn);
    if(channel_conn == NULL)
    {
        printf("error: server connector accept error %p!\n", channel_conn);
        return -EINVAL;
    }
    connector_setstate(channel_conn, CONN_CHANNEL_ACCEPT);
    print_cubeaudit("create a new channel %p!\n", channel_conn);
    conn_hub->hub_ops->add_connector(conn_hub, channel_conn, NULL);
	// should add a start message
    if(channel_conn->conn_ops->getpeeraddr!=NULL)
    {
        peer_addr=channel_conn->conn_ops->getpeeraddr(channel_conn);
		if(peer_addr!=NULL)
		    print_cubeaudit("build channel to %s !\n",peer_addr);	
	    _channel_set_recv_conn(sub_proc,channel_conn,peer_addr);
    }	
    return 0;
}

int modbus_server_read_message(BYTE * Buf, int len,void * sub_proc)
{
    int ret;
    int rc=0;
    int function_code;
    int unit_addr;
    void * send_msg;
    int data_len;


    function_code = (int)Buf[7];
    unit_addr=(int)Buf[6];

    send_msg=message_create(TYPE(MODBUS_CMD),function_code,NULL);
    if(send_msg==NULL)
        return -EINVAL;

    void * modbus_cmd;
    void * cmd_template;
    cmd_template = memdb_get_template(TYPE(MODBUS_CMD),function_code);
    if(cmd_template==NULL)
        return -EINVAL;

    ret = struct_size(cmd_template);
    if(ret<0)
        return  ret;

    modbus_cmd = Talloc0(ret);
    if(modbus_cmd==NULL)
        return -ENOMEM;
    
    data_len = blob_2_struct(Buf+8,modbus_cmd,cmd_template);
    if(data_len<0)
        return -EINVAL;
    if(data_len+2!=Buf[5])
        return -EINVAL;

    message_add_record(send_msg,modbus_cmd);
    ret = ex_module_sendmsg(sub_proc,send_msg);

    return ret;
}
int modbus_client_read_message(BYTE * Buf, int len,void * sub_proc)
{
    int ret;
    int rc=0;
    int function_code;
    int unit_addr;
    void * send_msg;
    int data_len;


    function_code = (int)Buf[7];
    unit_addr=(int)Buf[6];

    send_msg=message_create(TYPE(MODBUS_DATA),function_code,NULL);
    if(send_msg==NULL)
        return -EINVAL;

    void * modbus_data;
    void * data_template;
    data_template = memdb_get_template(TYPE(MODBUS_DATA),function_code);
    if(data_template==NULL)
        return -EINVAL;

    ret = struct_size(data_template);
    if(ret<0)
        return  ret;

    modbus_data = Talloc0(ret);
    if(modbus_data==NULL)
        return -ENOMEM;
    
    data_len = blob_2_struct(Buf+8,modbus_data,data_template);
    if(data_len<0)
        return -EINVAL;
    if(data_len+2!=Buf[5])
        return -EINVAL;

    message_add_record(send_msg,modbus_data);

    ret = ex_module_sendmsg(sub_proc,send_msg);

    return ret;
}

int modbus_channel_start(void * sub_proc,void * para)
{
    int ret = 0, len = 0, i = 0, j = 0;
    int rc = 0;
    void * recv_msg;
    int type;
    int subtype;

    struct tcloud_connector *recv_conn;
    struct tcloud_connector *temp_conn;
    struct timeval conn_val;
    conn_val.tv_sec=time_val.tv_sec;
    conn_val.tv_usec=time_val.tv_usec;

    if(mode==1) // client should send message to master
    {
           ret=proc_clientstate_send(sub_proc); 
           if(ret<0)
                return ret;
    }
    for (;;)
    {
        ret = conn_hub->hub_ops->select(conn_hub, &conn_val);
        usleep(conn_val.tv_usec);
    	conn_val.tv_usec = time_val.tv_usec;
        if (ret > 0) {
        do {
            // receive modbus data
                recv_conn = conn_hub->hub_ops->getactiveread(conn_hub);
                if (recv_conn == NULL)
                    break;
        	    usleep(conn_val.tv_usec);
                if (connector_get_type(recv_conn) == CONN_SERVER)
                {
                    // new modbus channel build
                    struct tcloud_connector * channel_conn;
		            char * peer_addr;
                    channel_conn = recv_conn->conn_ops->accept(recv_conn);
                    if(channel_conn == NULL)
                    {
                        printf("error: server connector accept error %p!\n", channel_conn);
                        continue;
                    }
                    connector_setstate(channel_conn, CONN_CHANNEL_ACCEPT);
                    printf("create a new channel %p!\n", channel_conn);

                    conn_hub->hub_ops->add_connector(conn_hub, channel_conn, NULL);
		            // should add a start message
                    //channel_conn->conn_ops->write(channel_conn,"hello",5);
		            if(channel_conn->conn_ops->getpeeraddr!=NULL)
		            {
			            peer_addr=channel_conn->conn_ops->getpeeraddr(channel_conn);
			            if(peer_addr!=NULL)
				            printf("build channel to %s !\n",peer_addr);	
			            _channel_set_recv_conn(sub_proc,channel_conn,peer_addr);
                    }	
                }
                else if (connector_get_type(recv_conn) == CONN_CHANNEL)
                {
                    // channel receive data from client
                    rc = 0;
                    len = recv_conn->conn_ops->read(recv_conn, Buf,256);
                    if (len < 0) {
                        perror("read error");
                        //conn_hub->hub_ops->del_connector(conn_hub, recv_conn);
                    } else if (len == 0) {
                        printf("peer close\n");
                        conn_hub->hub_ops->del_connector(conn_hub, recv_conn);
                    } 
 		            else
		            {
                        RECORD(MODBUS_STATE,CHANNEL) * channel_index=recv_conn->conn_extern_info;
                        if(channel_index->unit_addr == 0)
                            channel_index->unit_addr=Buf[6];

                        channel_index->no = *((UINT16 *)Buf);
			            modbus_server_read_message(Buf,len,sub_proc);
                    }
                }
                else if (connector_get_type(recv_conn) == CONN_CLIENT)
                {
                    rc = 0;
                    len = recv_conn->conn_ops->read(recv_conn, Buf,256);
                    if (len < 0) {
                        perror("read error");
                        //conn_hub->hub_ops->del_connector(conn_hub, recv_conn);
                    } else if (len == 0) {
                        printf("peer close\n");
                        conn_hub->hub_ops->del_connector(conn_hub, recv_conn);
                    } 
 		            else
		            {
			            modbus_client_read_message(Buf,len,sub_proc);
                   }
                }
            } while (1);
        }

        // if receive modbus message, we should send modbus message
        ret=ex_module_recvmsg(sub_proc,&recv_msg); 
        if(ret<0)
            continue;
        if(recv_msg == NULL)
            continue;
        type=message_get_type(recv_msg);
        subtype=message_get_subtype(recv_msg);
        if(!memdb_find_recordtype(type,subtype))
        {
            printf("message format (%d %d) is not registered!\n",
                message_get_type(recv_msg),message_get_subtype(recv_msg));
            continue;
        }

        if((type==TYPE(MODBUS_STATE))&&(subtype==SUBTYPE(MODBUS_STATE,SLAVE)))
        {
           ret=proc_serverstate_set(sub_proc,recv_msg); 
        }
        else if((type==TYPE(MODBUS_CMD)))
        {
           ret=proc_send_modbus_cmd(sub_proc,recv_msg); 
        }
        else if((type==TYPE(MODBUS_DATA)))
        {
           ret=proc_send_modbus_data(sub_proc,recv_msg,subtype); 
        }
	} 
    return 0;
}


int proc_clientstate_send(void * sub_proc)
{
    int ret = 0;
    RECORD(MODBUS_STATE,CLIENT) * client_index;
    RECORD(MODBUS_STATE,SLAVE) * slave_index;
    void * send_msg;
    client_index = (RECORD(MODBUS_STATE,SLAVE)*) ex_module_getpointer(sub_proc);
    if(client_index == NULL)
        return -EINVAL;
    slave_index = Talloc0(sizeof(*slave_index));
    if(slave_index==NULL)
        return -ENOMEM;

    Strncpy(slave_index->slave_name,client_index->server_name,DIGEST_SIZE);
    slave_index->unit_addr=client_index->unit_addr;

    send_msg = message_create(TYPE_PAIR(MODBUS_STATE,SLAVE),NULL);
    if(send_msg==NULL)
        return -EINVAL;
    ret=message_add_record(send_msg,slave_index);
    if(ret<0)
        return -EINVAL;
    ret=ex_module_sendmsg(sub_proc,send_msg);
    return ret;
}

int proc_serverstate_set(void * sub_proc,void * recv_msg)
{
    int ret;
    RECORD(MODBUS_STATE,SLAVE) * slave_index;
    RECORD(MODBUS_STATE,SERVER) * server_index;
    void * send_msg;

    ret=message_get_record(recv_msg,&slave_index,0);
    if(ret<0)
        return ret;

    server_index=ex_module_getpointer(sub_proc);
    if(server_index == NULL)
        return -EINVAL;
    if(server_index->server_name[0]==0)
        Memcpy(server_index->server_name,slave_index->slave_name,DIGEST_SIZE);
    if(server_index->unit_addr ==0)
        server_index->unit_addr=slave_index->unit_addr;
    if(Memcmp(server_index->server_key,EMPTY_UUID,DIGEST_SIZE) != 0)
            Memcpy(server_index->server_key,slave_index->slave_key,DIGEST_SIZE);
    ex_module_setpointer(sub_proc,server_index);
    send_msg=message_create(TYPE_PAIR(MODBUS_STATE,SERVER),recv_msg);
    if(send_msg==NULL)
        return -EINVAL;
    ret=message_add_record(send_msg,server_index);
    ret=ex_module_sendmsg(sub_proc,send_msg);
    return ret;

}

int proc_send_modbus_cmd(void * sub_proc, void * recv_msg)
{
    int ret;
    int len;
    int function_code;
    RECORD(MODBUS_TCP,MBAP) * mbap;
    RECORD(MODBUS_TCP,DATAGRAM) * datagram;
    RECORD(MODBUS_STATE,CLIENT) * client_index;
    BYTE databuf[256];
    int data_len;
    if(mode!=1)
    {
        print_cubeerr("modbus_channel:not master,shouldn't send cmd!");
        return -EINVAL;
    }
    
    client_index=ex_module_getpointer(sub_proc);
    function_code = message_get_subtype(recv_msg);
    *(UINT16 *)databuf = client_index->no++;
    databuf[2]=0;
    databuf[3]=0;
    databuf[4]=0;
    databuf[6]=client_index->unit_addr;

    void * cmd_template;

    cmd_template = memdb_get_template(TYPE(MODBUS_CMD),function_code);
    if(cmd_template == NULL)
        return -EINVAL;

    void * modbus_cmd;
    ret=message_get_record(recv_msg,&modbus_cmd,0);
    if(ret<0)
        return ret;
    data_len=struct_2_blob(modbus_cmd,databuf+8,cmd_template);
    databuf[5]=data_len+2;
    databuf[7]=function_code;

    //printf(" client key is:\n");
    //print_bin_data(client_index->client_key,DIGEST_SIZE,32);

	ret=client_conn->conn_ops->write(client_conn,databuf,8+data_len);
    return ret;
}


int proc_send_modbus_data(void * sub_proc, void * recv_msg)
{
    int ret;
    int len;
    int function_code;
    RECORD(MODBUS_TCP,MBAP) * mbap;
    RECORD(MODBUS_TCP,DATAGRAM) * datagram;
    BYTE databuf[256];
    int data_len;
    struct tcloud_connector *temp_conn;
	RECORD(MODBUS_STATE,CHANNEL) * channel_index;

    if(mode!=0)
    {
        print_cubeerr("modbus_channel:not slave,shouldn't response data!");
        return -EINVAL;
    }
    
    server_index=ex_module_getpointer(sub_proc);
    function_code = message_get_subtype(recv_msg);
    databuf[0]=0;
    databuf[1]=0;
    databuf[2]=0;
    databuf[3]=0;
    databuf[4]=0;
    databuf[6]=server_index->unit_addr;

    void * data_template;

    data_template = memdb_get_template(TYPE(MODBUS_DATA),function_code);
    if(data_template == NULL)
        return -EINVAL;

    void * modbus_data;
    ret=message_get_record(recv_msg,&modbus_data,0);
    if(ret<0)
        return ret;
    data_len=struct_2_blob(modbus_data,databuf+8,data_template);
    databuf[5]=data_len+2;
    databuf[7]=function_code;

    temp_conn=getConnByUnitaddr(conn_hub,server_index->unit_addr);

    channel_index=(RECORD(MODBUS_STATE,CHANNEL) *)temp_conn->conn_extern_info;

    *((UINT16 *)databuf)= channel_index->no;

    //printf(" channel key is:\n");
    //print_bin_data(channel_index->client_key,DIGEST_SIZE,32);

	ret=temp_conn->conn_ops->write(temp_conn,databuf,8+data_len);
    //printf(" send data: channel conn is %p\n",temp_conn);
    return ret;

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

struct tcloud_connector * getConnByUnitaddr(struct tcloud_connector_hub *hub, int unit_addr)
{
    struct tcloud_connector * conn =  hub_get_first_connector(hub);
	RECORD(MODBUS_STATE,CHANNEL) * channel_index;

    // find Ip's conn
    while (conn != NULL)
    {
        if (connector_get_type(conn) == CONN_CHANNEL)
        {
            // printf("conn peeraddr %s\n", conn->conn_peeraddr);
            channel_index=(RECORD(MODBUS_STATE,CHANNEL) *)conn->conn_extern_info;
            if(channel_index!=NULL)
            {
                if(channel_index->unit_addr == unit_addr)
                    return conn;
            }
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
