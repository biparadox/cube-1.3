#ifndef ECHO_CHANNEL_FUNC_H
#define ECHO_CHANNEL_FUNC_H


// plugin's init func and kickstart func
int echo_channel_init(void * sub_proc,void * para);
int echo_channel_start(void * sub_proc,void * para);

struct echo_init_para
{
	char * channel_name;
}__attribute__((packed));

#endif
