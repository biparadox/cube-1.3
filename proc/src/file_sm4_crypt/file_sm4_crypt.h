#ifndef FILE_SM4_CRYPT_FUNC_H
#define FILE_SM4_CRYPT_FUNC_H


// plugin's init func and kickstart func
int file_sm4_crypt_init(void * sub_proc,void * para);
int file_sm4_crypt_start(void * sub_proc,void * para);

struct init_struct
{
	char default_key[DIGEST_SIZE];
};
#endif
