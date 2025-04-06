#ifndef RECORDLIB_H
#define RECORDLIB_H

// plugin's init func and kickstart func
int recordlib_init(void * sub_proc,void * para);
int recordlib_start(void * sub_proc,void * para);
struct timeval time_val={0,50*1000};

#endif
