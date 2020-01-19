#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <dlfcn.h>

#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "basefunc.h"
#include "memdb.h"
#include "message.h"
#include "connector.h"
#include "ex_module.h"
#include "sys_func.h"

#include "main_proc_func.h"

typedef struct proc_init_parameter
{
	char * name;
	int type;
	int (* init) (void *,void *);
	int (* start) (void *,void *);
}PROC_INIT;

static char connector_config_file[DIGEST_SIZE*2]="./connector_config.cfg";
static char router_config_file[DIGEST_SIZE*2]="./router_policy.cfg";
static char aspect_config_file[DIGEST_SIZE*2]="./aspect_policy.cfg";
static char plugin_config_file[DIGEST_SIZE*2]="./plugin_config.cfg";
static char main_config_file[DIGEST_SIZE*2]="./main_config.cfg";
static char sys_config_file[DIGEST_SIZE*2]="./sys_config.cfg";
static char msgaudit_file[DIGEST_SIZE*2]="./message.log";
static char connector_plugin_file[DIGEST_SIZE*2]="libconnector_process_func.so";
static char router_plugin_file[DIGEST_SIZE*2]="librouter_process_func.so";

int main(int argc,char **argv)
{

    struct tcloud_connector_hub * hub;
    struct tcloud_connector * temp_conn;
    int ret;
    int retval;
    void * message_box;
    int i,j;
    int argv_offset;	
    char namebuffer[DIGEST_SIZE*4];
    char * sys_plugin;		
    char * app_plugin;		
    char * base_define;
    char * cube_output;


    char json_buffer[4096];
    char print_buffer[4096];
    int readlen;
    int json_offset;
    void * memdb_template ;
    BYTE uuid[DIGEST_SIZE];
    void * main_proc; // point to the main proc's subject struct
    void * conn_proc; // point to the conn proc's subject struct
    void * router_proc; // point to the conn proc's subject struct
    char local_uuid[DIGEST_SIZE*2];

    FILE * fp;
    char audit_text[4096];
    char buffer[4096];
    void * root_node;
    void * temp_node;
    PROC_INIT plugin_proc; 

    char * baseconfig[] =
    {
	"namelist.json",
	"dispatchnamelist.json",
	"typelist.json",
	"subtypelist.json",
	"baseflag.json",
	"memdb.json",
	"msghead.json",
	"msgrecord.json",
	"expandrecord.json",
	"base_msg.json",
	"return.json",
	"dispatchrecord.json",
	"exmoduledefine.json",
	"sys_conn.json",
	 NULL
    };

    struct start_para start_para;

    start_para.argc=argc;
    start_para.argv=argv;	

    int fd =open(msgaudit_file,O_CREAT|O_RDWR|O_TRUNC,0666);
    close(fd);
    audit_file_init();	
    sys_plugin=getenv("CUBE_SYS_PLUGIN");
//	alloc_init(alloc_buffer);
	struct_deal_init();
	memdb_init();

    	base_define=getenv("CUBE_BASE_DEFINE");
	for(i=0;baseconfig[i]!=NULL;i++)
	{
		Strcpy(namebuffer,base_define);
		Strcat(namebuffer,"/");
		Strcat(namebuffer,baseconfig[i]);
		ret=read_json_file(namebuffer);
		if(ret<0)
			return ret;
		print_cubeaudit("read %d elem from file %s!\n",ret,namebuffer);
	}

	msgfunc_init();
	router_tree_init();
	channel_init();

	ex_module_list_init();

    // init system
    system("mkdir lib");
    // init the main proc struct

    struct lib_para_struct * lib_para=NULL;
    fd=open(sys_config_file,O_RDONLY);
    if(fd>0)
    {

   	 ret=read_json_node(fd,&root_node);
  	 if(ret<0)
		return ret;	
    	 close(fd);
	

    	 ret=read_sys_cfg((void **)&lib_para,root_node,NULL);
    	 if(ret<0)
		return ret;
    }	 		
    fd=open(main_config_file,O_RDONLY);
    if(fd<0)
	return -EINVAL;

    ret=read_json_node(fd,&root_node);
    if(ret<0)
	return ret;	
    close(fd);
	
    ret=read_main_cfg(lib_para,root_node);
    if(ret<0)
	return ret; 		

    ret=get_local_uuid(local_uuid);
    digest_to_uuid(local_uuid,buffer);
    buffer[64]=0;
    print_cubeaudit("this machine's local uuid is %s\n",buffer);
    proc_share_data_setvalue("uuid",local_uuid);

    // init all the proc database

    usleep(time_val.tv_usec);


    // init the connect proc	
    Strcpy(namebuffer,sys_plugin);
    Strcat(namebuffer,"/");
    Strcat(namebuffer,connector_plugin_file);
    plugin_proc.init =main_read_func(namebuffer,"proc_conn_init");
    if(plugin_proc.init==NULL)
	return -EINVAL;
    plugin_proc.start =main_read_func(namebuffer,"proc_conn_start");
    if(plugin_proc.start==NULL)
	return -EINVAL;
     plugin_proc.name=dup_str("connector_proc",0);	
     plugin_proc.type=MOD_TYPE_CONN;
	
     ret=ex_module_create("connector_proc",MOD_TYPE_CONN,NULL,&conn_proc);
    if(ret<0)
	    return ret;

    ex_module_setinitfunc(conn_proc,plugin_proc.init);
    ex_module_setstartfunc(conn_proc,plugin_proc.start);

    ex_module_init(conn_proc,connector_config_file);
    add_ex_module(conn_proc);

    // init the router proc	
    Strcpy(namebuffer,sys_plugin);
    Strcat(namebuffer,"/");
    Strcat(namebuffer,router_plugin_file);
    plugin_proc.init =main_read_func(namebuffer,"proc_router_init");
    if(plugin_proc.init==NULL)
	return -EINVAL;
    plugin_proc.start =main_read_func(namebuffer,"proc_router_start");
    if(plugin_proc.start==NULL)
	return -EINVAL;
     plugin_proc.name=dup_str("router_proc",0);	
     plugin_proc.type=MOD_TYPE_ROUTER;
	
    ret=ex_module_create("router_proc",MOD_TYPE_MONITOR,NULL,&router_proc);
    if(ret<0)
	    return ret;

    ex_module_setinitfunc(router_proc,plugin_proc.init);
    ex_module_setstartfunc(router_proc,plugin_proc.start);

    struct router_init_para
    {
		char * router_file;
		char * aspect_file;
    } router_init;
    router_init.router_file=router_config_file;
    router_init.aspect_file=aspect_config_file;
   	

    ex_module_init(router_proc,&router_init);
	
    print_cubeaudit("prepare the router proc\n");
    ret=add_ex_module(router_proc);
    if(ret<0)
	    return ret;

    // loop to init all the plugin's 

    void * ex_module;	

    fd=open(plugin_config_file,O_RDONLY);
    if(fd>0)
    {	

    	while(read_json_node(fd,&root_node)>0)
    	{  		
		ret=read_plugin_cfg(&ex_module,root_node);
		if(ret>=0)
    			add_ex_module(ex_module);
    	}
    }	
    
/*
    }
  */   
    usleep(time_val.tv_usec);
    print_cubeaudit("prepare the conn proc\n");
    ret=ex_module_start(conn_proc,NULL);
    if(ret<0)
	    return ret;

    // second loop:  start all the monitor process
       	
    ret=get_first_ex_module(&ex_module);

    if(ret<0)
	return ret;
    int active_module_no=0;
    while(ex_module!=NULL)
    {
	  if((ex_module_gettype(ex_module) == MOD_TYPE_MONITOR)
	  	||(ex_module_gettype(ex_module) == MOD_TYPE_PORT))
	  {
  		ret=ex_module_start(ex_module,NULL);
	  	if(ret<0)
  			return ret;
		print_cubeaudit("monitor ex_modulec %s started successfully!\n",ex_module_getname(ex_module));
	  }
	  else if(ex_module_gettype(ex_module) == MOD_TYPE_START)
	  {
  		ret=ex_module_start(ex_module,&start_para);
	  	if(ret<0)
  			return ret;
		print_cubeaudit("start ex_module %s started successfully!\n",ex_module_getname(ex_module));

	  }
	  	
    	  ret= get_next_ex_module(&ex_module);
	  active_module_no++;
    	  if(ret<0)
		return ret;
    }

    if(active_module_no==0)
	return 0;

    proc_share_data_setstate(PROC_LOCAL_START);

    // third loop:  test if any module exit
    int * thread_retval;
    thread_retval=malloc(sizeof(int)*active_module_no);
    if(thread_retval==NULL)
		return -EINVAL;
/*
    i=0;
    while(1)
    {		 
	  usleep(time_val.tv_usec);		
  	  ret=get_first_ex_module(&ex_module);

   	  if(ret<0)
		return ret;
    	 while(ex_module!=NULL)
         {
		if(ex_module_tryjoin(ex_module,&thread_retval[i++])==0)
		{
			print_cubeaudit("monitor ex_modulec %s exited successfully!\n",ex_module_getname(ex_module));
			return thread_retval[i-1];
		}
    	  	ret= get_next_ex_module(&ex_module);
	  }
	  i=0;
	  	
    }
*/
    // third loop:  join all the non_system module 
       	
    ret=get_first_ex_module(&ex_module);

    if(ret<0)
	return ret;
    i=0;
    while(ex_module!=NULL)
    {
	  if((ex_module_gettype(ex_module) == MOD_TYPE_MONITOR)
	  	||(ex_module_gettype(ex_module) == MOD_TYPE_PORT)
	  	||(ex_module_gettype(ex_module) == MOD_TYPE_START))
	  {
		//if(Strcmp(ex_module_getname(ex_module),"router_proc")!=0)
		//{
    	   	ret=ex_module_join(ex_module,&thread_retval[i++]);
	  		if(ret<0)
			{
				print_cubeerr("%s module join error!\n",ex_module_getname(ex_module));
  				return ret;
			}
			print_cubeaudit("%s module exit!\n",ex_module_getname(ex_module));
		//}
	  }
	  	
    	  ret= get_next_ex_module(&ex_module);
    	  if(ret<0)
		return ret;
    }

    printf("thread return value %d!\n",thread_retval[0]);
    return ret;
}
