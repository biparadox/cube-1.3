#ifndef HELLO_FUNC_H
#define HELLO_FUNC_H


// plugin's init func and kickstart func
int hello_init(void * sub_proc,void * para);
int hello_start(void * sub_proc,void * para);
struct timeval time_val={0,50*1000};

#endif
