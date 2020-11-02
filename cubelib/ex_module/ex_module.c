#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>

#include "data_type.h"
#include "alloc.h"
#include "list.h"
#include "attrlist.h"
#include "memfunc.h"
#include "struct_deal.h"
//#include "connector.h"
#include "ex_module.h"

#include "slot_list.h"

typedef struct proc_ex_module
{
	EX_MODULE_HEAD head;
	pthread_t proc_thread; 
	pthread_attr_t thread_attr; 

	void * context;
	void * context_template;
	void * pointer;
	int  (*init)(void *,void *);
	int  (*start)(void *,void *);
	pthread_mutex_t mutex;
	void * recv_queue;
	void * send_queue;
	void * slots;
	void * socks;
	int  state;
	int  retval;
}__attribute__((packed)) EX_MODULE;

struct ex_module_list
{
	int state;
	pthread_rwlock_t rwlock;
	Record_List head;
	struct List_head * curr;
}; 

EX_MODULE * main_module;

void * ex_module_addslot(void * ex_mod,void * slot_port)
{
	EX_MODULE * ex_module=*(EX_MODULE **)ex_mod;
	if(ex_mod==NULL)
		return NULL;
	return slot_list_addslot(ex_module->slots,slot_port);
}

void * ex_module_addsock(void * ex_mod,void * sock)
{
	EX_MODULE * ex_module=*(EX_MODULE **)ex_mod;
	if(ex_mod==NULL)
		return NULL;
	return slot_list_addslot(ex_module->socks,sock);
}

void * ex_module_removesock(void * ex_mod,BYTE * uuid)
{
	EX_MODULE * ex_module=*(EX_MODULE **)ex_mod;
	if(ex_mod==NULL)
		return NULL;
	return slot_list_removesock(ex_module->socks,uuid);
}

void * ex_module_findport(void * ex_mod,char *name)
{
	EX_MODULE * ex_module=*(EX_MODULE **)ex_mod;
	if(ex_mod==NULL)
		return NULL;
	return slot_list_findport(ex_module->slots,name);
}

void * ex_module_findsock(void * ex_mod,BYTE * uuid)
{
	EX_MODULE * ex_module=*(EX_MODULE **)ex_mod;
	if(ex_mod==NULL)
		return NULL;
	return slot_list_findsock(ex_module->socks,uuid);
}

struct proc_context
{
	BYTE uuid[32];
	char *proc_name;
	char *user_name;
	int  state;
}__attribute__((packed));

static struct struct_elem_attr proc_context_desc[]=
{
	{"uuid",CUBE_TYPE_UUID,DIGEST_SIZE,NULL,NULL},
	{"proc_name",CUBE_TYPE_ESTRING,DIGEST_SIZE,NULL},
	{"user_name",CUBE_TYPE_ESTRING,DIGEST_SIZE,NULL},
	{"state",CUBE_TYPE_INT,0,NULL,NULL},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL}
};

static struct ex_module_list * ex_module_list;

int entity_comp_uuid(void * List_head, void * uuid) 
{                                                             
	struct List_head * head;
	EX_MODULE_HEAD * entity_head;    
	head=(struct List_head *)List_head;
	if(head==NULL)
		return -EINVAL;	
	Record_List * record;                             
	record = List_entry(head,Record_List,list);              
	entity_head = (EX_MODULE_HEAD *) record->record;                      
	if(entity_head == NULL)
		return -EINVAL;
	if(entity_head->uuid==NULL)
		return -EINVAL;
	return memcmp(entity_head->uuid,uuid,DIGEST_SIZE);        
}

int entity_comp_name(void * List_head, void * name) 
{                                                             
	struct List_head * head;
	EX_MODULE_HEAD * entity_head;    
	head=(struct List_head *)List_head;
	if(head==NULL)
		return -EINVAL;	
	Record_List * record;                             
	char * string;
	string=(char *)name;
	record = List_entry(head,Record_List,list);              
	entity_head = (EX_MODULE_HEAD *) record->record;                      
	if(entity_head == NULL)
		return -EINVAL;
	if(entity_head->name==NULL)
		return -EINVAL;
	return strncmp(entity_head->name,string,DIGEST_SIZE*2);        
}

int ex_module_list_init()
{
	int ret;
	ex_module_list=Salloc(sizeof(struct ex_module_list));
	if(ex_module_list==NULL)
		return -ENOMEM;
	INIT_LIST_HEAD(&(ex_module_list->head.list));
	ex_module_list->head.record=NULL;
	ex_module_list->curr=&(ex_module_list->head.list);
	ret=pthread_rwlock_init(&(ex_module_list->rwlock),NULL);

	main_module=Salloc0(sizeof(*main_module));
	main_module->context_template=create_struct_template(&proc_context_desc);
	if(main_module->context_template==NULL)
		return -EINVAL;
	main_module->context=Salloc0(struct_size(main_module->context_template));

	return 0;
}

