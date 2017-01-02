#ifndef SEC_ENTITY_H
#define SEC_ENTITY_H


enum ex_mod_state
{
	EX_MOD_CREATE=0,
	EX_MOD_INIT,
	EX_MOD_START,
	EX_MOD_RUNNING,
	EX_MOD_WAITING,
	EX_MOD_STOP,
	EX_MOD_ZOMBIE,
};

enum default_fsm_state
{
	PROC_LOCAL_INIT=0,
	PROC_LOCAL_START,
	PROC_LOCAL_STOP=0x7fff,
};

static NAME2VALUE default_state_list[]=
{
	{"init",PROC_LOCAL_INIT},
	{"start",PROC_LOCAL_START},
	{"stop",PROC_LOCAL_STOP},
	{NULL,0},
};

enum proc_type
{
	PROC_TYPE_MAIN,
	PROC_TYPE_CONN,
	PROC_TYPE_ROUTER,
	PROC_TYPE_PORT,
	PROC_TYPE_MONITOR,
	PROC_TYPE_DECIDE,
	PROC_TYPE_CONTROL,
};

// sec subject manage function
int ex_module_list_init();
int find_ex_module(char * name,void ** ex_mod);
int get_first_ex_module(void **ex_mod);
int get_next_ex_module(void **ex_mod);
int add_ex_module(void * ex_module);
int remove_ex_module(char * name,void **ex_mod);

// sec subject init function
int ex_module_create(char * name,int type,struct struct_elem_attr *  context_desc, void ** ex_mod);
//int ex_module_setstatelist(void * ex_mod,void * namelist);

int ex_module_init(void * ex_mod,void * pointer);

//int ex_module_create_statelist(void * ex_mod,char ** state_namelist);
// create a state name-value double list, and set default value in it
// this function is called in main_proc

//int ex_module_register_statelist(void * ex_mod,NAME2VALUE * state_list);
// register some value in subject ex_mod's state name list
// this function is called in sub_proc's init function

//int ex_module_create_funclist(void * ex_mod,char ** func_namelist);
// create a func name-pointer double list, and set default value in it
// this function is called in main_proc

//int ex_module_register_funclist(void * ex_mod,NAME2POINTER * func_list);
// register some value in subject ex_mod's state name list
// this function is called in sub_proc's init function

//int ex_module_getfunc(void * ex_mod,char * func_name, void ** func);
// get a func by func_name
int ex_module_gettype(void * ex_mod);
void * ex_module_getheadtemplate(void * ex_mod);

//int ex_module_getpolicy(void * ex_mod, void ** deal_policy);

//int ex_module_setpolicy(void * ex_mod, void * deal_policy);

int ex_module_setinitfunc(void * ex_mod,void * init);
int ex_module_setstartfunc(void * ex_mod,void * start);

// sec subject running test function
char * ex_module_getname(void * ex_mod);
int ex_module_setname(void * ex_mod,char * name);
int ex_module_setpointer(void * ex_mod,void * pointer);
void * ex_module_getpointer(void * ex_mod);

int ex_module_getcontext(void * ex_mod,void ** context);

int ex_module_start(void * ex_mod,void * para);
int ex_module_join(void * ex_mod,int * retval);
int ex_module_proc_getpara(void * arg,void ** ex_mod,void ** para);

void ex_module_destroy(void * ex_mod);

typedef struct tagex_module_head
{
	BYTE uuid[DIGEST_SIZE];
	char name[DIGEST_SIZE];
	int type;
}__attribute__((packed)) EX_MODULE_HEAD;
#endif
