#ifndef RANDOM_MSG_H
#define RANDOM_MSG_H


// plugin's init func and kickstart func
int utc_timer_init(void * sub_proc,void * para);
int utc_timer_start(void * sub_proc,void * para);
struct timeval time_val={0,50*1000};

struct init_para
{
	int mode;	  // 0 means only print the result 
                  // 1 means output expand data (GENERAL_RETURN,TIME_VAL)
                  // 2 means output an independent (GENERAL_RETURN,TIME_VAL) message
	char * label_string; // string to label the module's time output
}__attribute__((packed));
#endif
