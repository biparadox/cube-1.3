#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <dlfcn.h>
#include <stdarg.h> 

#include "data_type.h"
#include "alloc.h"
#include "json.h"
#include "memfunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "message.h"
#include "connector.h"
#include "ex_module.h"

#include "main_proc_func.h"

void * main_read_func(char * libname,char * sym)
{
    void * handle;	
    int (*func)(void *,void *);
    char * error;
    handle=dlopen(libname,RTLD_NOW);
     if(handle == NULL)		
     {
    	print_cubeerr("Failed to open library %s error:%s\n", libname, dlerror());
    	return NULL;
     }
     func=dlsym(handle,sym);
     if(func == NULL)		
     {
    	print_cubeerr("Failed to open func %s error:%s\n", sym, dlerror());
    	return NULL;
     }
     return func;
}     	

int read_sys_cfg(void ** lib_para_struct,void * root_node)
{
    struct lib_para_struct * lib_para;
    int json_offset;	
    int ret;
    char filename[DIGEST_SIZE*4];
	
    lib_para=Salloc0(sizeof(*lib_para));
    if(lib_para==NULL)
	return -ENOMEM;

     void * struct_template=memdb_get_template(DTYPE_EXMODULE,SUBTYPE_LIB_PARA);
    if(struct_template==NULL)
    {
	print_cubeerr("Fatal error!,wrong lib para format!\n");
	return -EINVAL;
    }
    ret=json_2_struct(root_node,lib_para,struct_template);
    if(ret<0)
    {
	print_cubeerr("sys config file format error!\n");
	return -EINVAL;
     }
 
    char * define_path=getenv("CUBE_DEFINE_PATH");	

    void * define_node=json_find_elem("define_file",root_node);	    
    if(define_node!=NULL)
    {
	if(json_get_type(define_node)==JSON_ELEM_STRING)
	{
		ret=read_json_file(json_get_valuestr(define_node));
		if(ret<0)
		{
			Strcpy(filename,define_path);
			Strcat(filename,"/");
			Strcat(filename,json_get_valuestr(define_node));					
			ret=read_json_file(filename);
			if(ret<0)
			{
				print_cubeerr("read define file  %s failed!\n",json_get_valuestr(define_node));
			}
		}
		if(ret>=0)
			print_cubeaudit("read %d elem from file %s!\n",ret,json_get_valuestr(define_node));
	}
	else if(json_get_type(define_node)==JSON_ELEM_ARRAY)
	{
		void * define_file=json_get_first_child(define_node);
		while(define_file!=NULL)
		{
			ret=read_json_file(json_get_valuestr(define_file));
			if(ret<0)
			{
				Strcpy(filename,define_path);
				Strcat(filename,"/");
				Strcat(filename,json_get_valuestr(define_file));					
				ret=read_json_file(filename);
				if(ret<0)
				{
					print_cubeerr("read define file  %s failed!\n",json_get_valuestr(define_file));
				}
			}
			if(ret>=0)
				print_cubeaudit("read %d elem from file %s!\n",ret,json_get_valuestr(define_file));
			define_file=json_get_next_child(define_node);
		}
	}	
    }

    void * para_node=json_find_elem("init_para_desc",root_node);
     
    if(para_node!=NULL)
    {
	  lib_para->para_template=memdb_read_struct_template(para_node);
	 if(lib_para->para_template==NULL)
		return -EINVAL;		
    }
    *lib_para_struct=lib_para;
    return ret;
}


