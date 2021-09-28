#ifndef MSG2BINSTREAM_H
#define MSG2BINSTREAM_H
int msg2binstream_init(void * sub_proc,void * para);
int msg2binstream_start(void * sub_proc,void * para);

struct msg2binstream_init_para
{
     char * channel_name;
}__attribute__((packed));
#endif
