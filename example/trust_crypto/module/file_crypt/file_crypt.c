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
#include "file_crypt.h"

#include "../../include/app_struct.h"

static struct timeval time_val={0,50*1000};
int print_error(char * str, int result)
{
	printf("%s %s",str,tss_err_string(result));
}

int file_crypt_init(void * sub_proc,void * para)
{
	return 0;
}

int file_crypt_start(void * sub_proc,void * para)
{
	int ret;
	int retval;
	void * recv_msg;
	void * send_msg;
	void * context;
	void * sock;
	BYTE uuid[DIGEST_SIZE];
	int i;
	int type;
	int subtype;

	// build a  slot for key crypt info
	void * slot_port=slot_port_init("file_crypt",2);
	slot_port_addrecordpin(slot_port,DTYPE_TRUST_DEMO,SUBTYPE_FILE_OPTION);
	slot_port_addmessagepin(slot_port,DTYPE_MESSAGE,SUBTYPE_UUID_RECORD);
	slot_port_addmessagepin(slot_port,DTYPE_TRUST_DEMO,SUBTYPE_FILECRYPT_INFO);
	ex_module_addslot(sub_proc,slot_port);
	// build a  slot for key decrypt info
	slot_port=slot_port_init("file_decrypt",2);
	slot_port_addrecordpin(slot_port,DTYPE_TRUST_DEMO,SUBTYPE_FILE_OPTION);
	slot_port_addmessagepin(slot_port,DTYPE_TRUST_DEMO,SUBTYPE_FILECRYPT_INFO);
	slot_port_addmessagepin(slot_port,DTYPE_MESSAGE,SUBTYPE_UUID_RECORD);
	ex_module_addslot(sub_proc,slot_port);
	printf("begin file crypt start!\n");
	
	sleep(1);
	
	ret=proc_filecrypt_start(sub_proc,para);
	if(ret<0)
		return ret;

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
		
		if(((type==DTYPE_MESSAGE)&&(subtype==SUBTYPE_UUID_RECORD))
		    ||((type==DTYPE_TRUST_DEMO)&&(subtype==SUBTYPE_FILECRYPT_INFO)))	
			
		{
			ret=message_get_uuid(recv_msg,uuid);						
			if(ret<0)
				continue;
			sock=ex_module_findsock(sub_proc,uuid);
			if(sock==NULL)
				continue;	
			ret=slot_sock_addmsg(sock,recv_msg);
			if(ret>0)
				proc_file_deal(sub_proc,sock);
			
		}
	}

	return 0;
};


int crypt_file(char * filename,BYTE * key)
{
	const int buflen=1024;
	int fd;
	int fd1;
	int readlen;
	int cryptlen;
	int offset=0;
	BYTE plain[buflen];
	BYTE *cipher;
	fd=open(filename,O_RDWR);
	if(fd<0)
		return -EINVAL;

	char * tempfilename="tempcrypt.file";

	fd1=open(tempfilename,O_WRONLY|O_CREAT|O_TRUNC,0666);
	if(fd1<0)
		return -EINVAL;	

	readlen=read(fd,plain,buflen);

	while(readlen>0)
	{
		cryptlen=sm4_context_crypt(plain,&cipher,readlen,key);
		write(fd1,cipher,cryptlen);
		readlen=read(fd,plain,buflen);
	}

	close(fd);
	close(fd1);
	remove(filename);
	rename(tempfilename,filename);
	return 0;
	
	
}

int decrypt_file(char * filename,BYTE * key)
{
	const int buflen=1024;
	int fd;
	int fd1;
	int readlen;
	int cryptlen;
	int offset=0;
	BYTE plain[buflen];
	BYTE *cipher;
	fd=open(filename,O_RDWR);
	if(fd<0)
		return -EINVAL;

	char * tempfilename="tempcrypt.file";

	fd1=open(tempfilename,O_WRONLY|O_CREAT|O_TRUNC,0666);
	if(fd1<0)
		return -EINVAL;	

	readlen=read(fd,plain,buflen);

	while(readlen>0)
	{
		cryptlen=sm4_context_decrypt(plain,&cipher,readlen,key);
		write(fd1,cipher,cryptlen);
		readlen=read(fd,plain,buflen);
	}

	close(fd);
	close(fd1);
	remove(filename);
	rename(tempfilename,filename);
	return 0;
}


int proc_filecrypt_start(void * sub_proc,void * para)
{
	int ret;
	int i;
	printf("begin proc filecrypt \n");

	struct start_para * start_para=para;
	struct trust_file_option * file_option;
	struct trust_demo_keyinfo * keyinfo;
	
	file_option=Calloc(sizeof(*file_option));

	if((start_para==NULL) || (start_para->argc !=3))
		return -EINVAL;

	if(start_para->argv[1][0]!='-')
	{
		printf(" error usage! should be %s -(e|d) filename!",start_para->argv[0]);
		return -EINVAL;	
	}

	file_option->filename=dup_str(start_para->argv[2],0);

	switch(start_para->argv[1][1])
	{
		case 'e':
		case 'E':
			file_option->option=dup_str("encrypt",0);
			ret=proc_crypt_requestkey(sub_proc,file_option);
			break;
		case 'd':
		case 'D':
			file_option->option=dup_str("decrypt",0);
			ret=proc_decrypt_requestkey(sub_proc,file_option);
			break;
		default:
			printf(" error usage! should be %s -(e|d) filename!",start_para->argv[0]);
			return -EINVAL;	
	}
	return ret;
}
		
