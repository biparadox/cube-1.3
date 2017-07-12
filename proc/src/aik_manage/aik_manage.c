#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>

#include "data_type.h"
#include "errno.h"
#include "alloc.h"
#include "string.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "message.h"
#include "ex_module.h"
#include "tesi.h"

#include "file_struct.h"
#include "tesi_key.h"
#include "tesi_aik_struct.h"
#include "aik_manage.h"

static struct timeval time_val={0,50*1000};
static char pdir[] = {"/home/lib206/cube-1.3/example/trust_crypto/aik_user"};
static char pub_key_dir[] = {"/pubkey"};

int print_error(char * str, int result)
{
	printf("%s %s",str,tss_err_string(result));
}

struct aik_proc_pointer
{
	TSS_HKEY hAIKey;
	TSS_HKEY hSignKey;
	TESI_SIGN_DATA * aik_cert;
};

int aik_manage_init(void * sub_proc,void * para)
{
	int ret;
	TSS_RESULT result;	
	char local_uuid[DIGEST_SIZE*2+1];
	
	struct aik_proc_pointer * aik_pointer;
	void * context;
	aik_pointer= malloc(sizeof(struct aik_proc_pointer));
	if(aik_pointer==NULL)
		return -ENOMEM;
	memset(aik_pointer,0,sizeof(struct aik_proc_pointer));


	result=TESI_Local_ReloadWithAuth("ooo","sss");
	if(result!=TSS_SUCCESS)
	{
		printf("open tpm error %d!\n",result);
		return -ENFILE;
	}
	result= TESI_Local_GetPubEK("pubkey/pubek","ooo");
	if(result!=TSS_SUCCESS)
	{
		printf("get pubek error %d!\n",result);
		return -EIO;
	}
	ret=ex_module_setpointer(sub_proc,aik_pointer);
	if(ret<0)
		return ret;
	return 0;
}

int aik_manage_start(void * sub_proc,void * para)
{
	int ret;
	int retval;
	void * recv_msg;
	void * send_msg;
	void * context;
	int i;
	int type;
	int subtype;

	char local_uuid[DIGEST_SIZE*2+1];
	char proc_name[DIGEST_SIZE*2+1];
	char path[DIGEST_SIZE*3];
	char filename[DIGEST_SIZE*3];
	
	ret=proc_share_data_getvalue("uuid",local_uuid);
	ret=proc_share_data_getvalue("proc_name",proc_name);

	printf("aik manager start!\n");

	for(i=0;i<300*1000;i++)
	{
		usleep(time_val.tv_usec);
		ret=ex_module_recvmsg(sub_proc,&recv_msg);
		if(ret<0)
			continue;
		if(recv_msg==NULL)
			continue;

		type=message_get_type(recv_msg);
		subtype=message_get_subtype(recv_msg);
		
		
		strcat(path,pdir);
		strcat(path,pub_key_dir);
		ret = readFileList(path, filename);
		
		if(ret>0)
		{
			printf(filename);
			printf("\n");
			continue;
		}	
		proc_aik_manage_apply_with_userinfo(sub_proc,recv_msg);
	}

	return 0;
};

int proc_aik_manage_apply_with_userinfo(void * sub_proc,void * recv_msg)
{
	TSS_RESULT result;
	TSS_HKEY 	hSignKey;
	TSS_HKEY	hAIKey, hCAKey;
	struct aik_cert_info reqinfo;
	struct policyfile_data * reqdata;
	int ret;
	
	struct aik_user_info * user_info;

	user_info=memdb_get_first_record(DTYPE_AIK_STRUCT,SUBTYPE_AIK_USER_INFO);
	if(user_info==NULL)
		return -EINVAL;

	memcpy(&reqinfo.user_info,user_info,sizeof(struct aik_user_info));

	printf("Finish reading user info");
	
	void * send_msg;
	send_msg=message_create(DTYPE_AIK_STRUCT,SUBTYPE_AIK_USER_INFO,NULL);
	if(send_msg==NULL)
		return -EINVAL;
	message_add_record(send_msg,user_info);
	ex_module_sendmsg(sub_proc,send_msg);
	
	printf("Finish sending user info");

	return 0;
}

/*
给定目录，回复一个目录下.pem文件的文件名（包含路径）
*/
int readFileList(char *Path, char *filename)  
{  
	int file_num = 0;  
	
	DIR *dir;  
	struct dirent *ptr;    
  
	if ((dir = opendir(Path)) == NULL)  
	{  
		perror("Open dir error...");  
		exit(1);  
	}  
  
	while ((ptr = readdir(dir)) != NULL)  
	{  
		int size = strlen(ptr->d_name);		//检查文件名长度
		if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)    //去除当前目录和上级目录  
			continue;  
		else if (strcmp( ( ptr->d_name + (size - 4) ) , ".pem") == 0)    //读取后缀为“pem”文件路径  
		{  
			strcpy(filename, Path);  
			strcat(filename, ptr->d_name);  
			file_num++;
			break;
		}  
		else   
		{  
			continue;  
		}  
	}  
	closedir(dir);  
	return file_num;  
}

/*int proc_get_exe_pwd( char* pwd )
{
	int count;
	
	count = readlink( "/proc/self/exe", pwd, DIGEST_SIZE*3 );  
	
	if ( count < 0 || count >= DIGEST_SIZE*3 )  
    {   
        printf( "Failed\n" );  
        return -1;  
    }   
	
    pwd[ count ] = '\0';  
    printf( "/proc/self/exe -> [%s]\n", pwd );  
  
    return 0; 
}*/
