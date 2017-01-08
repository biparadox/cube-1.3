#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/time.h>

#include "data_type.h"
#include "alloc.h"
#include "string.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "message.h"
#include "connector.h"

struct init_struct
{
	char * name;
};

int example_proc_init(void * main_proc,void * init_para)
{
	int ret;
	struct init_struct * para=(struct init_struct *)init_para;
	printf("init para is %s!\n",para->name);

	return 0;
}
