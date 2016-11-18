#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
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
	struct tcloud_connector * conn;
	struct tcloud_connector_hub * hub;
	int ret;
	char readbuf[1024];
	int i;
	char writebuf[1024];
	pid_t pid;

	pid=getpid();

	memset(readbuf,0,1024);
	struct connector_client_ops * client_ops;

	conn=get_connector(CONN_CLIENT,AF_UNIX);
//	conn=get_connector(CONN_CLIENT,AF_INET);
	if((conn ==NULL) & IS_ERR(conn))
	{
		printf("get conn failed!\n");
		exit(-1);
	}

	hub=get_connector_hub();
	ret=conn->conn_ops->init(conn,"client","TCLOUD-CONN-M-1");
//	ret=conn->conn_ops->init(conn,"client","127.0.0.1:9099");
	fprintf(stdout,"haha01\n");
	sleep(1);
		
	hub->hub_ops->add_connector(hub,conn,NULL);

	ret=conn->conn_ops->connect(conn);
	fprintf(stdout,"test connect,return value is %d!\n",ret);

	if(ret<0)
		exit(-1);

	for(i=0;i<20;i++)
	{
		sleep(1);
		ret=hub->hub_ops->select(hub,NULL);
		if(ret<=0)
			continue;

		ret=conn->conn_ops->read(conn,readbuf,1024);
		fprintf(stdout,"test read msg,return value is %d!\n",ret);
		sprintf(writebuf,"%d client send %d msg!",pid,i);
		ret=conn->conn_ops->write(conn,writebuf,strlen(writebuf));
		fprintf(stdout,"%d client test write msg,return value is %d!\n",pid,ret);
	}
	sleep(3);
	sprintf(writebuf,"End %d client !",pid);
	ret=conn->conn_ops->write(conn,writebuf,strlen(writebuf));
	fprintf(stdout,"%d client test write end msg,return value is %d!\n",pid,ret);


/*
* Close and unlink our socket path:
*/
	close (conn->conn_ops->getfd(conn));
	return 0;
}
