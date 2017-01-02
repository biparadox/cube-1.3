#ifndef ROUTERECTOR_PROCESS_FUNC_H
#define ROUTERECTOR_PROCESS_FUNC_H


enum proc_router_state
{
	PROC_ROUTER_START=0x1000,
	PROC_ROUTER_SYNC,
	PROC_ROUTER_FAIL,
};

static NAME2VALUE router_state_list[]=
{
	{"start",PROC_ROUTER_START},
	{"sync",PROC_ROUTER_SYNC},
	{"fail",PROC_ROUTER_FAIL},
	{NULL,0}
};

int proc_router_start(void * this_proc,void * para);

static char * router_process_state_name[]=
{
	"start",
	"sync",
	"acksend",
	"ackrecv",
	"channelbuild",
	NULL
};
static char * router_process_func_name[]=
{
	"start",
	NULL
};

static NAME2POINTER router_func_list[]=
{
	{"start",&proc_router_start},
	{NULL,0}
};

#endif
