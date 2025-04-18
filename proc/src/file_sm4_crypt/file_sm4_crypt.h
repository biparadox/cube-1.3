#ifndef FILE_SM4_CRYPT_FUNC_H
#define FILE_SM4_CRYPT_FUNC_H


// plugin's init func and kickstart func
int file_sm4_crypt_init(void * sub_proc,void * para);
int file_sm4_crypt_start(void * sub_proc,void * para);
struct timeval time_val={0,50*1000};

#endif
