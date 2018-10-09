#ifndef LINUX_SYS_FUNC_H
#define LINUX_SYS_FUNC_H

#include <sys/time.h>
int audit_file_init();
int print_cubeerr(char * format,...);
int print_cubeaudit(char * format,...);
int read_json_node(int fd, void ** node);
int read_json_file(char * file_name);
int read_record_file(char * record_file);

struct timeval time_val={0,20*1000};

int set_semvalue(int semid,int value);
void del_semvalue(int semid);
int semaphore_p(int semid,int value);
int semaphore_v(int semid,int value);
#endif
