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
#include "sys_func.h"

#include "file_struct.h"
#include "file_channel.h"

#define MAX_LINE_LEN 1024

static char * tcp_addr;
static int tcp_port;
int    tcp_type=0;   // 0 means server, 1 means client
static struct tcloud_connector_hub * conn_hub;
static BYTE Buf[DIGEST_SIZE*64];
static int index = 0;
static BYTE * ReadBuf=Buf+DIGEST_SIZE*32;
static int readbuf_len;
int fd=0;
int file_count=0;
int block_size=512;
unsigned int left_size=0;
BYTE Blob[2048];

static void * default_conn = NULL;
struct tcloud_connector * server_conn;

struct tcloud_connector * client_conn;

struct tcloud_connector * channel_conn;

int proc_file_send_start(void * recv_msg);

int proc_file_receive_start(struct tcloud_connector * recv_conn);

int proc_file_send(struct tcloud_connector * send_conn);

int proc_file_receive(void * sub_proc,struct tcloud_connector * recv_conn);

struct policyfile_data * pfdata;
void * pfdata_template;

/*
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
*/


int file_channel_init(void * sub_proc,void * para)
{
    struct tcp_init_para * init_para=para;
    int ret;

    conn_hub = get_connector_hub();

    if(conn_hub==NULL)
	return -EINVAL;
	
    if(!Strcmp(init_para->tcp_type,"server") || 
	!Strcmp(init_para->tcp_type,"SERVER"))
    {
	tcp_type=0;
    }
    else if(!Strcmp(init_para->tcp_type,"client") || 
	!Strcmp(init_para->tcp_type,"CLIENT"))
    { 
	tcp_type=1;
    } 
    else	
	return -EINVAL;

    if(tcp_type== 0 )
    {
    	server_conn	= get_connector(CONN_SERVER,AF_INET);
    	if((server_conn ==NULL) & IS_ERR(server_conn))
    	{
       	       printf("get conn failed!\n");
               return -EINVAL;
        }
 
    	Strcpy(Buf,init_para->tcp_addr);
    	Strcat(Buf,":");
    	Itoa(init_para->tcp_port,Buf+Strlen(Buf));

    	ret=server_conn->conn_ops->init(server_conn,"file_channel_server",Buf);
    	if(ret<0)
		return ret;
        conn_hub->hub_ops->add_connector(conn_hub,server_conn,NULL);

        ret=server_conn->conn_ops->listen(server_conn);
        fprintf(stdout,"test server listen,return value is %d!\n",ret);
    }
    else if(tcp_type==1)
    {
    	client_conn	= get_connector(CONN_CLIENT,AF_INET);
    	if((client_conn ==NULL) & IS_ERR(client_conn))
    	{
       	       printf("get client conn failed!\n");
               return -EINVAL;
        }
 
    	Strcpy(Buf,init_para->tcp_addr);
    	Strcat(Buf,":");
    	Itoa(init_para->tcp_port,Buf+Strlen(Buf));

    	ret=client_conn->conn_ops->init(client_conn,"file_channel_client",Buf);
    	if(ret<0)
		return ret;
        conn_hub->hub_ops->add_connector(conn_hub,client_conn,NULL);

        fprintf(stdout,"test client generate %d!\n",ret);

    }
    else
	return -EINVAL;

    
    return 0;
}