int find_ex_module(char * name,void ** ex_mod)
{
	struct List_head * curr_head;
	Record_List * record_elem;
	Record_List * record_list;
	record_list=&(ex_module_list->head);
	int ret;
	*ex_mod=NULL;

	pthread_rwlock_rdlock(&(ex_module_list->rwlock));
	curr_head = find_elem_with_tag(record_list,
		entity_comp_name,name);
	if(curr_head == NULL)
	{
		pthread_rwlock_unlock(&(ex_module_list->rwlock));
		return 0;
	}
	if(IS_ERR(curr_head))
	{
		pthread_rwlock_unlock(&(ex_module_list->rwlock));
		return curr_head;
	}
	record_elem=List_entry(curr_head,Record_List,list);
	pthread_rwlock_unlock(&(ex_module_list->rwlock));
	*ex_mod=&(record_elem->record);
	return 1;	
}

int get_first_ex_module(void **ex_mod)
{
	Record_List * recordhead;
	Record_List * newrecord;
	struct List_head * curr_head;

	pthread_rwlock_rdlock(&(ex_module_list->rwlock));

	recordhead = &(ex_module_list->head);
	if(recordhead==NULL)
	{
		pthread_rwlock_unlock(&(ex_module_list->rwlock));
		*ex_mod=NULL;
		return 0;
	}
	curr_head = recordhead->list.next;
	ex_module_list->curr = curr_head;
	newrecord = List_entry(curr_head,Record_List,list);
	pthread_rwlock_unlock(&(ex_module_list->rwlock));
	*ex_mod=&(newrecord->record);
	return 1;
}

int get_next_ex_module(void **ex_mod)
{
	Record_List * recordhead;
	Record_List * newrecord;
	struct List_head * curr_head;

	if((ex_mod == NULL) ||(*ex_mod==NULL))
		return -EINVAL;

	recordhead = &(ex_module_list->head);
	newrecord = List_entry(*ex_mod,Record_List,record);

	curr_head = newrecord->list.next;
	if(curr_head==recordhead)
	{
		*ex_mod=NULL;
		return 0;
	}
	newrecord = List_entry(curr_head,Record_List,list);
	*ex_mod=&(newrecord->record);
	return 1;
}

int add_ex_module(void * ex_module)
{
	Record_List * recordhead;
	Record_List * newrecord;

	recordhead = &(ex_module_list->head);
	if(recordhead==NULL)
		return -ENOMEM;

	newrecord=List_entry(ex_module,Record_List,record);

	pthread_rwlock_wrlock(&(ex_module_list->rwlock));
	List_add_tail(&(newrecord->list),recordhead);
	pthread_rwlock_unlock(&(ex_module_list->rwlock));
	return 0;
}	

int remove_ex_module(char * name,void **ex_mod)
{
	Record_List * recordhead;
	Record_List * record_elem;
	struct List_head * curr_head;
	void * record;

	recordhead = &(ex_module_list->head);
	if(recordhead==NULL)
		return 0;

	pthread_rwlock_wrlock(&(ex_module_list->rwlock));
	curr_head=find_elem_with_tag(recordhead,entity_comp_uuid,name);
	if(curr_head==NULL)
	{
		pthread_rwlock_unlock(&(ex_module_list->rwlock));
		return 0;
	}
	record_elem=List_entry(curr_head,Record_List,list);
	List_del(curr_head);
	pthread_rwlock_unlock(&(ex_module_list->rwlock));
	record=&(record_elem->record);
	free(record_elem);
        *ex_mod=record;	
	return 1;
}	

int ex_module_create(char * name,int type,struct struct_elem_attr *  context_desc, void ** ex_mod)
{
	int ret;
	EX_MODULE * ex_module;
	Record_List * module_record;
	if(name==NULL)
		return -EINVAL;

	

	// alloc mem for ex_module
	ex_module=Calloc(sizeof(EX_MODULE));
	if(ex_module==NULL)
		return -ENOMEM;
	Memset(ex_module,0,sizeof(EX_MODULE));

	// alloc mem for module's head
	module_record=Calloc(sizeof(*module_record));
	if(module_record==NULL)
		return -EINVAL;
	
	INIT_LIST_HEAD(&(module_record->list));
	module_record->record=ex_module;
	*ex_mod=&(module_record->record);

	// assign some  value for ex_module
	strncpy(ex_module->head.name,name,DIGEST_SIZE*2);

	// init the proc's mutex and the cond

	if(context_desc!=NULL)
	{
		ex_module->context_template=create_struct_template(context_desc);
		if(ex_module->context_template==NULL)
		{
			free(ex_module);
			return -EINVAL;
		}
	}
	// init the send message queue and the receive message queue 
	ret=message_queue_init(&(ex_module->recv_queue));
	if(ret<0)
	{
		pthread_mutex_destroy(&(ex_module->mutex));
		free(ex_module);
		return -EINVAL;
	}
	ret=message_queue_init(&(ex_module->send_queue));
	if(ret<0)
	{
		message_queue_destroy(&(ex_module->recv_queue));
		pthread_mutex_destroy(&(ex_module->mutex));
		free(ex_module);
		return -EINVAL;
	}


	pthread_attr_init(&(ex_module->thread_attr));
	ex_module->head.type=type;
	ex_module->init=NULL;
	ex_module->start=NULL;
	// init the  slot list and sock list

	ex_module->slots=slot_list_init();
	if(ex_module->slots==NULL)
		return -EINVAL;

	ex_module->socks=slot_list_init();
	if(ex_module->socks==NULL)
		return -EINVAL;
	
	// init the proc's mutex and the cond
	ret=pthread_mutex_init(&(ex_module->mutex),NULL);

	return 0;
}

