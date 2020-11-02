
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../include/data_type.h"
#include "../include/connector.h"

static void
bail(const char *on_what) {
	perror(on_what);
	exit(1);
}


int main()
{
	struct tcloud_connector * unix_conn;
	struct tcloud_connector * inet_conn;
	struct tcloud_connector_hub * hub;
	int ret;
	char *msg="test srv send msg!";
	char *msg1="test srv response msg!";
	struct tcloud_connector * temp_conn; 
	char readbuf[1024];
	int i,j;

	struct timeval select_interval;
	select_interval.tv_sec=2;
	select_interval.tv_usec=0;


	unix_conn=get_connector(CONN_SERVER,AF_UNIX);
	inet_conn=get_connector(CONN_SERVER,AF_INET);
	hub=get_connector_hub();
	if((unix_conn ==NULL) & IS_ERR(unix_conn))
	{
		printf("get conn failed!\n");
		exit(-1);
	}
	if((inet_conn ==NULL) & IS_ERR(inet_conn))
	{
		printf("get conn failed!\n");
		exit(-1);
	}


	ret=inet_conn->conn_ops->init(inet_conn,"controller","127.0.0.1:9099");
//	ret=inet_conn->conn_ops->init(inet_conn,"controller","172.21.6.78:9099");
	ret=unix_conn->conn_ops->init(unix_conn,"controller","TCLOUD-CONN-M-1");
	sleep(1);

	hub->hub_ops->add_connector(hub,inet_conn,NULL);
	hub->hub_ops->add_connector(hub,unix_conn,NULL);

	ret=inet_conn->conn_ops->listen(inet_conn);
	fprintf(stdout,"test inet listen,return value is %d!\n",ret);

	ret=unix_conn->conn_ops->listen(unix_conn);
	fprintf(stdout,"test unix listen,return value is %d!\n",ret);
	i=0;
	for(;;)
	{

		ret=hub->hub_ops->select(hub,NULL);
		printf("select return %d!\n",ret);
		if(ret<=0)
		{
			continue;
		}


		do{
			temp_conn=hub->hub_ops->getactiveread(hub);
			if(temp_conn==NULL)
				break;
			if(temp_conn==inet_conn)
			{
				temp_conn=temp_conn->conn_ops->accept(inet_conn);
				if(ret==-1)
				{
					bail("accept()");
					continue;
				}
				fprintf(stdout,"new channel: peeraddr %s !\n",temp_conn->conn_ops->getpeeraddr(temp_conn));
				ret=temp_conn->conn_ops->write(temp_conn,msg,strlen(msg));
				fprintf(stdout,"test write msg,return value is %d!\n",ret);
				hub->hub_ops->add_connector(hub,temp_conn,NULL);
			}
			else if(temp_conn==unix_conn)
			{
				temp_conn=temp_conn->conn_ops->accept(unix_conn);
				if(ret==-1)
				{
					bail("accept()");
					continue;
				}
				fprintf(stdout,"test accept ,return value is %d!\n",ret);
				ret=temp_conn->conn_ops->write(temp_conn,msg,strlen(msg));
				fprintf(stdout,"test write msg,return value is %d!\n",ret);
				hub->hub_ops->add_connector(hub,temp_conn,NULL);
			}
			else
			{
				ret=temp_conn->conn_ops->read(temp_conn,readbuf,1024);
				if(ret<=0)
				{
					hub->hub_ops->del_connector(hub,temp_conn);
					continue;
				}
				readbuf[ret]=0;
				fprintf(stdout,"test read msg %s!\n",readbuf);
				if(readbuf[0]=='E')
				{
					temp_conn->conn_ops->disconnect(temp_conn);
					hub->hub_ops->del_connector(hub,temp_conn);
				}
				else
				{
					ret=temp_conn->conn_ops->write(temp_conn,msg1,strlen(msg));
					fprintf(stdout,"test write msg1,return value is %d!\n",ret);
				}

			}
		}while(1);	


	}
	return 0;
}
