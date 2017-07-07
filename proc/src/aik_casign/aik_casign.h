#ifndef AIK_CASIGN_H
#define AIK_CASIGN_H

struct init_struct
{
	char passwd[DIGEST_SIZE];
};

int aik_casign_init(void * sub_proc,void * para);
int aik_casign_start(void * sub_proc,void * para);

#endif
