#ifndef LINUX_SYS_FUNC_H
#define LINUX_SYS_FUNC_H

int print_cubeerr(char * format,...);
int print_cubeaudit(char * format,...);
int read_json_node(int fd, void ** node);
int read_json_file(char * file_name);
int read_record_file(char * record_file);

#endif
