#ifndef SEC_ENTITY_H
#define SEC_ENTITY_H

enum dynamic_exmodule_typelist
{
	DTYPE_EXMODULE=0x220,
};

enum subtypelist_exmodule
{
	SUBTYPE_LIB_PARA=0x01,
};

enum module_type
{
	MOD_TYPE_MAIN=0x01,
	MOD_TYPE_CONN,
	MOD_TYPE_ROUTER,
	MOD_TYPE_PORT,
	MOD_TYPE_MONITOR,
	MOD_TYPE_DECIDE,
	MOD_TYPE_CONTROL,
	MOD_TYPE_START,
	MOD_TYPE_TRANSLATER,
};
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

struct start_para
{
	int argc;
	char **argv;
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

int ex_module_gettype(void * ex_mod);
void * ex_module_getheadtemplate(void * ex_mod);

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

int ex_module_sendmsg(void * ex_mod,void *msg);
int ex_module_recvmsg(void * ex_mod,void **msg);

int send_ex_module_msg(void * ex_mod,void * msg);
int recv_ex_module_msg(void * ex_mod,void ** msg);

int proc_share_data_getstate();
int proc_share_data_setstate(int state);
void * proc_share_data_getpointer();
int proc_share_data_setpointer(void * pointer);
int proc_share_data_getvalue(char * valuename,void * value);
int proc_share_data_setvalue(char * valuename,void * value);
void * ex_module_addslot(void * ex_mod,void * slot_port);
void * ex_module_findport(void * ex_mod,char * name);
void * ex_module_addsock(void * ex_mod,void * sock);
void * ex_module_removesock(void * ex_mod,BYTE * uuid);
void * ex_module_findsock(void * ex_mod,BYTE * uuid);

void * slot_port_init(char * name, int port_num);
void * slot_port_addrecordpin(void * port,int type,int subtype);
void * slot_port_addmessagepin(void * port,int type,int subtype);
void * slot_create_sock(void * slot_port,BYTE * uuid);
int slot_sock_addrecord(void * sock,void * record);
int slot_sock_addrecorddata(void * sock,int type,int subtype, void * record);
int slot_sock_addmsg(void * sock, void * message);
int slot_sock_isactive(void * sock);
int slot_sock_isempty(void * sock);
void * slot_sock_removerecord(void * sock,int type,int subtype);
void * slot_sock_removemessage(void * sock,int type,int subtype);

#endif
