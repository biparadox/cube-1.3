#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "message.h"
#include "ex_module.h"

#include "sys_func.h"
#include "file_dealer.h"
#include "file_struct.h"


//static struct timeval time_val={0,20*1000};
static int block_size=512;


int file_dealer_init(void * sub_proc,void * para)
{
	int ret=0;
	return ret;
}

int file_dealer_start(void * sub_proc,void * para)
{
	int ret;
	void * recv_msg;
	void * context;
	int i;
	int type;
	int subtype;

	print_cubeaudit("begin file_dealer start process! \n");

	while(1)
	{
		usleep(time_val.tv_usec);
		ret=ex_module_recvmsg(sub_proc,&recv_msg);
		if(ret<0)
			continue;
		if(recv_msg==NULL)
			continue;

 		type=message_get_type(recv_msg);
 		subtype=message_get_subtype(recv_msg);
		if(type==NULL)
		{
			message_free(recv_msg);
			continue;
		}
		if(!memdb_find_recordtype(type,subtype))
		{
			print_cubeerr("message format (%d %d) is not registered!\n",
				message_get_type(recv_msg),message_get_subtype(recv_msg));
			ex_module_sendmsg(sub_proc,recv_msg);
			continue;
		}
		if(type==TYPE(FILE_TRANS))
		{
			if(subtype==SUBTYPE(FILE_TRANS,FILE_DATA))
			{
				proc_file_receive(sub_proc,recv_msg);
			}
			else if(subtype==SUBTYPE(FILE_TRANS,REQUEST))
			{
				proc_file_send(sub_proc,recv_msg);
			}
		}
		else
		{
			ex_module_sendmsg(sub_proc,recv_msg);
		}
	}
	return 0;
};


int _is_samefile_exists(void * record)
{
	struct policyfile_data * pfdata=record;
	char digest[DIGEST_SIZE];
	int fd;
	int ret;
	fd=open(pfdata->filename,O_RDONLY);
	if(fd>0)
	{
		close(fd);
		ret=calculate_sm3(pfdata->filename,digest);
		if(ret<0)
			return ret;
		if(Memcmp(pfdata->uuid,digest,DIGEST_SIZE)!=0)
		{
			return 1;
		}
		return 0;
	}
	return 2;	

}

int get_filedata_from_message(void * message)
{
	struct policyfile_data * pfdata;
	int retval;
	MSG_HEAD * message_head;
	int fd;
        BYTE digest[DIGEST_SIZE];
	int type;
	int subtype;


	pfdata= Talloc(sizeof(struct policyfile_data));
        if(pfdata==NULL)
       		return -ENOMEM;	       
	message_head=message_get_head(message);
	if(message_head==NULL)
		return -EINVAL;
	
	type=message_get_type(message);
	subtype=message_get_subtype(message);
	if((type!=TYPE(FILE_TRANS)) && (subtype!=SUBTYPE(FILE_TRANS,FILE_DATA)))
	{
		return -EINVAL;
	}

	if(message_get_flag(message) &MSG_FLAG_CRYPT)
	{
		print_cubeerr("error! filedata message is crypted!\n");
		return -EINVAL;
	}
	retval=message_get_record(message,&pfdata,0);

	fd=open(pfdata->filename,O_CREAT|O_WRONLY,0666);
	if(fd<0)
		return fd;
	lseek(fd,pfdata->offset,SEEK_SET);
	write(fd,pfdata->file_data,pfdata->data_size);
	close(fd);
	return 0;
}

