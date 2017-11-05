#ifndef ROUTERECTOR_PROCESS_FUNC_H
#define ROUTERECTOR_PROCESS_FUNC_H

struct router_init_para
{ 
	char * router_file;
	char * aspect_file;
};

int proc_router_init(void * sub_proc,void * para);
int proc_router_start(void * this_proc,void * para);

//static struct timeval time_val={0,50*1000};
#endif
