#ifndef ECHO_PLUGIN_FUNC_H
#define ECHO_PLUGIN_FUNC_H


// plugin's init func and kickstart func
int echo_plugin_init(void * sub_proc,void * para);
int echo_plugin_start(void * sub_proc,void * para);
struct timeval time_val={0,50*1000};

#endif
