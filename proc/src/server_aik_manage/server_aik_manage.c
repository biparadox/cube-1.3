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
#include "key_manage_struct.h"

static struct timeval time_val={0,50*1000};

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
		
		if((type==DTYPE_KEY_MANAGE)&&(subtype==SUBTYPE_AIK_BIND))
		{
			proc_record_aik_pair(sub_proc, recv_msg);
		}
		else
		{
			ret = proc_recover_aik_pair();
			if(ret!=1)
				proc_aik_manage_apply_with_userinfo(sub_proc,recv_msg);
		} 
	}

	return 0;
};

/*
通过用户信息申请aik
*/
int proc_aik_manage_apply_with_userinfo(void * sub_proc,void * recv_msg)
{
	TSS_RESULT result;
	TSS_HKEY 	hSignKey;
	TSS_HKEY	hAIKey, hCAKey;
	struct aik_cert_info reqinfo;
	struct policyfile_data * reqdata;
	int ret;
	
	struct aik_user_info * user_info;

	user_info=memdb_get_first_record(DTYPE_AIK_STRUCT,SUBTYPE_AIK_USER_INFO);		//获取从list读入的用户信息
	if(user_info==NULL)
		return -EINVAL;

	memcpy(&reqinfo.user_info,user_info,sizeof(struct aik_user_info));				//构造证书结构

	printf("Finish reading user info");
	
	void * send_msg;																//构造申请消息
	send_msg=message_create(DTYPE_AIK_STRUCT,SUBTYPE_AIK_USER_INFO,NULL);
	if(send_msg==NULL)
		return -EINVAL;
	message_add_record(send_msg,user_info);
	ex_module_sendmsg(sub_proc,send_msg);
	
	printf("Finish sending user info");

	return 0;
}

/*
读取存储的信息
*/
int proc_recover_aik_pair()
{
	DB_RECORD *record;
	struct key_manage_aik_bind *db_aik_bindinfo;
	
	void * struct_template = memdb_get_template(DTYPE_KEY_MANAGE,SUBTYPE_AIK_BIND);	//构造读取模板
	if(struct_template==NULL)
		return -EINVAL;
	
	db_aik_bindinfo = memdb_get_first_record(DTYPE_KEY_MANAGE,SUBTYPE_AIK_BIND);		//从内存数据库中读取记录项
	while(record!=NULL)
	{
		if(db_aik_bindinfo==NULL)
			return 0;
		else
		{
			char local_uuid[DIGEST_SIZE*2+1];		//获取系统当前的uuid和user
			struct aik_user_info * user_info;
			
			proc_share_data_getvalue("uuid",local_uuid);
			user_info=memdb_get_first_record(DTYPE_AIK_STRUCT,SUBTYPE_AIK_USER_INFO);
			
			if( !Memcmp(local_uuid, db_aik_bindinfo->machine_uuid, DIGEST_SIZE) && !strcmp(user_info->user_name, db_aik_bindinfo->user_name))		//对比系统记录项和本机信息，如果一致认为已经持有aik，不再申请。
				return 1;
		}
		record=memdb_get_next(DTYPE_KEY_MANAGE,SUBTYPE_AIK_BIND);
	}
	return 0;
}

/*
存储aik记录项
*/
int proc_record_aik_pair(void * sub_proc,void * recv_msg)
{
	int ret;
	void * send_msg;
	
	struct key_manage_aik_bind *aik_info;
	struct types_pair * types;
	
	ret = message_get_record(recv_msg, &aik_info, 0);		//从消息中获得aik_bind结构
	if(ret<0)
		return ret;
	
	memdb_store(aik_info,DTYPE_KEY_MANAGE,SUBTYPE_AIK_BIND,NULL);		//将信息存储至内存数据库
	
	send_msg=message_create(DTYPE_MESSAGE,SUBTYPE_TYPES,NULL);			//构造存储请求给recordlib，要求其存储信息
	if(send_msg==NULL)
		return -EINVAL;
	types=Talloc(sizeof(*types));
	if(types==NULL)
		return -EINVAL;
	types->type=DTYPE_KEY_MANAGE;
	types->subtype=SUBTYPE_AIK_BIND;
	message_add_record(send_msg,types);
	ex_module_sendmsg(sub_proc,send_msg);
	
	printf("Stored aik info to lib\n");
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
