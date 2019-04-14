#ifndef MSG2CHANNEL_H
#define MSG2CHANNEL_H
int msg2channel_init(void * sub_proc,void * para);
int msg2channel_start(void * sub_proc,void * para);

struct msg2channel_init_para
{
     char * channel_name;
}__attribute__((packed));
#endif
