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

static char * err_file="cube_err.log";
static char * audit_file="cube_audit.log";

int audit_file_init()
{
	int fd;
    	fd =open(audit_file,O_CREAT|O_RDWR|O_TRUNC,0666);
	if(fd<0)
		return fd;
	
    	close(fd);
    	fd =open(err_file,O_CREAT|O_RDWR|O_TRUNC,0666);
	if(fd<0)
		return fd;
    	close(fd);
	return 0;
}

int print_cubeerr(char * format,...)
{
	int fd;
	int len;
	int ret;
	char buffer[DIGEST_SIZE*32];
  	va_list args;
  	va_start (args, format);
  	vsprintf (buffer,format, args);
  	va_end (args);
	len=Strnlen(buffer,DIGEST_SIZE*32);
	
	fd=open(err_file,O_WRONLY|O_APPEND);
	if(fd<0)
		return -EINVAL;
	ret=write(fd,buffer,len);
	return ret;
}

int print_cubeaudit(char * format,...)
{
	int fd;
	int len;
	int ret;
	char buffer[DIGEST_SIZE*32];
  	va_list args;
  	va_start (args, format);
  	vsprintf (buffer,format, args);
  	va_end (args);
	len=Strnlen(buffer,DIGEST_SIZE*32);
	
	fd=open(audit_file,O_WRONLY|O_APPEND);
	if(fd<0)
		return -EINVAL;
	ret=write(fd,buffer,len);
	close(fd);
	return ret;
}

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
	char * tempbuf = Talloc(len+1);
	if(tempbuf==NULL)
		return -EINVAL;
	memcpy(tempbuf,buf,len+1);
	return tempbuf;
}
	
int convert_machine_uuid(char * uuidstr,BYTE * uuid)
{
	int i=0;
	int j=0;
	int len;
	int hflag=0;
	BYTE digit_value;
	char digit;
	BYTE *temp;

	
	len=Strlen(uuidstr);
	if(len<0)
		return 0;	
	if(len>DIGEST_SIZE*2)
		return -EINVAL;
	
	temp=Talloc0(DIGEST_SIZE);
	if(temp==NULL)
		return -ENOMEM;

	for(i=0;i<len;i++)
	{
		digit=uuidstr[i];
		if((digit>='0') &&(digit<='9'))
			digit_value= digit-'0';
		else if((digit>='a') &&(digit<='f'))
			digit_value= digit-'a'+10;
		else if((digit>='A') &&(digit<='F'))
			digit_value =digit-'A'+10;
		else
			continue;
		if(hflag==0)
		{
			temp[j]=digit_value;
			hflag=1;
		}
		else
		{
			temp[j]=(temp[j++]<<4)+digit_value;
			hflag=0;
		}
	}
	calculate_context_sm3(temp,DIGEST_SIZE,uuid);
	Free(temp);
	return DIGEST_SIZE;
}

int get_local_uuid(BYTE * uuid)
{
	FILE *fi,*fo;
	int i=0,j=0;
	char *s,ch;
	int len;
	int ret;

	char uuidstr[DIGEST_SIZE*2];
	
	char cmd[128];
	char *tempfile1,*tempfile2;
	char filename[DIGEST_SIZE*4];
	char *libpath;

	tempfile1=get_temp_filename(".001");
	if((tempfile1==NULL) || IS_ERR(tempfile1)) 
		return tempfile1;

	sprintf(cmd,"dmidecode | grep UUID | awk '{print $2}' > %s",tempfile1);
	ret=system(cmd);

	fi=fopen(tempfile1,"r");
	memset(uuidstr,0,DIGEST_SIZE*2);
	len=fread(uuidstr,1,36,fi);
	sprintf(cmd,"rm -f %s",tempfile1);
	ret=system(cmd);

	if(len==0)
	{
		libpath=getenv("CUBE_SYS_PLUGIN");
		if(libpath==NULL)
			return 0;	
		Strncpy(filename,libpath,DIGEST_SIZE*3);
		Strcat(filename,"/uuid");
		fi=fopen(filename,"r");
		memset(uuidstr,0,DIGEST_SIZE);
		len=fread(uuidstr,1,36,fi);
		if(len<=0)
			return 0;
	}

	len=convert_machine_uuid(uuidstr,uuid);
	return len;
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
		print_cubeerr("solve json str error!\n");
		return -EINVAL;
	}
	leftlen=readlen-ret;

	lseek(fd,-leftlen,SEEK_CUR);	
	*node=root_node;
	return ret;
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

	ret=read_json_node(fd,&root_node);
	if(ret>0)
	{
		while(root_node!=NULL)
		{
			ret=memdb_read_desc(root_node,uuid);
			if(ret<0)
			{	
				print_cubeerr("read %d record format failed!\n",struct_no);
				break;
			}
			struct_no++;
			ret=read_json_node(fd,&root_node);
			if(ret<0)
			{
				print_cubeerr("read %d record json node failed!\n",struct_no);
				break;
			}
			if(ret<32)
				break;
		}
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
int read_record_file(char * record_file)
{
	int type,subtype;
	void * head_node;
	void * record_node;	
	void * elem_node;	
	int fd;
	int ret;
	void * record_template;
	int count=0;
	fd=open(record_file,O_RDONLY);
	if(fd<0)
		return -EIO;
	ret=read_json_node(fd,&head_node);
	if(ret<0)
		return -EINVAL;
	if(head_node==NULL)
		return -EINVAL;
	elem_node=json_find_elem("type",head_node);
	if(elem_node==NULL)
	{
		close(fd);
		return -EINVAL;
	}
	type=memdb_get_typeno(json_get_valuestr(elem_node));
	if(type<=0)
	{
		close(fd);
		return -EINVAL;
	} 			
	elem_node=json_find_elem("subtype",head_node);
	if(elem_node==NULL)
	{
		close(fd);
		return -EINVAL;
	}
	subtype=memdb_get_subtypeno(type,json_get_valuestr(elem_node));
	if(subtype<=0)
	{
		close(fd);
		return -EINVAL;
	} 
	record_template=memdb_get_template(type,subtype);
	if(record_template==NULL)
	{
		close(fd);
		return -EINVAL;
	}		
	
	ret=read_json_node(fd,&record_node);
	int record_size=struct_size(record_template);	
	void * record=NULL;

	while(ret>0)
	{
		if(record==NULL)
			record=Talloc(record_size);
		if(record_node!=NULL)
		{
			ret=json_2_struct(record_node,record,record_template);
			if(ret>=0)
			{
				char * record_name=NULL;
				void * temp_node=json_find_elem("name",record_node);
				if(temp_node!=NULL)
					record_name=json_get_valuestr(temp_node);
				ret=memdb_store(record,type,subtype,record_name);
				if(ret>=0)
				{
					record=NULL;
					count++;
				}
				
			}
		}	
		ret=read_json_node(fd,&record_node);
	}
	close(fd);
	return count; 	
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
