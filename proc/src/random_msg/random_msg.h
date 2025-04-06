#ifndef RANDOM_MSG_H
#define RANDOM_MSG_H


// plugin's init func and kickstart func
int random_msg_init(void * sub_proc,void * para);
int random_msg_start(void * sub_proc,void * para);
struct timeval time_val={0,50*1000};

struct init_para
{
	char * default_name;  // delay time in first msg send(ms)
	int default_type;	  // 0 an 1 means string 
                          // 2 means BINDATA
                          // 3 means UUID
                          // 4 means int,when choose int, value's size is 4, 
                          // when choose 2 or 3, size value will be ignored 
    int size;
	int dynamic_flag; // 0 bit means if default name will be changed when receive matching GENERAL_RETURN message
                      // 1 bit means if default type will be changed when receive matching GENERAL_RETURN message
                      // 0 means not change, 1 means change
}__attribute__((packed));
#endif
