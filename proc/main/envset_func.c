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
#include "json.h"
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

int envset_func(char * envset_file)
{

    int ret;
    int retval;
    int i,j;
    char namebuffer[DIGEST_SIZE*4];

    char * cube_path;
    char * ld_library_path;
    char * app_path;
    char * sys_plugin;		
    char * app_plugin=NULL;		
    char * instance;
    char * cube_define_path;

    char json_buffer[2048];
    int readlen;
    int offset;
    int instance_offset;
    char instance_buffer[1024];
    int instance_count=0;	
    BYTE uuid[DIGEST_SIZE];
    char local_uuid[DIGEST_SIZE*2];

    FILE * fp;
    char buffer[1024];
    void * define_node;
    void * define_elem;
    int fd;	
    int cube_argc;
    char *cube_argv[10];
    int argv_offset=0;
    char * output_file;
    char argv_buffer[DIGEST_SIZE*16];

    //	alloc_init(alloc_buffer);
    struct_deal_init();
    memdb_init();

    fd=open(envset_file,O_RDONLY);
    if(fd<0)
    {
	    print_cubeerr(" can't find instances define file %s!\n",envset_file);
	    return -EINVAL;
    }	

    // read the default define
    read_json_node(fd,&define_node);
    if(define_node ==NULL)
    {
	    print_cubeerr("instance's default define can't find!\n");
	    return -EINVAL;	
    }

    define_elem=json_find_elem("CUBE_PATH",define_node);
    if(define_elem==NULL)
    {
	    print_cubeerr("default instance define format error,no CUBE_PATH!\n");
	    return -EINVAL;
    }			

    offset=json_node_getvalue(define_elem,json_buffer,DIGEST_SIZE*4)+1;
    cube_path=json_buffer; 	
    offset++;	

    // set CUBE_PATH environment
    setenv("CUBE_BASE_DEFINE",namebuffer,1);

    // set CUBELIBPATH environment
    Strncpy(namebuffer,cube_path,DIGEST_SIZE*3);
    Strcat(namebuffer,"/cubelib");
    setenv("CUBELIBPATH",namebuffer,1);

    // set CUBESYSPATH environment
    Strncpy(namebuffer,cube_path,DIGEST_SIZE*3);
    Strcat(namebuffer,"/proc");
    setenv("CUBESYSPATH",namebuffer,1);

    // set CUBE_DEFINE_PATH environment
    define_elem=json_find_elem("CUBE_DEFINE_PATH",define_node);
    if(define_elem==NULL)
    {	
	    Strncpy(json_buffer+offset,cube_path,DIGEST_SIZE*3);
	    Strcat(json_buffer+offset,"/proc/define");
    }
    else
    {
	    ret=json_node_getvalue(define_elem,json_buffer+offset,DIGEST_SIZE*4);
	    if(ret<=0)
		    return -EINVAL;
    }
    cube_define_path=json_buffer+offset;
    setenv("CUBE_DEFINE_PATH",json_buffer+offset,1);
    ret=Strlen(json_buffer+offset);
    offset+=ret+1;

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
    ld_library_path=json_buffer+offset;
    if(getenv("LD_LIBRARY_PATH")==NULL)
    {
	Strcpy(ld_library_path,cube_path);
	Strcat(ld_library_path,"/cubelib/lib");
	offset+=Strlen(ld_library_path)+1;
    }
    else
    {		
   	Strcpy(ld_library_path,getenv("LD_LIBRARY_PATH"));
	Strcat(ld_library_path,cube_path);
	Strcat(ld_library_path,"/cubelib/lib");
    	ret=Strlen(ld_library_path);
    	offset+=ret+1;
    }


    print_cubeaudit("new LD_LIBRARY_PATH is %s\n",ld_library_path);

    read_json_node(fd,&define_node);


	instance_offset=0;
    	instance=instance_buffer+instance_offset;
	// get application's path, if it is empty,use CUBE_PATH
    	define_elem=json_find_elem("CUBE_APP_PATH",define_node);
    	if(define_elem==NULL)
    	{
		Strcpy(instance,cube_path);
    	}			
	else
	{
		app_path=instance_buffer+instance_offset;
    		ret=json_node_getvalue(define_elem,app_path,DIGEST_SIZE*4);
		if(ret<=0)
		{
			print_cubeerr("CUBE_APP_PATH format error!\n");
			return -EINVAL;
		}
		instance_offset+=ret+1;
		instance=instance_buffer+instance_offset;
		Strcpy(instance,app_path);
	}
	// get instance name
    	define_elem=json_find_elem("INSTANCE",define_node);
    	if(define_elem==NULL)
    	{
		print_cubeerr("error format! without instance name!\n");
		return -EINVAL;
    	}
   	ret=json_node_getvalue(define_elem,namebuffer,DIGEST_SIZE*4);
	if(ret<=0)
	{
		print_cubeerr("INSTANCE format error!\n");
		return -EINVAL;
	}
	Strcat(instance,"/");			
	Strcat(instance,namebuffer);
	ret=Strlen(instance);
	instance_offset+=ret+1;	
	
/*
*/
    	// set CUBE_APP_PLUGIN environment
	if(app_path == NULL)
	{
		unsetenv("CUBE_APP_PLUGIN");
	}
	else
	{
    		define_elem=json_find_elem("CUBE_APP_PLUGIN",define_node);
    		if(define_elem==NULL)
    		{
			sprintf(namebuffer,"%s/plugin/",app_path);
			setenv("CUBE_APP_PLUGIN",namebuffer,1);
    		}
		else
		{
   			ret=json_node_getvalue(define_elem,namebuffer,DIGEST_SIZE*4);
			if(ret<=0)
			{
				print_cubeerr("CUBE_APP_PLUGIN format error!\n");
				return -EINVAL;
			}
    			Strcat(instance_buffer+instance_offset,namebuffer);
    			setenv("CUBE_APP_PLUGIN",instance_buffer+instance_offset,1);
		}
	}

    	// reset CUBE_DEFINE_PATH for app

	if(app_path  == NULL)
		setenv("CUBE_DEFINE_PATH",cube_define_path,1);
	else
	{
    		define_elem=json_find_elem("CUBE_DEFINE_PATH",define_node);
    		if(define_elem==NULL)
    		{
			sprintf(namebuffer,"%s/define/:%s",app_path,cube_define_path);
			setenv("CUBE_DEFINE_PATH",namebuffer,1);
    		}
		else
		{
   			ret=json_node_getvalue(define_elem,namebuffer,DIGEST_SIZE*4);
			if(ret<=0)
			{
				print_cubeerr("CUBE_DEFINE_PATH format error!\n");
				return -EINVAL;
			}
			sprintf(instance_buffer+instance_offset,"%s/define/:",app_path);
    			Strcat(instance_buffer+instance_offset,namebuffer);
    			Strcat(instance_buffer+instance_offset,":");
    			Strcat(instance_buffer+instance_offset,cube_define_path);
    			setenv("CUBE_DEFINE_PATH",instance_buffer+instance_offset,1);
		}
	}

    	print_cubeaudit("instance %s's LD_LIBRARY_PATH is %s\n",instance,getenv("LD_LIBRARY_PATH"));
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
				print_cubeerr("CUBE_APP_LIB format error!\n");
				return -EINVAL;
			}
			sprintf(instance_buffer+instance_offset,"%s/locallib/bin/:",app_path);
    			Strcat(instance_buffer+instance_offset,":");
    			Strcat(instance_buffer+instance_offset,namebuffer);
    			Strcat(instance_buffer+instance_offset,":");
    			Strcat(instance_buffer+instance_offset,ld_library_path);
    			setenv("LD_LIBRARY_PATH",instance_buffer+instance_offset,1);
		}
	}

    	print_cubeerr("instance %s's LD_LIBRARY_PATH is %s\n",instance,getenv("LD_LIBRARY_PATH"));
	instance_count++;
    	// set OUTPUT file
    	define_elem=json_find_elem("CUBE_OUTPUT",define_node);
	if(define_elem==NULL)
	{
		unsetenv("CUBE_OUTPUT");
	}
	else
    	{	
	    ret=json_node_getvalue(define_elem,instance_buffer+instance_offset,DIGEST_SIZE*4);
	    if(ret<=0)
		    return -EINVAL;
	    setenv("CUBE_OUTPUT",instance_buffer+instance_offset,1);
        }
	ret=chdir(instance);
	print_cubeaudit("change path %s %d!\n",instance,ret);
/*
*/
			

    return ret;
}




