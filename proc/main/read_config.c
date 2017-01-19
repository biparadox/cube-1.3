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
#include "string.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "message.h"
#include "connector.h"
#include "ex_module.h"

#include "main_proc_func.h"


char *  get_temp_filename(char * tag )
{
	char buf[128];
	int len;
	pid_t pid=getpid();
	sprintf(buf,"/tmp/temp.%d",pid);
	len=strlen(buf);
	if(tag!= NULL)
	{
		len+=strlen(tag);
		if(strlen(tag)+strlen(tag)>=128)
			return -EINVAL;
		strcat(buf,tag);
	}
	char * tempbuf = malloc(len+1);
	if(tempbuf==NULL)
		return -EINVAL;
	memcpy(tempbuf,buf,len+1);
	return tempbuf;
}
	
int get_local_uuid(char * uuid)
{
	FILE *fi,*fo;
	int i=0;
	char *s,ch;
	int len;

	char cmd[128];
	char *tempfile1,*tempfile2;

	tempfile1=get_temp_filename(".001");
	if((tempfile1==NULL) || IS_ERR(tempfile1)) 
		return tempfile1;

	sprintf(cmd,"dmidecode | grep UUID | awk '{print $2}' > %s",tempfile1);
	system(cmd);

	fi=fopen(tempfile1,"r");
	memset(uuid,0,DIGEST_SIZE*2);
	len=fread(uuid,1,36,fi);
	sprintf(cmd,"rm -f %s",tempfile1);
	system(cmd);
	return len;
}

int read_json_file(char * file_name)
{
	int ret;

	int fd;
	int readlen;
	int leftlen;
	int json_offset;

	int struct_no=0;
	void * root_node;
	void * findlist;
	void * memdb_template ;
	BYTE uuid[DIGEST_SIZE];
	char json_buffer[4096];

	fd=open(file_name,O_RDONLY);
	if(fd<0)
		return fd;

	readlen=read(fd,json_buffer,1024);
	if(readlen<0)
		return -EIO;
	leftlen=readlen;
	json_buffer[readlen]=0;

	json_offset=0;
	while(leftlen>32)
	{
		ret=json_solve_str(&root_node,json_buffer+json_offset);
		if(ret<0)
		{
			printf("solve json str error!\n");
			break;
		}

		leftlen-=ret;
		json_offset+=ret;

		if(ret<32)
			continue;
		if(leftlen+json_offset==1024)
		{
			Memcpy(json_buffer,json_buffer+json_offset,leftlen);
			readlen=read(fd,json_buffer+leftlen,1024-leftlen);
			if(readlen<0)
				return readlen;
			leftlen+=readlen;
			json_offset=0;
		}

		ret=memdb_read_desc(root_node,uuid);
		if(ret<0)
			break;
		struct_no++;
	}

	close(fd);
	return struct_no;
}

void * main_read_func(char * libname,char * sym)
{
    void * handle;	
    int (*func)(void *,void *);
    char * error;
    handle=dlopen(libname,RTLD_NOW);
     if(handle == NULL)		
     {
    	fprintf(stderr, "Failed to open library %s error:%s\n", libname, dlerror());
    	return NULL;
     }
     func=dlsym(handle,sym);
     if(func == NULL)		
     {
    	fprintf(stderr, "Failed to open func %s error:%s\n", sym, dlerror());
    	return NULL;
     }
     return func;
}     	

int read_sys_cfg(void ** lib_para_struct,void * root_node)
{
    struct lib_para_struct * lib_para;
    int json_offset;	
    int ret;
	
    ret=Galloc0(&lib_para,sizeof(*lib_para));
    if(lib_para==NULL)
	return -ENOMEM;

     void * struct_template=memdb_get_template(DTYPE_EXMODULE,SUBTYPE_LIB_PARA);
    if(struct_template==NULL)
    {
	printf("Fatal error!\n");
	return -EINVAL;
    }
    ret=json_2_struct(root_node,lib_para,struct_template);
    if(ret<0)
    {
	printf("sys config file format error!\n");
	return -EINVAL;
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
		ret=Galloc0(&init_para,struct_size(lib_para->para_template));
		if(init_para==NULL)
			return -ENOMEM;
		ret=json_2_struct(temp_node,init_para,lib_para->para_template);
	}
	ret=ex_module_init(NULL,init_para);
    }
    return ret;
}

int read_json_node(int fd, void ** node)
{
	int ret;

	int readlen;
	int leftlen;
	int json_offset;

	void * root_node;
	char json_buffer[1025];

	readlen=read(fd,json_buffer,1024);
	if(readlen<0)
		return -EIO;
	json_buffer[readlen]=0;

	ret=json_solve_str(&root_node,json_buffer);
	if(ret<0)
	{
		printf("solve json str error!\n");
		return -EINVAL;
	}
	leftlen=readlen-ret;

	lseek(fd,-leftlen,SEEK_CUR);	
	*node=root_node;
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
	ret=Galloc0(&init_para,struct_size(lib_para->para_template));
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