int proc_file_receive(void * sub_proc,void * message)
{
	struct policyfile_data * pfdata;
	struct policyfile_store * storedata;
	int ret;
	void * send_msg;

	print_cubeaudit("begin file receive!\n");
	char buffer[4096];
	BYTE digest[DIGEST_SIZE];
	int blobsize=0;
	int fd;

	if(message_get_flag(message) &MSG_FLAG_CRYPT)
	{
		print_cubeerr("error! filedata message is crypted!\n");
		return -EINVAL;
	}

	ret=message_get_record(message,&pfdata,0);
	if(ret<0)
		return -EINVAL;
	if(pfdata->total_size==pfdata->data_size)
	{
		// judge if there exists a same-name file 
		switch(ret=_is_samefile_exists(pfdata))
		{
			case 0:     // samefile exists
				print_cubeerr("file %s has existed!\n",pfdata->filename);
				struct policyfile_notice * pfnotice;
				pfnotice=Talloc0(sizeof(struct policyfile_notice));
				if(pfnotice==NULL)
					return -ENOMEM;
				Memcpy(pfnotice->uuid,pfdata->uuid,DIGEST_SIZE);
				pfnotice->filename=dup_str(pfdata->filename,0);
				pfnotice->result=0;	
				send_msg=message_create(TYPE_PAIR(FILE_TRANS,FILE_NOTICE),message);
				if(send_msg==NULL)
					return -EINVAL;
				message_add_record(send_msg,pfnotice);
				ex_module_sendmsg(sub_proc,send_msg);
				return 0;
			case 1:
				print_cubeaudit("overwrite the file %s!\n",pfdata->filename);
				ret=remove(pfdata->filename);
				if(ret<0)
					return ret;
			case 2:
			{
				ret=get_filedata_from_message(message);
				struct policyfile_notice * pfnotice;
				pfnotice=Talloc0(sizeof(struct policyfile_notice));
				if(pfnotice==NULL)
					return -ENOMEM;
				Memcpy(pfnotice->uuid,pfdata->uuid,DIGEST_SIZE);
				pfnotice->filename=dup_str(pfdata->filename,0);
				if(_is_samefile_exists(pfdata)==0)
				{
					pfnotice->result=1;	
				}
				else
				{
					pfnotice->result=0;	
				}
				send_msg=message_create(TYPE_PAIR(FILE_TRANS,FILE_NOTICE),message);
				if(send_msg==NULL)
					return -EINVAL;
				message_add_record(send_msg,pfnotice);
				ex_module_sendmsg(sub_proc,send_msg);
				return ret;
			}
			default:
				return ret;
		}
	}
	else
	{
		DB_RECORD * record;
		record=memdb_find_first(TYPE_PAIR(FILE_TRANS,FILE_STORE),"uuid",pfdata->uuid);
		if(record==NULL)
			storedata=NULL;
		else
			storedata=record->record;
		if(storedata==NULL)
		{
			switch(ret=_is_samefile_exists(pfdata))
			{
				case 0:     // samefile exists
					print_cubeerr("file %s has existed!\n",pfdata->filename);
					return 0;
				case 1:
					print_cubeaudit("overwrite the file %s!\n",pfdata->filename);
					ret=remove(pfdata->filename);
					if(ret<0)
						return ret;
					break;
				case 2:
					break;

				default:
					return ret;
			}
			storedata=Talloc0(sizeof(struct policyfile_store));
			if(storedata==NULL)
				return -ENOMEM;
			Memcpy(storedata->uuid,pfdata->uuid,DIGEST_SIZE);				
			storedata->filename=dup_str(pfdata->filename,0);
			storedata->file_size=pfdata->total_size;
			storedata->block_size=block_size;
			storedata->block_num=(pfdata->total_size+(storedata->block_size-1))/storedata->block_size;
			storedata->mark_len=(storedata->block_num+7)/8;
			storedata->marks=Talloc(storedata->mark_len);
			if(storedata->marks==NULL)
				return -ENOMEM;
			memset(storedata->marks,0,storedata->mark_len);
		}

		int site= pfdata->offset/block_size;
		bitmap_set(storedata->marks,site);
		if(record==NULL)
		{
			memdb_store(storedata,TYPE_PAIR(FILE_TRANS,FILE_STORE),NULL);	
		}
		else
		{
			memdb_store_record(record);	
		}
		ret=get_filedata_from_message(message);
		if(ret<0)
			return ret;
	
		if(bitmap_is_allset(storedata->marks,storedata->block_num))
		{
			print_cubeaudit("get file %s succeed!\n",pfdata->filename);
			struct policyfile_notice * pfnotice;
			pfnotice=Talloc0(sizeof(struct policyfile_notice));
			if(pfnotice==NULL)
				return -ENOMEM;
			Memcpy(pfnotice->uuid,pfdata->uuid,DIGEST_SIZE);
			pfnotice->filename=dup_str(pfdata->filename,0);
			if(_is_samefile_exists(pfdata)==0)
			{
				pfnotice->result=1;	
			}
			else
			{
				pfnotice->result=0;	
			}
			send_msg=message_create(TYPE_PAIR(FILE_TRANS,FILE_NOTICE),message);
			if(send_msg==NULL)
				return -EINVAL;
			message_add_record(send_msg,pfnotice);
			ex_module_sendmsg(sub_proc,send_msg);
			return pfdata->data_size;
		}
	}

	return 0;
}