int ex_module_setinitfunc(void * ex_mod,void * init)
{
	int ret;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
	{
		ex_module=main_module;
	}
	else
	{
		ex_module=*(EX_MODULE **)ex_mod;
	}
	ex_module->init=init;
	return 0;
}



int ex_module_setstartfunc(void * ex_mod,void * start)
{
	int ret;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
		return -EINVAL;
	ex_module = *(EX_MODULE **)ex_mod;
	ex_module->start=start;
	return 0;
}

char * ex_module_getname(void * ex_mod)
{
	int ret;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
		return NULL;
	ex_module = *(EX_MODULE **)ex_mod;

	return &ex_module->head.name;
}

int ex_module_setname(void * ex_mod,char * name)
{
	int ret;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
		return -EINVAL;
	ex_module = *(EX_MODULE **)ex_mod;
	strncpy(ex_module->head.name,name,DIGEST_SIZE);

	return &ex_module->head.name;
}

int ex_module_setpointer(void * ex_mod,void * pointer)
{
	int ret;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
		return -EINVAL;
	ex_module = *(EX_MODULE **)ex_mod;
	ex_module->pointer=pointer;

	return 0;
}

void * ex_module_getpointer(void * ex_mod)
{
	int ret;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
		return NULL;
	ex_module = *(EX_MODULE **)ex_mod;

	return ex_module->pointer;
}

int ex_module_getcontext(void * ex_mod,void ** context)
{
	int ret;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
		return -EINVAL;
	ex_module = *(EX_MODULE **)ex_mod;
	if(ex_module==NULL)
		return -EINVAL;
	*context=ex_module->context;
	return 0;
}

int ex_module_gettype(void * ex_mod)
{
	int ret;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
		return -EINVAL;
	ex_module = *(EX_MODULE **)ex_mod;
	if(ex_module==NULL)
		return -EINVAL;

	return ex_module->head.type;
}
int _ex_module_passpara(void * pointer)
{
	struct subject_para_struct
	{
		void  * ex_mod;
		void * para;
		int (*start)(void *,void *);
	};
	
	struct subject_para_struct * trans_pointer=pointer;

	EX_MODULE * ex_module;

	
	if((trans_pointer==NULL) ||IS_ERR(trans_pointer))
	pthread_exit((void *)-EINVAL);
	if(trans_pointer->ex_mod==NULL)
	{
		ex_module=main_module;
	}
	else
	{
		ex_module=*(EX_MODULE **)(trans_pointer->ex_mod);
	}
	ex_module->retval=trans_pointer->start(trans_pointer->ex_mod,trans_pointer->para);
	pthread_exit((void *)&(ex_module->retval));

}

int ex_module_init(void * ex_mod,void * para)
{
	int ret=0;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
	{
		ex_mod=&main_module;
	}
	ex_module=*(EX_MODULE **)ex_mod;

	// judge if the ex_module's state is right
	if(ex_module->init ==NULL)
	{
		return ret;
	}
	ret=ex_module->init(ex_mod,para);

	return ret;
}

int ex_module_start(void * ex_mod,void * para)
{
	struct subject_para_struct
	{
		void * ex_mod;
		void * para;
		int (*start)(void *,void *);
	};
	
	struct subject_para_struct * trans_pointer;

	int ret;
	
	EX_MODULE * ex_module=*(EX_MODULE **)ex_mod;
	if(ex_mod==NULL)
		return -EINVAL;
	if(ex_module->start==NULL)
		return -EINVAL;

	trans_pointer=malloc(sizeof(struct subject_para_struct));
	if(trans_pointer==NULL)
	{
		free(trans_pointer);
		return -ENOMEM;
	}


	trans_pointer->ex_mod=ex_mod;
	trans_pointer->para=para;
	trans_pointer->start=ex_module->start;
	
	ret=pthread_create(&(ex_module->proc_thread),NULL,_ex_module_passpara,trans_pointer);
	return ret;

}

