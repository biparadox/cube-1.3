#ifndef SYMM_CRYPT_FUNC_H
#define SYMM_CRYPT_FUNC_H

struct init_struct
{
	char defaultkey[DIGEST_SIZE];
	char updatename[DIGEST_SIZE];
};

// plugin's init func and kickstart func
int sm4crypt_init(void * sub_proc,void * para);
int sm4crypt_start(void * sub_proc,void * para);

#endif
