#ifndef MSG_EXPAND_SEND_H
#define MSG_EXPAND_SEND_H
 
int msg_expand_send_init (void * sub_proc, void * para);
int msg_expand_send_start (void * sub_proc, void * para);

struct init_para
{
	int start_delay;  // delay time in first msg send(ms)
	int infile_delay;	
	int exfile_delay;	
}__attribute__((packed));

#endif