int read_plugin_cfg(void ** plugin,void * root_node)
{
    struct lib_para_struct * lib_para;
    int ret;
    void * temp_node;
    void * lib_node;
    int (*init) (void *,void *);
    int (*start) (void *,void *);
    void * ex_module;	
    int fd;
    char * libname;	
    char filename[512];   
    char * plugin_dir=NULL;
    void * init_para;	

    temp_node=json_find_elem("libname",root_node);
    if(temp_node==NULL)
	return -EINVAL;

    libname=json_get_valuestr(temp_node);
    Strcpy(filename,libname);
    Strcat(filename,".cfg");
   
    fd=open(filename,O_RDONLY);
    if(fd<0)
    {
    	plugin_dir=getenv("CUBE_APP_PLUGIN");
	if(plugin_dir!=NULL)
	{
       		Strcpy(filename,plugin_dir);
		Strcat(filename,"/");
		Strcat(filename,libname);
    		Strcat(filename,".cfg");
    		fd=open(filename,O_RDONLY);
	}
	if(fd<0)
	{
    		plugin_dir=getenv("CUBE_SYS_PLUGIN");
        	Strcpy(filename,plugin_dir);
		Strcat(filename,"/");
		Strcat(filename,libname);
    		Strcat(filename,".cfg");
    		fd=open(filename,O_RDONLY);
		if(fd<0)
			return -EIO;
	}
    }	

    ret=read_json_node(fd,&lib_node);
    if(ret<0)
	return ret;	
    close(fd);

    ret=read_sys_cfg(&lib_para,lib_node);
    if(ret<0)
	return ret;		    	
    temp_node=json_find_elem("name",root_node);
    if(temp_node==NULL)
	return -EINVAL;	
    ret=ex_module_create(json_get_valuestr(temp_node),lib_para->type,NULL,&ex_module);
    if(ret<0)
	return -EINVAL;
    if(lib_para->dynamic_lib==NULL)
	return -EINVAL;
 
    if(plugin_dir==NULL)
    {	
   	 Strcpy(filename,lib_para->dynamic_lib);
    }
    else
    {
   	 Strcpy(filename,plugin_dir);
	 Strcat(filename,"/");
   	 Strcat(filename,lib_para->dynamic_lib);
    }			

    init=main_read_func(filename,lib_para->init_func);
    if(init==NULL)
	return -EINVAL;
    ex_module_setinitfunc(ex_module,init);
    start=main_read_func(filename,lib_para->start_func);
    if(init==NULL)
	return -EINVAL;
    ex_module_setstartfunc(ex_module,start);
    init_para=NULL;
    temp_node=json_find_elem("init_para",root_node);
    if(temp_node!=NULL)
    {
	init_para=Salloc0(struct_size(lib_para->para_template));
	if(init_para==NULL)
		return -ENOMEM;
	ret=json_2_struct(temp_node,init_para,lib_para->para_template);
	if(ret<0)
		return -EINVAL;
    }
    ret=ex_module_init(ex_module,init_para);
    *plugin=ex_module;	  
    return ret;
}
/*

void * main_read_func(char * libname,char * sym)
{
    void * handle;	
    int (*func)(void *,void *);
    char * error;
    handle=dlopen(libname,RTLD_NOW);
     if(handle == NULL)		
     {
    	print_cubeerr(stderr, "Failed to open library %s error:%s\n", libname, dlerror());
    	return NULL;
     }
     func=dlsym(handle,sym);
     if(func == NULL)		
     {
    	print_cubeerr(stderr, "Failed to open func %s error:%s\n", sym, dlerror());
    	return NULL;
     }
     return func;
}     	

int read_sys_cfg(void ** lib_para_struct,void * root_node)
{
    struct lib_para_struct * lib_para;
    int json_offset;	
    int ret;
    char filename[DIGEST_SIZE*4];
	
    lib_para=Salloc0(sizeof(*lib_para));
    if(lib_para==NULL)
	return -ENOMEM;

     void * struct_template=memdb_get_template(DTYPE_EXMODULE,SUBTYPE_LIB_PARA);
    if(struct_template==NULL)
    {
	print_cubeerrprintf("Fatal error!\n");
	return -EINVAL;
    }
    ret=json_2_struct(root_node,lib_para,struct_template);
    if(ret<0)
    {
	printf("sys config file format error!\n");
	return -EINVAL;
     }
 
    char * define_path=getenv("CUBE_DEFINE_PATH");	

    void * define_node=json_find_elem("define_file",root_node);	    
    if(define_node!=NULL)
    {
	if(json_get_type(define_node)==JSON_ELEM_STRING)
	{
		ret=read_json_file(json_get_valuestr(define_node));
		if(ret<0)
		{
			Strcpy(filename,define_path);
			Strcat(filename,"/");
			Strcat(filename,json_get_valuestr(define_node));					
			ret=read_json_file(filename);
			if(ret<0)
			{
				printf("read define file  %s failed!\n",json_get_valuestr(define_node));
			}
		}
		if(ret>=0)
			printf("read %d elem from file %s!\n",ret,json_get_valuestr(define_node));
	}
	else if(json_get_type(define_node)==JSON_ELEM_ARRAY)
	{
		void * define_file=json_get_first_child(define_node);
		while(define_file!=NULL)
		{
			ret=read_json_file(json_get_valuestr(define_file));
			if(ret<0)
			{
				Strcpy(filename,define_path);
				Strcat(filename,"/");
				Strcat(filename,json_get_valuestr(define_file));					
				ret=read_json_file(filename);
				if(ret<0)
				{
					printf("read define file  %s failed!\n",json_get_valuestr(define_file));
				}
			}
			if(ret>=0)
				printf("read %d elem from file %s!\n",ret,json_get_valuestr(define_file));
			define_file=json_get_next_child(define_node);
		}
	}	
    }

    void * para_node=json_find_elem("init_para_desc",root_node);
     
    if(para_node!=NULL)
    {
	  lib_para->para_template=memdb_read_struct_template(para_node);
	 if(lib_para->para_template==NULL)
		return -EINVAL;		
    }
    *lib_para_struct=lib_para;
    return ret;
}


int read_plugin_cfg(void ** plugin,void * root_node)
{
    struct lib_para_struct * lib_para;
    int ret;
    void * temp_node;
    void * lib_node;
    int (*init) (void *,void *);
    int (*start) (void *,void *);
    void * ex_module;	
    int fd;
    char * libname;	
    char filename[512];   
    char * plugin_dir=NULL;
    void * init_para;	

    temp_node=json_find_elem("libname",root_node);
    if(temp_node==NULL)
	return -EINVAL;

    libname=json_get_valuestr(temp_node);
    Strcpy(filename,libname);
    Strcat(filename,".cfg");
   
    fd=open(filename,O_RDONLY);
    if(fd<0)
    {
    	plugin_dir=getenv("CUBE_APP_PLUGIN");
	if(plugin_dir!=NULL)
	{
       		Strcpy(filename,plugin_dir);
		Strcat(filename,"/");
		Strcat(filename,libname);
    		Strcat(filename,".cfg");
    		fd=open(filename,O_RDONLY);
	}
	if(fd<0)
	{
    		plugin_dir=getenv("CUBE_SYS_PLUGIN");
        	Strcpy(filename,plugin_dir);
		Strcat(filename,"/");
		Strcat(filename,libname);
    		Strcat(filename,".cfg");
    		fd=open(filename,O_RDONLY);
		if(fd<0)
			return -EIO;
	}
    }	

    ret=read_json_node(fd,&lib_node);
    if(ret<0)
	return ret;	
    close(fd);

    ret=read_sys_cfg(&lib_para,lib_node);
    if(ret<0)
	return ret;		    	
    temp_node=json_find_elem("name",root_node);
    if(temp_node==NULL)
	return -EINVAL;	
    ret=ex_module_create(json_get_valuestr(temp_node),lib_para->type,NULL,&ex_module);
    if(ret<0)
	return -EINVAL;
    if(lib_para->dynamic_lib==NULL)
	return -EINVAL;
 
    if(plugin_dir==NULL)
    {	
   	 Strcpy(filename,lib_para->dynamic_lib);
    }
    else
    {
   	 Strcpy(filename,plugin_dir);
	 Strcat(filename,"/");
   	 Strcat(filename,lib_para->dynamic_lib);
    }			

    init=main_read_func(filename,lib_para->init_func);
    if(init==NULL)
	return -EINVAL;
    ex_module_setinitfunc(ex_module,init);
    start=main_read_func(filename,lib_para->start_func);
    if(init==NULL)
	return -EINVAL;
    ex_module_setstartfunc(ex_module,start);
    init_para=NULL;
    temp_node=json_find_elem("init_para",root_node);
    if(temp_node!=NULL)
    {
	init_para=Salloc0(struct_size(lib_para->para_template));
	if(init_para==NULL)
		return -ENOMEM;
	ret=json_2_struct(temp_node,init_para,lib_para->para_template);
	if(ret<0)
		return -EINVAL;
    }
    ret=ex_module_init(ex_module,init_para);
    *plugin=ex_module;	  
    return ret;
}
*/
int read_main_cfg(void * lib_para_struct,void * root_node)
{
    struct lib_para_struct * lib_para=lib_para_struct;
    int ret;
    void * temp_node;
    int (*init) (void *,void *);
    temp_node=json_find_elem("proc_name",root_node);
    if(temp_node==NULL)
	return -EINVAL;
    
    void * init_para;
    	
    ret=proc_share_data_setvalue("proc_name",json_get_valuestr(temp_node));
    if(ret<0)
	return ret;

    if(lib_para==NULL)
	return 0;	
    ret=0;

    void * record_list=json_find_elem("record_file",root_node);
    if(record_list!=NULL)
    {
	if(json_get_type(record_list)==JSON_ELEM_STRING)
	{
		ret=read_record_file(json_get_valuestr(record_list));
		if(ret>0)
			print_cubeaudit("read %d elem from file %s!\n",ret,json_get_valuestr(record_list));
	}
	else if(json_get_type(record_list)==JSON_ELEM_ARRAY)
	{
		void * record_file=json_get_first_child(record_list);
		while(record_file!=NULL)
		{
			ret=read_record_file(json_get_valuestr(record_file));
			if(ret>0)
				print_cubeaudit("read %d elem from file %s!\n",ret,json_get_valuestr(record_file));
			record_file=json_get_next_child(record_list);
		}
	}	
    }		

    if(lib_para->dynamic_lib!=NULL)
    {
    	init=main_read_func(lib_para->dynamic_lib,lib_para->init_func);
	if(init==NULL)
		return -EINVAL;
	ex_module_setinitfunc(NULL,init);
        init_para=NULL;
	temp_node=json_find_elem("init_para",root_node);
        if(temp_node!=NULL)
        {
		init_para=Salloc0(struct_size(lib_para->para_template));
		if(init_para==NULL)
			return -ENOMEM;
		ret=json_2_struct(temp_node,init_para,lib_para->para_template);
	}
	ret=ex_module_init(NULL,init_para);
    }
    return ret;
}
