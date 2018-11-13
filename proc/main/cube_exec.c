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
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "message.h"
#include "connector.h"
#include "ex_module.h"
#include "sys_func.h"

#include "main_proc_func.h"

// this program is the entry of cube instance,
// it will set envirement variable and run one or several cube instance


int main(int argc,char **argv)
{

    int ret;
    int retval;
    int i,j;
    int argv_offset;	
    char namebuffer[DIGEST_SIZE*4];
    void * main_proc; // point to the main proc's subject struct

    char * cube_path;
    char * ld_library_path;
    char * app_path;
    char * sys_plugin;		
    char * app_plugin=NULL;		
    char * instance;

    char json_buffer[4096];
    char print_buffer[4096];
    int readlen;
    int offset;
    int instance_count=0;	
    void * memdb_template ;
    BYTE uuid[DIGEST_SIZE];
    char local_uuid[DIGEST_SIZE*2];

    FILE * fp;
    char audit_text[4096];
    char buffer[4096];
    void * define_node;
    void * define_elem;
    int fd;	

    char * baseconfig[] =
    {
	    "namelist.json",
	    "dispatchnamelist.json",
	    "typelist.json",
	    "subtypelist.json",
	    "memdb.json",
	    "msghead.json",
	    "msgrecord.json",
	    "expandrecord.json",
	    "base_msg.json",
	    "dispatchrecord.json",
	    "exmoduledefine.json",
	    NULL
    };

    if(argc!=2)
    {
	    printf("error format! should be %s [instance define file]\n",argv[0]);
	    return -EINVAL;
    }

    //	alloc_init(alloc_buffer);
    struct_deal_init();
    memdb_init();

    fd=open(argv[1],O_RDONLY);
    if(fd<0)
    {
	    printf(" can't find instances define file %s!\n",argv[1]);
	    return -EINVAL;
    }	

    // read the default define
    read_json_node(fd,&define_node);
    if(define_node ==NULL)
    {
	    printf("instance's default define can't find!\n");
	    return -EINVAL;	
    }

    define_elem=json_find_elem("CUBE_PATH",define_node);
    if(define_elem==NULL)
    {
	    printf("default instance define format error,no CUBE_PATH!\n");
	    return -EINVAL;
    }			

    offset=json_node_getvalue(define_elem,json_buffer,DIGEST_SIZE*4)+1;
    cube_path=json_buffer; 	
    offset++;	

    ld_library_path=json_buffer+offset;
    // set CUBE_PATH environment
    setenv("CUBE_BASE_DEFINE",namebuffer,1);

    // set CUBELIBPATH environment
    Strncpy(namebuffer,cube_path,DIGEST_SIZE*3);
    Strcat(namebuffer,"/cubelib");
    setenv("CUBELIBPATH",namebuffer,1);

    //Strcpy(ld_library_path,namebuffer);	
    //Strcat(ld_library_path,":");	


    // set CUBESYSPATH environment
    Strncpy(namebuffer,cube_path,DIGEST_SIZE*3);
    Strcat(namebuffer,"/proc");
    setenv("CUBESYSPATH",namebuffer,1);

    //Strcat(ld_library_path,namebuffer);	
    //Strcat(ld_library_path,":");	

    // set CUBE_DEFINE_PATH environment
    define_elem=json_find_elem("CUBE_DEFINE_PATH",define_node);
    if(define_elem==NULL)
    {	
	    Strncpy(namebuffer,cube_path,DIGEST_SIZE*3);
	    Strcat(namebuffer,"/proc/define");
	    setenv("CUBE_DEFINE_PATH",namebuffer,1);
    }
    else
    {
	    ret=json_node_getvalue(define_elem,namebuffer,DIGEST_SIZE*4);
	    if(ret<=0)
		    return -EINVAL;
	    setenv("CUBE_DEFINE_PATH",namebuffer,1);
    }

    // set CUBE_SYS_PLUGIN environment
    define_elem=json_find_elem("CUBE_SYS_PLUGIN",define_node);
    if(define_elem==NULL)
    {	
	    Strncpy(namebuffer,cube_path,DIGEST_SIZE*3);
	    Strcat(namebuffer,"/proc/plugin");
	    setenv("CUBE_SYS_PLUGIN",namebuffer,1);
    }
    else
    {
	    ret=json_node_getvalue(define_elem,namebuffer,DIGEST_SIZE*4);
	    if(ret<=0)
		    return -EINVAL;
	    setenv("CUBE_SYS_PLUGIN",namebuffer,1);
    }

    // set CUBE_BASE_DEFINE environment
    define_elem=json_find_elem("CUBE_BASE_DEFINE",define_node);
    if(define_elem==NULL)
    {	
	    Strncpy(namebuffer,cube_path,DIGEST_SIZE*3);
	    Strcat(namebuffer,"/proc/main/base_define");
	    setenv("CUBE_BASE_DEFINE",namebuffer,1);
    }
    else
    {
	    ret=json_node_getvalue(define_elem,namebuffer,DIGEST_SIZE*4);
	    if(ret<=0)
		    return -EINVAL;
	    setenv("CUBE_BASE_DEFINE",namebuffer,1);
    }

    // set LD_LIBRARY_PATH
    //Strcat(ld_library_path,getenv("LD_LIBRARY_PATH"));
    Strcpy(ld_library_path,getenv("LD_LIBRARY_PATH"));
    ret=Strlen(ld_library_path);
    offset+=ret+1;


    printf("new LD_LIBRARY_PATH is %s\n",ld_library_path);

/*
    for(i=0;baseconfig[i]!=NULL;i++)
    {
	    Strcpy(namebuffer,cube_path);
	    Strcat(namebuffer,"/");
	    Strcat(namebuffer,baseconfig[i]);
	    ret=read_json_file(namebuffer);
	    if(ret<0)
		    return ret;
	    print_cubeaudit("read %d elem from file %s!\n",ret,namebuffer);
    }
*/
//    msgfunc_init();
    //	dispatch_init();

    //	ex_module_list_init();

    // read the  instance parameter

    read_json_node(fd,&define_node);

    instance=json_buffer+offset;

    while(define_node!=NULL)
    {
    	define_elem=json_find_elem("CUBE_APP_PATH",define_node);
    	if(define_elem==NULL)
    	{
		Strcpy(instance,cube_path);
    	}			
	else
	{
		app_path=json_buffer+offset;
    		ret=json_node_getvalue(define_elem,app_path,DIGEST_SIZE*4);
		if(ret<=0)
		{
			printf("CUBE_APP_PATH format error!\n");
			return -EINVAL;
		}
		offset+=ret+1;
		instance=json_buffer+offset;
		Strcpy(instance,app_path);
	}
    	define_elem=json_find_elem("INSTANCE",define_node);
    	if(define_elem==NULL)
    	{
		printf("error format! without instance name!\n");
		break;
    	}
   	ret=json_node_getvalue(define_elem,namebuffer,DIGEST_SIZE*4);
	if(ret<=0)
	{
		printf("INSTANCE format error!\n");
		return -EINVAL;
	}
	Strcat(instance,"/");			
	Strcat(instance,namebuffer);
	ret=Strlen(instance);
	offset+=ret+1;	

    	// set CUBE_APP_PLUGIN environment
	if(app_path != NULL)
	{
    		Strncpy(namebuffer,app_path,DIGEST_SIZE*3);
    		Strcat(namebuffer,"/plugin/");
    		setenv("CUBE_APP_PLUGIN",namebuffer,1);
	}

    	// reset LD_LIBRARY_PATH_FOR_app

	if(app_path  == NULL)
		setenv("LD_LIBRARY_PATH",ld_library_path,1);
	else
	{
    		define_elem=json_find_elem("CUBE_APP_LIB",define_node);
    		if(define_elem==NULL)
    		{
			setenv("LD_LIBRARY_PATH",ld_library_path,1);
    		}
		else
		{
   			ret=json_node_getvalue(define_elem,namebuffer,DIGEST_SIZE*4);
			if(ret<=0)
			{
				printf("CUBE_APP_LIB format error!\n");
				return -EINVAL;
			}
    			Strncpy(json_buffer+offset,app_path,DIGEST_SIZE*3);
    			Strcat(json_buffer+offset,"/");
    			Strcat(json_buffer+offset,namebuffer);
    			Strcat(json_buffer+offset,":");
    			Strcat(json_buffer+offset,ld_library_path);
    			setenv("LD_LIBRARY_PATH",json_buffer+offset,1);
		}
	}

    	printf("instance %s's LD_LIBRARY_PATH is %s\n",instance,getenv("LD_LIBRARY_PATH"));
	instance_count++;

	int pid=fork();

	if(pid==0)
	{
		ret=chdir(instance);
		printf("change path %s %d!\n",instance,ret);
		sprintf(json_buffer+offset,"%s/proc/main/main_proc",cube_path);
		ret=execv(json_buffer+offset,NULL);
		perror("execv");
		exit(0);
	}

        read_json_node(fd,&define_node);
	sleep(2);
			
    }


    return ret;
}
