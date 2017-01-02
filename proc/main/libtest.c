#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../include/data_type.h"
#include "../include/struct_deal.h"
#include "../include/crypto_func.h"
#include "../include/logic_baselib.h"
#include "../include/policy_ui.h"
#include "../include/message_struct.h"
#include "../include/sec_entity.h"
#include "../include/openstack_trust_lib.h"



int main(int argc,char * argv[])
{

    int ret;
    int retval;
    int i,j;

    FILE * fp;
    char audit_text[4096];
    char filename[128];

    void * struct_template;
    void * record;

    if(argc!=2)
    {
	    printf("Error format! should be %s <libtype>, for example,\n %s PLAI\n",argv[0]);
	    return -EINVAL;
    }

    if(strlen(argv[1])!=4)
    {
	   printf("wrong lib type format!\n"); 
	   return -EINVAL;
    }

    sprintf(filename,"./lib/%s.lib",argv[1]);
    POLICY_HEAD libhead;


    int fd =open(filename,O_RDONLY);
    if(fd<0)
    {
	    printf("open lib error!\n");
	    return -EINVAL;
    }
    ret=read(fd,&libhead,sizeof(POLICY_HEAD));
    if(ret!=sizeof(POLICY_HEAD))
    {
	    printf("lib head error!\n");
	    return -EINVAL;
    }
    close(fd);
    openstack_trust_lib_init();

    ret=register_policy_lib(libhead.PolicyType,&general_lib_ops);
    if(ret<0)
    {
	    printf("register lib error!\n");
	    return -EINVAL;
    }



   	
   struct_template=load_record_template(argv[1]);
   if(struct_template==NULL)
   {
	   printf("read record format error!\n");
	   return -EINVAL;

   }
   ret=LoadPolicy(argv[1]);
   if(ret<0)
   {
	   printf("Load policy %s failed!\n",argv[1]);
	   return -EINVAL;
   }

   ret=GetFirstPolicy(&record,argv[1]);
   int offset=0;
   while(record!=NULL)
   {

	   ret=struct_2_json(record,audit_text,struct_template,&offset);
	   printf("****************************************************\n");
	   printf("%s\n",audit_text);
	   ret=GetNextPolicy(&record,argv[1]);
	   offset=0;
   } 
   printf("****************************************************\n");

   return ret;
}