int ex_module_join(void * ex_mod,int * retval)
{
	int ret;
	int * thread_return;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
	{
		return -EINVAL;
	}
	ex_module = *(EX_MODULE **)ex_mod;
	ret=pthread_join(ex_module->proc_thread,&thread_return);
	ex_module->retval=*thread_return;
	*retval=*thread_return;
	
	return ret;
}

int ex_module_tryjoin(void * ex_mod,int * retval)
{
	int ret;
	int * thread_return;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
	{
		return -EINVAL;
	}
	ex_module = *(EX_MODULE **)ex_mod;
	ret=pthread_tryjoin_np(ex_module->proc_thread,&thread_return);
	ex_module->retval=*thread_return;
	*retval=*thread_return;
	
	return ret;
}

int ex_module_proc_getpara(void * arg,void ** ex_mod,void ** para)
{
	struct subject_para_struct
	{
		void * ex_module;
		void * para;
	};
	
	if((arg==NULL) || IS_ERR(arg))
		return -EINVAL;
	//printf("subject getpara!,arg=%x\n",arg);
	struct subject_para_struct * trans_pointer=(struct subject_para_struct * )arg;
	
	//printf("ex_module =%x\n",trans_pointer->ex_module);
	if((trans_pointer->ex_module==NULL)||IS_ERR(trans_pointer->ex_module))
	{
		//printf("sec subject get para err!\n");
		return -EINVAL;
	}

	*ex_mod=trans_pointer->ex_module;
	*para=trans_pointer->para;
	free(trans_pointer);
	return 0;	
}

int ex_module_sendmsg(void * ex_mod,void *msg)
{
	int ret;
	EX_MODULE * ex_module=*(EX_MODULE **)ex_mod;
	if(ex_mod==NULL)
		return -EINVAL;
	if(msg==NULL)
		return -EINVAL;

	BYTE buffer[DIGEST_SIZE];
	Memset(buffer,0,DIGEST_SIZE);
	message_set_receiver(msg,buffer);
	//message_set_sender(msg,ex_module_getname(ex_mod));

	return message_queue_putmsg(ex_module->send_queue,msg);
}

int ex_module_recvmsg(void * ex_mod,void **msg)
{
	int ret;
	EX_MODULE * ex_module=*(EX_MODULE **)ex_mod;
	if(ex_mod==NULL)
		return -EINVAL;
	*msg=NULL;

	return message_queue_getmsg(ex_module->recv_queue,msg);

}

int send_ex_module_msg(void * ex_mod,void * msg)
{
	int ret;
	EX_MODULE * ex_module=*(EX_MODULE **)ex_mod;
	if(ex_mod==NULL)
		return -EINVAL;

	return message_queue_putmsg(ex_module->recv_queue,msg);
}

int recv_ex_module_msg(void * ex_mod,void ** msg)
{
	int ret;
	EX_MODULE * ex_module=*(EX_MODULE **)ex_mod;
	if(ex_mod==NULL)
		return -EINVAL;

	return message_queue_getmsg(ex_module->send_queue,msg);
}

void ex_module_destroy(void * ex_mod)
{
	int ret;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
		return ;
	ex_module = *(EX_MODULE **)ex_mod;
	pthread_mutex_destroy(&(ex_module->mutex));
	free(ex_module);
	return;
}

int proc_share_data_getstate()
{

	struct proc_context * context;
	if(main_module==NULL)
		return -1;
	context=main_module->context;
	if(context==NULL)
		return -1;
	return context->state;
}

int proc_share_data_setstate(int state)
{
	struct proc_context * context;
	if(main_module==NULL)
		return -1;
	context=main_module->context;
	if(context==NULL)
		return -1;
	context->state=state;	
	return state;
}


void * proc_share_data_getpointer()
{
	struct proc_context * context;
	if(main_module==NULL)
		return -1;
	return main_module->pointer;
}
int proc_share_data_setpointer(void * pointer)
{
	struct proc_context * context;
	if(main_module==NULL)
		return -1;
	main_module->pointer=pointer;
	return 0;
}

int proc_share_data_getvalue(char * valuename,void * value)
{
	struct proc_context * context;
	if(main_module==NULL)
		return -1;
	context=main_module->context;
	if(context==NULL)
		return -1;
	int ret;
	
	ret=struct_read_elem(valuename,main_module->context,value,main_module->context_template);
	return ret;

}
 
int proc_share_data_setvalue(char * valuename,void * value)
{
	struct proc_context * context;
	if(main_module==NULL)
		return -1;
	context=main_module->context;
	if(context==NULL)
		return -1;
	int ret;
	ret=struct_write_elem(valuename,main_module->context,value,main_module->context_template);
	return ret;
}
