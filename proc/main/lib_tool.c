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
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "message.h"
#include "connector.h"
#include "ex_module.h"

#include "main_proc_func.h"

typedef struct proc_init_parameter
{
	char * name;
	int type;
	int (* init) (void *,void *);
	int (* start) (void *,void *);
}PROC_INIT;

static char main_config_file[DIGEST_SIZE*2]="./main_config.cfg";
static char sys_config_file[DIGEST_SIZE*2]="./sys_config.cfg";

int lib_read(int fd,int type,int subtype,void ** record)
{
	int ret;
	int offset;
	int read_size;
	void * struct_template;
	DB_RECORD * db_record;
	BYTE buffer[2048];
	read_size=read(fd,buffer,2048);
	if(read_size<0)
		return read_size;
	if(read_size==0)
		return 0;

	struct_template=memdb_get_template(type,subtype);
	if(struct_template==NULL)
		return -EINVAL;

	*record=Talloc0(struct_size(struct_template));
	if(*record==NULL)
		return -ENOMEM;
	
	offset=blob_2_struct(buffer,*record,struct_template);
	if(offset<0)
		return -EINVAL;
	lseek(fd,offset-read_size,SEEK_CUR);
	return 1;
}

int lib_write(int fd, int type,int subtype, void * record)
{
	int ret;
	int offset;
	void * struct_template;
	BYTE buffer[2048];

	if(record==NULL)
		return 0;



	struct_template=memdb_get_template(type,subtype);
	if(struct_template==NULL)
		return -EINVAL;

	offset=struct_2_blob(record,buffer,struct_template);
	if(offset<0)
		return -EINVAL;
	ret=write(fd,buffer,offset);
	if(ret<0)
		return ret;
	return 1;
}
int main(int argc,char **argv)
{

    int ret;
    int retval;
    int i,j;
    int argv_offset;	
    char namebuffer[DIGEST_SIZE*4];
    void * main_proc; // point to the main proc's subject struct
    char * sys_plugin;		
    char * app_plugin;		
    char * base_define;

    char json_buffer[4096];
    char print_buffer[4096];
    int readlen;
    int json_offset;
    void * memdb_template ;
    BYTE uuid[DIGEST_SIZE];
    char local_uuid[DIGEST_SIZE*2];

    FILE * fp;
    char audit_text[4096];
    char buffer[4096];
    void * root_node;
    void * temp_node;
    PROC_INIT plugin_proc; 
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

    struct start_para start_para;

    start_para.argc=argc;
    start_para.argv=argv;	

    sys_plugin=getenv("CUBE_SYS_PLUGIN");
//    if(sys_plugin==NULL)
//	return -EINVAL;		
//    app_plugin=getenv("CUBE_APP_PLUGIN");
    // process the command argument

   int ifmerge=0;

    if(argc>=2)
    {
	argv_offset=1;
	if(strcmp(argv[1],"merge")==0)
	{
		ifmerge=1;
		if(argc<=3)
		{
			printf("error lib_tool merge format! should be %s merge destdir srcdir1 srcdir2 ...\n",argv[0]);
		}
	}
	else if(strcmp(argv[1],"show"))
	{
		if(argc<3)
		printf("error format! should be %s show MEMDB_TYPE MEMDB_SUBTYPE ...\n",argv[0]);
		return -EINVAL;
	}
    }

      
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
		printf("read %d elem from file %s!\n",ret,namebuffer);
	}


	msgfunc_init();
	dispatch_init();

	ex_module_list_init();

    struct lib_para_struct * lib_para=NULL;
    fd=open(sys_config_file,O_RDONLY);
    if(fd>0)
    {

   	 ret=read_json_node(fd,&root_node);
  	 if(ret<0)
		return ret;	
    	 close(fd);
	

    	 ret=read_sys_cfg(&lib_para,root_node);
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
    printf("this machine's local uuid is %s\n",local_uuid);
    proc_share_data_setvalue("uuid",local_uuid);


	if(!ifmerge)
	{
		int typeno,subtypeno;
		void * record;
		DB_RECORD * db_record;
		typeno=memdb_get_typeno(argv[1]);
		if(typeno<=0)
			return -EINVAL;
		subtypeno=memdb_get_subtypeno(typeno,argv[2]);
		if(subtypeno<0)
			return -EINVAL;

		sprintf(namebuffer,"lib/%s-%s.lib",argv[1],argv[2]);
		if((fd=open(namebuffer,O_RDONLY))<0)
		{
			printf("Error! memdb (%s,%s) does not exist!\n",argv[1],argv[2]);
			return -EINVAL;
		}

		while((ret=lib_read(fd,typeno,subtypeno,&record))>0)	
		{
			ret=memdb_store(record,typeno,subtypeno,NULL);
			if(ret<0)
				break;
		}
		close(fd);
	
		db_record=memdb_get_first(typeno,subtypeno);
		while(db_record!=NULL)
		{
			memdb_print(db_record,json_buffer);
			printf("%s\n",json_buffer);
			db_record=memdb_get_next(typeno,subtypeno);
		}
	}	

    return ret;
}