int proc_crypt_requestkey ( void * sub_proc, struct trust_file_option * file_option)
{
	int ret;
	struct trust_demo_keyinfo * keyinfo;
	BYTE uuid[DIGEST_SIZE];
	BYTE local_uuid[DIGEST_SIZE];
	char proc_name[DIGEST_SIZE];
	
	ret=proc_share_data_getvalue("uuid",local_uuid);
	ret=proc_share_data_getvalue("proc_name",proc_name);
	keyinfo=Calloc(sizeof(*keyinfo));
	if(keyinfo==NULL)
		return -ENOMEM;
	Memcpy(keyinfo->vtpm_uuid,local_uuid,DIGEST_SIZE);
	keyinfo->ispubkey=0;
	keyinfo->owner=dup_str(proc_name,DIGEST_SIZE);
	keyinfo->peer=dup_str("",0);
	keyinfo->usage=dup_str("",0);
	keyinfo->passwd=dup_str("kkk",0);
	keyinfo->key_type=TPM_KEY_BIND;
	void * send_msg=message_create(DTYPE_TRUST_DEMO,SUBTYPE_KEY_INFO,NULL);
	if(send_msg==NULL)
		return -EINVAL;
	message_add_record(send_msg,keyinfo);
	void * slot_port=ex_module_findport(sub_proc,"file_crypt");
	if(slot_port==NULL)
		return -EINVAL;

	message_get_uuid(send_msg,uuid);
	void * sock = slot_create_sock(slot_port,uuid);
	ret=slot_sock_addrecorddata(sock,DTYPE_TRUST_DEMO,SUBTYPE_FILE_OPTION,file_option);	
	ex_module_addsock(sub_proc,sock);

	ret=ex_module_sendmsg(sub_proc,send_msg);
	return 0;		

}

int proc_decrypt_requestkey ( void * sub_proc, struct trust_file_option * file_option)
{
	int ret;
	struct trust_filecrypt_info * decrypt_info;
	BYTE uuid[DIGEST_SIZE];
	char local_uuid[DIGEST_SIZE];
	char proc_name[DIGEST_SIZE];
	
	ret=proc_share_data_getvalue("uuid",local_uuid);
	ret=proc_share_data_getvalue("proc_name",proc_name);
	decrypt_info=Calloc(sizeof(*decrypt_info));
	if(decrypt_info==NULL)
		return -ENOMEM;
	Memcpy(decrypt_info->vtpm_uuid,local_uuid,DIGEST_SIZE);
	Memset(decrypt_info->encdata_uuid,0,DIGEST_SIZE);
	Memset(decrypt_info->plain_uuid,0,DIGEST_SIZE);
	calculate_sm3(file_option->filename,decrypt_info->cipher_uuid);
	void * send_msg=message_create(DTYPE_TRUST_DEMO,SUBTYPE_FILECRYPT_INFO,NULL);
	if(send_msg==NULL)
		return -EINVAL;
	message_add_record(send_msg,decrypt_info);
	void * slot_port=ex_module_findport(sub_proc,"file_decrypt");
	if(slot_port==NULL)
		return -EINVAL;

	message_get_uuid(send_msg,uuid);
	void * sock = slot_create_sock(slot_port,uuid);
	ret=slot_sock_addrecorddata(sock,DTYPE_TRUST_DEMO,SUBTYPE_FILE_OPTION,file_option);	
	ex_module_addsock(sub_proc,sock);
	ret=slot_sock_addmsg(sock,send_msg);

	ret=ex_module_sendmsg(sub_proc,send_msg);
	return 0;		

}

int proc_file_deal(void * sub_proc,void * sock)
{
	int ret;
	BYTE * uuidkey;
	struct trust_file_option * file_option;
	struct trust_filecrypt_info * crypt_info;
	void * key_msg;
	void * crypt_msg;
	void * send_msg;

	file_option=slot_sock_removerecord(sock,DTYPE_TRUST_DEMO,SUBTYPE_FILE_OPTION);
	if(file_option==NULL)
		return -EINVAL;
	key_msg=slot_sock_removemessage(sock,DTYPE_MESSAGE,SUBTYPE_UUID_RECORD);
	if(key_msg==NULL)
		return -EINVAL;
	crypt_msg=slot_sock_removemessage(sock,DTYPE_TRUST_DEMO,SUBTYPE_FILECRYPT_INFO);
	if(crypt_msg==NULL)
		return -EINVAL;
	ret=message_get_record(key_msg,&uuidkey,0);
	if(ret<0)
		return -EINVAL;
	if(uuidkey==NULL)
		return -EINVAL;
	ret=message_get_record(crypt_msg,&crypt_info,0);
	if(ret<0)
		return -EINVAL;
	if(crypt_info==NULL)
		return -EINVAL;

	if(Strncmp(file_option->option,"encrypt",DIGEST_SIZE)==0)
	{
		calculate_sm3(file_option->filename,crypt_info->plain_uuid);
		crypt_file(file_option->filename,uuidkey);				
		calculate_sm3(file_option->filename,crypt_info->cipher_uuid);
	}
	else if(Strncmp(file_option->option,"decrypt",DIGEST_SIZE)==0)
	{
		calculate_sm3(file_option->filename,crypt_info->cipher_uuid);
		decrypt_file(file_option->filename,uuidkey);				
		calculate_sm3(file_option->filename,crypt_info->plain_uuid);
	}

	send_msg=message_create(DTYPE_TRUST_DEMO,SUBTYPE_FILECRYPT_INFO,NULL);
	message_add_record(send_msg,crypt_info);
	ex_module_sendmsg(sub_proc,send_msg);

	return 0;	
	
}
