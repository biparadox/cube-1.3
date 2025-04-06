#ifndef OUTPUT_CHANNEL_FUNC_H
#define OUTPUT_CHANNEL_FUNC_H


// plugin's init func and kickstart func
int output_channel_init(void * sub_proc,void * para);
int output_channel_start(void * sub_proc,void * para);

struct output_init_para
{
	char * channel_name;
	char * datafile;
}__attribute__((packed));

#endif
