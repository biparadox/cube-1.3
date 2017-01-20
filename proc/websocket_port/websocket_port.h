#ifndef WEBSOCKET_PORT_FUNC_H
#define WEBSOCKET_PORT_FUNC_H


// plugin's init func and kickstart func
int websocket_port_init(void * sub_proc,void * para);
int websocket_port_start(void * sub_proc,void * para);

struct websocket_init_para
{
	char * ws_addr;
	int ws_port;	
}__attribute__((packed));

#endif