int proc_file_send(void * sub_proc,void * message)
{

	struct policyfile_data * pfdata;
	struct policyfile_req  * reqdata;
	int ret;
	int fd;
	int data_size;
	int total_size;
	BYTE digest[DIGEST_SIZE];
        struct stat statbuf;
	void * send_msg;
	print_cubeaudit("begin file send!\n");

	int record_no=0;

	ret=message_get_record(message,&reqdata,record_no++);
	if(ret<0)
		return -EINVAL;
	while(reqdata!=NULL)
	{
		if(reqdata->filename!=NULL)
		{
			fd=open(reqdata->filename,O_RDONLY);
			if(fd<0)
				return fd;
        		if(fstat(fd,&statbuf)<0)
        		{
                		print_cubeerr("fstat error\n");
                		return -EINVAL;
        		}
        		total_size=statbuf.st_size;
			close(fd);

			calculate_sm3(reqdata->filename,digest);

			fd=open(reqdata->filename,O_RDONLY);
			if(fd<0)
				return fd;
			int i=0;
		
			for(i=0;i<total_size/block_size;i++)
			{
		
        			pfdata=Talloc0(sizeof(struct policyfile_data));
				if(pfdata==NULL)
				{
					return -ENOMEM;
				}
        			Memcpy(pfdata->uuid,digest,DIGEST_SIZE);
        			pfdata->filename=dup_str(reqdata->filename,0);
        			pfdata->record_no=i;
        			pfdata->offset=i*block_size;
        			pfdata->total_size=total_size;
        			pfdata->data_size=block_size;
				pfdata->file_data=(BYTE *)Talloc0(sizeof(char)*pfdata->data_size);

        			if(read(fd,pfdata->file_data,pfdata->data_size)!=pfdata->data_size)
        			{
               			 	print_cubeerr("read vm list error! \n");
                			return -EINVAL;
        			}

				send_msg=message_create(TYPE_PAIR(FILE_TRANS,FILE_DATA),message);
				if(send_msg==NULL)
					return -EINVAL;
				message_add_record(send_msg,pfdata);

				ex_module_sendmsg(sub_proc,send_msg);
				usleep(50*1000);
			}	
			if((data_size=total_size%block_size)!=0)
			{
        			pfdata=Talloc0(sizeof(struct policyfile_data));
				if(pfdata==NULL)
				{
					return -ENOMEM;
				}
        			Memcpy(pfdata->uuid,digest,DIGEST_SIZE);
        			pfdata->filename=dup_str(reqdata->filename,0);
        			pfdata->record_no=i;
        			pfdata->offset=i*block_size;
        			pfdata->total_size=total_size;
        			pfdata->data_size=data_size;
				pfdata->file_data=(BYTE *)Talloc(sizeof(char)*data_size);

        			if(read(fd,pfdata->file_data,data_size)!=data_size)
        			{
               			 	print_cubeerr("read vm list error! \n");
                			return NULL;
        			}
				send_msg=message_create(TYPE_PAIR(FILE_TRANS,FILE_DATA),message);
				if(send_msg==NULL)
					return -EINVAL;
				message_add_record(send_msg,pfdata);
				ex_module_sendmsg(sub_proc,send_msg);
				usleep(100*1000);
			}
			close(fd);
			print_cubeaudit("send file %s succeed!\n",reqdata->filename);
		}
		ret=message_get_record(message,&reqdata,record_no++);
		if(ret<0)
			return -EINVAL;
	}

	return 0;
}
