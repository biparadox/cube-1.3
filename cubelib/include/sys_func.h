#ifndef LINUX_SYS_FUNC_H
#define LINUX_SYS_FUNC_H

#include <sys/time.h>
int audit_file_init();
int print_cubeerr(char * format,...);
int print_cubeaudit(char * format,...);
int debug_message(void * msg, char * info);
int read_json_node(int fd, void ** node);
int read_json_file(char * file_name);
int read_record_file(char * record_file);

extern struct timeval time_val;

int set_semvalue(int semid,int value);
void del_semvalue(int semid);
int semaphore_p(int semid,int value);
int semaphore_v(int semid,int value);

// debug functions

void  set_cubeerrinfo(char * errinfo);
char *get_cubeerrinfo();
void set_cubeerrnum(int err_num,int site);
int get_cubeerrnum(int site);
int convert_uuidname(char * name,int len,BYTE * digest,char * newfilename);


int RAND_bytes(unsigned char *buffer, size_t len);
void print_bin_data(BYTE * data,int len,int width);
int  print_pretty_text(char * text,int fd);
#endif