int file_channel_start(void * sub_proc,void * para)
{
    int ret = 0, len = 0, i = 0, j = 0;
    int rc = 0;

    int state = -1;        // -1 not connect 0 wait  1 send file 2 receive file 3 error

    struct tcloud_connector *recv_conn;
    struct tcloud_connector *temp_conn;
    struct timeval conn_val;
    conn_val.tv_sec=time_val.tv_sec;
    conn_val.tv_usec=time_val.tv_usec;

    void * recv_msg;

    


    for (;;)
    {
        usleep(conn_val.tv_usec);
    	if((state==-1) && (tcp_type == 1))
        {
	    client_conn->conn_ops->connect(client_conn);
	    state=0;
	    continue;
        }
		
	if((state==-1) &&(tcp_type==0))
	{
		// build channel and link it
        	ret = conn_hub->hub_ops->select(conn_hub, &conn_val);
    		conn_val.tv_usec = time_val.tv_usec;
		if(ret<=0)
			continue;
               	recv_conn = conn_hub->hub_ops->getactiveread(conn_hub);
               	if (recv_conn == NULL)
                    	continue;
		char * peer_addr;

                channel_conn = recv_conn->conn_ops->accept(recv_conn);
                if(channel_conn == NULL)
                {
                        printf("error: server connector accept error %p!\n", channel_conn);
                        continue;
                }
                connector_setstate(channel_conn, CONN_CHANNEL_ACCEPT);
                conn_hub->hub_ops->add_connector(conn_hub, channel_conn, NULL);
		    // should add a start message
		if(channel_conn->conn_ops->getpeeraddr!=NULL)
		{
			peer_addr=channel_conn->conn_ops->getpeeraddr(channel_conn);
			if(peer_addr!=NULL)
				printf("build channel to %s !\n",peer_addr);	
                }
		state=0;
         } 	
		
	if(state==0)
	{
		if(tcp_type==1)
		{
			ret=ex_module_recvmsg(sub_proc,&recv_msg);
			if(ret<0)
				continue;
			if(recv_msg==NULL)
				continue;
			int type,subtype;
			type=message_get_type(recv_msg);
			subtype=message_get_subtype(recv_msg);
			if((type==TYPE(FILE_TRANS))&&(subtype==SUBTYPE(FILE_TRANS,REQUEST)))
			{
				state=1;
				file_count=0;
			
				ret=proc_file_send_start(recv_msg);
				if(ret<0)
					state=0;
			}
		}
		else if(tcp_type==0)
		{
        		ret = conn_hub->hub_ops->select(conn_hub, &conn_val);
    			conn_val.tv_usec = time_val.tv_usec;
			if(ret<=0)
				continue;
               		recv_conn = conn_hub->hub_ops->getactiveread(conn_hub);
               		if (recv_conn == NULL)
                    		continue;
			if(recv_conn==channel_conn)
				state=1;
			ret=proc_file_receive_start(channel_conn);
			if(ret==0)
				state=0;
		}
	}
		
        if(state==1)
	{
		if(tcp_type==1)
		{
			
			ret=proc_file_send(client_conn);
			if(ret==0)
				state=0;
		}
		else if(tcp_type==0)
		{
        		ret = conn_hub->hub_ops->select(conn_hub, &conn_val);
    			conn_val.tv_usec = time_val.tv_usec;
			if(ret<=0)
				continue;
               		recv_conn = conn_hub->hub_ops->getactiveread(conn_hub);
               		if (recv_conn == NULL)
                    		continue;
			if(recv_conn==channel_conn)
				state=1;
			ret=proc_file_receive(sub_proc,channel_conn);
			if(ret==0)
				state=0;
		}
		
	}
	    
    }
    return 0;
}

int proc_file_send_start(void * recv_msg)
{
	struct policyfile_req  * reqdata;
	int ret;
	int data_size;
	int total_size;
	BYTE digest[DIGEST_SIZE];
        struct stat statbuf;
	void * send_msg;
	print_cubeaudit("begin file send!\n");

	int record_no=0;

	ret=message_get_record(recv_msg,&reqdata,record_no);
	if(ret<0)
		return -EINVAL;
	if(reqdata!=NULL)
	{
		if(reqdata->filename!=NULL)
		{
			fd=open(reqdata->filename,O_RDONLY);
			if(fd<0)
				return fd;
        		if(fstat(fd,&statbuf)<0)
        		{
                		print_cubeerr("fstat error\n");
                		return -EINVAL;
        		}
        		total_size=statbuf.st_size;
			close(fd);

			calculate_sm3(reqdata->filename,digest);

			fd=open(reqdata->filename,O_RDONLY);
			if(fd<0)
				return fd;
			file_count=0;
		
        		pfdata=Dalloc0(sizeof(struct policyfile_data),0);
			if(pfdata==NULL)
			{
				return -ENOMEM;
			}
        		Memcpy(pfdata->uuid,digest,DIGEST_SIZE);
        		pfdata->filename=dup_str(reqdata->filename,0);
        		pfdata->record_no=file_count;
        		pfdata->offset=file_count*block_size;
        		pfdata->total_size=total_size;
        		pfdata->data_size=block_size;
			pfdata->file_data=NULL;

		}
	}
	printf("\n");
	return 0;	
}

