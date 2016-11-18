#ifndef SEC_ENTITY_H
#define SEC_ENTITY_H


enum secure_proc_state
{
	SEC_PROC_CREATE=0,
	SEC_PROC_INIT,
	SEC_PROC_START,
	SEC_PROC_RUNNING,
	SEC_PROC_WAITING,
	SEC_PROC_STOP,
	SEC_PROC_ZOMBIE,
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

enum sec_resource_state
{
	SEC_RES_INIT,
	SEC_RES_FREE,
	SEC_RES_OCCUPY,
	SEC_RES_ERR,
};
enum proc_type
{
	PROC_TYPE_MAIN,
	PROC_TYPE_CONN,
	PROC_TYPE_ROUTER,
	PROC_TYPE_MONITOR,
	PROC_TYPE_DECIDE,
	PROC_TYPE_CONTROL,
};

int message_queue_init(void **msg_queue);
int message_queue_putmsg(void * msg_queue,void * msg);
int message_queue_getmsg(void * msg_queue,void ** msg);
int message_queue_destroy(void * msg_queue);


int proc_share_data_init(struct struct_elem_attr * share_data_desc);
int proc_share_data_getstate();
int proc_share_data_setstate(int state);
void * proc_share_data_getpointer();
int proc_share_data_setpointer(void * pointer);
int proc_share_data_getvalue(char * valuename,void * value);
int  proc_share_data_setvalue(char * valuename,void * value);
int proc_share_data_reset();
int proc_share_data_destroy();

int sec_object_list_init();
void * find_sec_object(char * uuid);
void * get_first_sec_object();
void * get_next_sec_object();
int    add_sec_object(void * sec_object);
void * remove_sec_object();

void * sec_object_init(char * uuid,struct struct_elem_attr *  share_data_desc);
int sec_object_getstate(void * sec_object);
int sec_object_setstate(void * sec_object,int state);
void * sec_object_getpointer(void * sec_object);
int sec_object_setpointer(void * sec_object,void * pointer);
int sec_object_getvalue(void * sec_object,char * valuename,void * value);
int sec_object_setvalue(void * sec_objet,char * valuename,void * value);
int sec_object_reset();
int sec_object_destroy();


int sec_respool_list_init();
void * find_sec_respool(char * uuid);
void * get_first_sec_respool();
void * get_next_sec_respool();
int add_sec_respool(void * sec_respool);
void * remove_sec_respool(char * uuid);
void * sec_respool_init(char * uuid);
void * sec_resource_init(char * uuid,struct struct_elem_attr *  share_data_desc);
int sec_respool_addres(void * respool,void * res);
int sec_respool_getres(void * respool,void ** res);
int sec_respool_freeres(void * respool,void * res);
int sec_respool_reset(void * respool);
int sec_respool_getstate(void * sec_obj);
int sec_respool_setstate(void * sec_obj,int state);
void * sec_resource_getpointer(void * sec_res);
int sec_resource_setpointer(void * sec_res,void * pointer);
int sec_resource_getvalue(void * sec_res,char * valuename,void * value);
int sec_resource_setvalue(void * sec_res,char * valuename,void * value);
int sec_resource_destroy(void * sec_res);


/*
struct verify_info  
{
	char verify_data_uuid[DIGEST_SIZE*2];
	char entity_uuid[DIGEST_SIZE*2];
	char policy_type[4];
	int trust_level;
	int info_len;
	char * info;
}__attribute__((packed));
*/

// sec subject manage function
int sec_subject_list_init();
int find_sec_subject(char * name,void ** sec_sub);
int get_first_sec_subject(void **sec_sub);
int get_next_sec_subject(void **sec_sub);
int add_sec_subject(void * sec_subject);
int remove_sec_subject(char * name,void **sec_sub);

// sec subject init function
int sec_subject_create(char * name,int type,struct struct_elem_attr *  context_desc, void ** sec_sub);
//int sec_subject_setstatelist(void * sec_sub,void * namelist);

int sec_subject_init(void * sec_sub,void * pointer);

//int sec_subject_create_statelist(void * sec_sub,char ** state_namelist);
// create a state name-value double list, and set default value in it
// this function is called in main_proc

//int sec_subject_register_statelist(void * sec_sub,NAME2VALUE * state_list);
// register some value in subject sec_sub's state name list
// this function is called in sub_proc's init function

//int sec_subject_create_funclist(void * sec_sub,char ** func_namelist);
// create a func name-pointer double list, and set default value in it
// this function is called in main_proc

//int sec_subject_register_funclist(void * sec_sub,NAME2POINTER * func_list);
// register some value in subject sec_sub's state name list
// this function is called in sub_proc's init function

//int sec_subject_getfunc(void * sec_sub,char * func_name, void ** func);
// get a func by func_name
int sec_subject_gettype(void * sec_sub);
void * sec_subject_getheadtemplate(void * sec_sub);

//int sec_subject_getpolicy(void * sec_sub, void ** deal_policy);

//int sec_subject_setpolicy(void * sec_sub, void * deal_policy);

int sec_subject_setinitfunc(void * sec_sub,void * init);
int sec_subject_setstartfunc(void * sec_sub,void * start);

// sec subject running test function
int sec_subject_getprocstate(void * sec_sub);
int sec_subject_getstatestr(void * sec_sub,char ** statestr);
int sec_subject_teststate(void * sec_sub,char * statestr);
int sec_subject_getstate(void * sec_sub);
void * sec_subject_getname(void * sec_sub);
int sec_subject_getcontext(void * sec_sub,void ** context);

int sec_subject_start(void * sec_sub,void * para);
int sec_subject_join(void * sec_sub,int * retval);
int sec_subject_proc_getpara(void * arg,void ** sec_sub,void ** para);
int sec_subject_sendmsg(void * sec_sub,void *msg);
int sec_subject_recvmsg(void * sec_sub,void **msg);

int send_sec_subject_msg(void * sec_sub,void * msg);
int recv_sec_subject_msg(void * sec_sub,void ** msg);

void sec_subject_destroy(void * sec_sub);

typedef struct tagsec_subject_head
{
	char name[DIGEST_SIZE*2];
	char uuid[DIGEST_SIZE*2];
	int type;
	int proc_state;
	int fsm_state;
}__attribute__((packed)) SEC_SUBJECT_HEAD;
#endif
