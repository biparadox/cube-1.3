#ifndef LOG_MSG_RECOVER_H
#define LOG_MSG_RECOVER_H
 
int log_msg_recover_init (void * sub_proc, void * para);
int log_msg_recover_start (void * sub_proc, void * para);

struct init_para
{
	int start_delay;  // delay time in first msg send(ms)
	int infile_delay;	
	int exfile_delay;	
}__attribute__((packed));

#endif
