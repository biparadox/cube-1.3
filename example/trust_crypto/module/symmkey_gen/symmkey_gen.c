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
#include "symmkey_gen.h"

#include "../../include/app_struct.h"

static struct timeval time_val={0,50*1000};
int print_error(char * str, int result)
{
	printf("%s %s",str,tss_err_string(result));
}

int symmkey_gen_init(void * sub_proc,void * para)
{
	int ret;
	TSS_RESULT result;	
	char local_uuid[DIGEST_SIZE];
	
	result=TESI_Local_ReloadWithAuth("ooo","sss");
	if(result!=TSS_SUCCESS)
	{
		printf("open tpm error %d!\n",result);
		return -ENFILE;
	}

	// prepare the slot sock
	return 0;
}

int symmkey_gen_start(void * sub_proc,void * para)
{
	int ret;
	int retval;
	void * recv_msg;
	void * send_msg;
	void * context;
	int i;
	int type;
	int subtype;
	void * sock;	
	BYTE uuid[DIGEST_SIZE];

	char local_uuid[DIGEST_SIZE];
	char proc_name[DIGEST_SIZE];
	
	ret=proc_share_data_getvalue("uuid",local_uuid);
	ret=proc_share_data_getvalue("proc_name",proc_name);

	printf("begin tpm key manage1 start!\n");

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
		
		if((type==DTYPE_TESI_KEY_STRUCT)&&(subtype==SUBTYPE_WRAPPED_KEY))
		{
			proc_symmkey_gen(sub_proc,recv_msg);
		}
		else
		{
	//		message_set_activemsg(recv_msg,recv_msg);
			ex_module_sendmsg(sub_proc,recv_msg);
			recv_msg=NULL;
		}
	}

	return 0;
};

void * find_key_info(void * key_info)
{
	struct trust_demo_keyinfo * in_keyinfo=key_info;
	struct trust_demo_keyinfo * db_keyinfo;
	DB_RECORD * record;
	void * comp_template;
	int ret;
	comp_template=memdb_get_template(DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO);
	ret=struct_set_flag(comp_template,CUBE_ELEM_FLAG_TEMP,"vtpm_uuid,owner,peer,usage,key_type");
	if(ret<0)
		return NULL;
	record=memdb_get_first(DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO);
	while(record!=NULL)
	{
		db_keyinfo=record->record;
		if(db_keyinfo==NULL)
			return NULL;
		if(struct_part_compare(db_keyinfo,in_keyinfo,comp_template,CUBE_ELEM_FLAG_TEMP))
			break;
		record=memdb_get_next(DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO);
	}

	return record;
}

int proc_symmkey_gen(void * sub_proc,void * recv_msg)
{
	int ret=0;
	int i=0;
	struct trust_encdata_info * encdatainfo;
	struct vTPM_wrappedkey    * key_struct;	
	void * send_msg;


	TSS_HKEY hEnckey; 

	BYTE uuid[DIGEST_SIZE];
	DB_RECORD * keyinfo_record;
	DB_RECORD * key_record;
	BYTE EncData[DIGEST_SIZE];
	BYTE uuidstr[DIGEST_SIZE*2];

	char filename[DIGEST_SIZE*4];
	char buffer[DIGEST_SIZE*8];

	// get message
	ret=message_get_record(recv_msg,&key_struct,0);
	if(key_struct==NULL)
		return -EINVAL;
			
	encdatainfo=Calloc(sizeof(*encdatainfo));
	if(encdatainfo==NULL)
		return -ENOMEM;	

	key_record=memdb_find_first(DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,"uuid",key_struct->uuid);
	if(key_record==NULL)
		return -EINVAL;
	hEnckey=key_record->tail;	
	
	TESI_Local_GetRandom(EncData,DIGEST_SIZE);
	
	int fd=open("encdata/tempkey.dat",O_RDWR|O_CREAT|O_TRUNC,0666);
	if(fd<0)
		return -EINVAL;	
	write(fd,EncData,DIGEST_SIZE);
	close(fd);
	TESI_Local_Bind("encdata/tempkey.dat",hEnckey,"encdata/symmkey.dat");
	convert_uuidname("encdata/symmkey",".dat",encdatainfo->uuid,filename);
	Memcpy(encdatainfo->vtpm_uuid,key_struct->vtpm_uuid,DIGEST_SIZE);
	Memcpy(encdatainfo->enckey_uuid,key_struct->uuid,DIGEST_SIZE);
	
	getcwd(buffer,DIGEST_SIZE*5-6);
	Strcat(buffer,"/");	
	Strcat(buffer,filename);	
	
	encdatainfo->filename=dup_str(buffer,DIGEST_SIZE*7);

	send_msg=message_create(DTYPE_TRUST_DEMO,SUBTYPE_ENCDATA_INFO,recv_msg);
	if(send_msg==NULL)
		return -EINVAL;
	message_add_record(send_msg,encdatainfo);
	memdb_store(encdatainfo,DTYPE_TRUST_DEMO,SUBTYPE_ENCDATA_INFO,NULL);
	ex_module_sendmsg(sub_proc,send_msg);	
	ex_module_sendmsg(sub_proc,recv_msg);	
	return ret;
}
