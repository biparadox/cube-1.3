#ifndef HELLO_FUNC_H
#define HELLO_FUNC_H


struct init_struct
{
	char * filename;
};
// plugin's init func and kickstart func
int request_init(void * sub_proc,void * para);
int request_start(void * sub_proc,void * para);

#endif