int proc_file_send(struct tcloud_connector * send_conn)
{
	int ret;
	int isend;
	int package_size;

	
	void * pfdata_template = memdb_get_template(TYPE_PAIR(FILE_TRANS,FILE_DATA));
	if(pfdata_template==NULL)
		return -EINVAL;

       	pfdata->record_no=file_count;
        pfdata->offset=file_count*block_size;
	if(pfdata->total_size-pfdata->offset > block_size)
	{
		pfdata->data_size=block_size;
		isend=1;
	}
	else
	{
		pfdata->data_size=pfdata->total_size-pfdata->offset;
		isend=0;
	}
	pfdata->file_data=(BYTE *)Talloc0(sizeof(char)*pfdata->data_size);

       	if(read(fd,pfdata->file_data,pfdata->data_size)!=pfdata->data_size)
       	{
       	 	print_cubeerr("read vm list error! \n");
       		return -EINVAL;
       	}

	package_size=struct_2_blob(pfdata,Blob,pfdata_template);
		
	if(package_size>0)
	{
		ret=send_conn->conn_ops->write(send_conn,Blob,package_size);
	}
	
	if(ret!=package_size)
		return -EINVAL;

	printf("\r record %d",pfdata->record_no);
	file_count++;	

	if(isend)
		return 1;
	else
		return 0;
		
}

int proc_file_receive_start( struct tcloud_connector * channel_conn)
{
	int ret;
	int blob_size;
	void * pfdata_template = memdb_get_template(TYPE_PAIR(FILE_TRANS,FILE_DATA));
	if(pfdata_template==NULL)
		return -EINVAL;

	ret=channel_conn->conn_ops->read(channel_conn,Blob,1024);

	if(ret <=0)
		return -EINVAL;

        pfdata=Dalloc0(sizeof(struct policyfile_data),0);
	if(pfdata==NULL)
	{
		return -ENOMEM;
	}
	blob_size = blob_2_struct(Blob,pfdata,pfdata_template);
	if(blob_size<0)
		return blob_size;

	fd = open(pfdata->filename,O_CREAT|O_TRUNC|O_RDWR,0666);
	if(fd<0)
		return fd;
	lseek(fd,pfdata->offset,SEEK_SET);
	write(fd,pfdata->file_data,pfdata->data_size);
	printf("\n");
	if(pfdata->total_size<=pfdata->offset+pfdata->data_size)
	{
		close(fd);
		return 0;
	}
	return 1;
}

int proc_file_receive( void * sub_proc,struct tcloud_connector * recv_conn)
{
	int ret;
	int blob_size;
	int curr_size;
	void * pfdata_template = memdb_get_template(TYPE_PAIR(FILE_TRANS,FILE_DATA));
	if(pfdata_template==NULL)
		return -EINVAL;

	ret=channel_conn->conn_ops->read(channel_conn,Blob+left_size,2048-left_size);

	if(ret <0)
		return -EINVAL;

	curr_size=left_size+ret;
	
	left_size=0;


	blob_size = blob_2_struct(Blob,pfdata,pfdata_template);
	if(blob_size<0)
		return blob_size;
	left_size= curr_size-blob_size;

	if(left_size>2048)
	{
		printf("data error!\n");
		return -EINVAL;
	}
	print_cubeaudit("file_receive left_size %d blob_size %d ",left_size,blob_size);
	Memcpy(Blob,Blob+blob_size,left_size);

	lseek(fd,pfdata->offset,SEEK_SET);
	write(fd,pfdata->file_data,pfdata->data_size);
	printf("\r record %d",pfdata->record_no);
	if(pfdata->total_size<=pfdata->offset+pfdata->data_size)
	{
		close(fd);
		return 0;
	}
	return 1;
}
