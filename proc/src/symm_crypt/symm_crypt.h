#ifndef SYMM_CRYPT_FUNC_H
#define SYMM_CRYPT_FUNC_H

struct init_struct
{
	char passwd[DIGEST_SIZE];
};

// plugin's init func and kickstart func
int symm_crypt_init(void * sub_proc,void * para);
int symm_crypt_start(void * sub_proc,void * para);

#endif
