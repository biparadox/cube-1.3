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
#include "http_server.h"
#include "http_fileserver_agent.h"


//static struct timeval time_val={0,20*1000};

int http_fileserver_agent_init(void * sub_proc,void * para)
{
	int ret=0;
	return ret;
}

int http_fileserver_agent_start(void * sub_proc,void * para)
{
	int ret;
	void * recv_msg;
	void * context;
	int i;
	int type;
	int subtype;

	print_cubeaudit("begin http_fileserver_agent start process! \n");

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
		if(type==TYPE(HTTP_SERVER))
		{
			if(subtype==SUBTYPE(HTTP_SERVER,SERVER))
			{
				proc_http_server_register(sub_proc,recv_msg);
			}
			else if(subtype==SUBTYPE(HTTP_SERVER,ACTION))
			{
				proc_http_server_action(sub_proc,recv_msg);
			}
            		else
            		{
			    ex_module_sendmsg(sub_proc,recv_msg);
            		}
		}
		else
		{
			ex_module_sendmsg(sub_proc,recv_msg);
		}
	}
	return 0;
};


int _is_samefile_exists(RECORD(HTTP_SERVER,FILE) * http_file, void * record)
{
	char digest[DIGEST_SIZE];
	int fd;
	int ret;
	fd=open(http_file->file_name,O_RDONLY);
	if(fd>0)
	{
		close(fd);
		ret=calculate_sm3(http_file->file_name,digest);
		if(ret<0)
			return -EINVAL;
		if(Memcmp(http_file->uuid,digest,DIGEST_SIZE)!=0)
		{
			return ENUM(HTTP_SERVER_RESULT,OCCUPY);
		}
		else
			return ENUM(HTTP_SERVER_RESULT,EXIST);
	}
	return ENUM(HTTP_SERVER_RESULT,EMPTY);	

}

int proc_http_server_register(void * sub_proc,void * recv_msg)
{

	int ret;
	RECORD(HTTP_SERVER,SERVER) * http_server;

	ret=message_get_record(recv_msg,&http_server,0);
	if(ret<0)
		return -EINVAL;
	memdb_store(http_server,TYPE_PAIR(HTTP_SERVER,SERVER),http_server->server_name);
	
	void * type_msg = message_gen_typesmsg(TYPE_PAIR(HTTP_SERVER,SERVER),NULL);
	if(type_msg!=NULL)
	{
		ex_module_sendmsg(sub_proc,type_msg);
	}

	return 0;
}
		
	

int proc_http_server_action(void * sub_proc,void * recv_msg)
{
	int ret;
	RECORD(HTTP_SERVER,ACTION) * file_action;
	ret=message_get_record(recv_msg,&file_action,0);
	if(ret<0)
		return -EINVAL;
	if(file_action->action == ENUM(HTTP_SERVER_ACTION,START))
	{
		ret = proc_http_server_start(sub_proc,recv_msg,file_action);
	}	
	else if(file_action->action == ENUM(HTTP_SERVER_ACTION,DOWNLOAD))
	{
		ret = proc_http_file_download(sub_proc,recv_msg,file_action);
	}	
	else if(file_action->action == ENUM(HTTP_SERVER_ACTION,UPLOAD))
	{
		ret = proc_http_file_upload(sub_proc,recv_msg,file_action);
	}	
	else if(file_action->action == ENUM(HTTP_SERVER_ACTION,CHECK))
	{
		ret = proc_http_file_check(sub_proc,recv_msg,file_action);
	}	

	return ret;
}

int proc_http_server_start(void * sub_proc,void * recv_msg,
	       RECORD(HTTP_SERVER,ACTION) * file_action)
{
	int ret;
	char cmd_buf[DIGEST_SIZE*8];
	char short_buf[DIGEST_SIZE];
	RECORD(HTTP_SERVER,SERVER) * http_server;

	DB_RECORD * db_record;

	if(Isemptyuuid(file_action->uuid))
	{
		db_record = memdb_find_byname(file_action->server_name,TYPE_PAIR(HTTP_SERVER,SERVER));
		if(db_record == NULL)
			return -EINVAL;
	}
	else
	{
		db_record = memdb_find_first(TYPE_PAIR(HTTP_SERVER,SERVER),"uuid",
				file_action->uuid);
		if(db_record == NULL)
			return -EINVAL;

	}
	http_server = db_record->record;

	//Strcpy(cmd_buf,"python3 -m ./http_server2.py");
	Strcpy(cmd_buf,"python3 ./http_server2.py");
	if(http_server->port > 0)
	{
		Itoa(http_server->port,short_buf);
		Strcat(cmd_buf," -port ");
		Strcat(cmd_buf,short_buf);
	}
	if(http_server->ip_addr != NULL)
	{
		Strcat(cmd_buf," -ip_addr ");
		Strcat(cmd_buf,http_server->ip_addr);
	}
	if(http_server->server_dir != NULL)
	{
		if(http_server->server_dir[0]!=0)
		{
			Strcat(cmd_buf," -path ");
			Strcat(cmd_buf,http_server->server_dir);
		}
	} 
	print_cubeaudit("http_fileserver_agent: exec cmd %s",cmd_buf);
	system(cmd_buf);
	return 0;
}

int proc_http_file_download(void * sub_proc,void * recv_msg,
	       RECORD(HTTP_SERVER,ACTION) * file_action)
{
	int ret;
	char cmd_buf[DIGEST_SIZE*8];
	char short_buf[DIGEST_SIZE];
	RECORD(HTTP_SERVER,SERVER) * http_server;

	DB_RECORD * db_record;

	db_record = memdb_find_byname(file_action->server_name,TYPE_PAIR(HTTP_SERVER,SERVER));
	if(db_record == NULL)
		return -EINVAL;

	http_server = db_record->record;

	Strcpy(cmd_buf,"curl -O http://");
	if(http_server->ip_addr != NULL)
		Strcat(cmd_buf,http_server->ip_addr);
	else
		Strcat(cmd_buf,"127.0.0.1");
	if(http_server->port > 0)
	{
		Itoa(http_server->port,short_buf);
		Strcat(cmd_buf,":");
		Strcat(cmd_buf,short_buf);
	}
	else
		Strcat(cmd_buf,":8000");
	Strcat(cmd_buf,"/");
	Strcat(cmd_buf,file_action->file_name);
	system(cmd_buf);
	print_cubeaudit("http_fileserver_agent: exec cmd %s",cmd_buf);
	return 0;
}

int proc_http_file_upload(void * sub_proc,void * recv_msg,
	       RECORD(HTTP_SERVER,ACTION) * file_action)
{
	int ret;
	char cmd_buf[DIGEST_SIZE*8];
	char short_buf[DIGEST_SIZE];
	RECORD(HTTP_SERVER,SERVER) * http_server;

	DB_RECORD * db_record;

	db_record = memdb_find_byname(file_action->server_name,TYPE_PAIR(HTTP_SERVER,SERVER));
	if(db_record == NULL)
		return -EINVAL;

	http_server = db_record->record;

	Strcpy(cmd_buf,"curl -F \"file=@");
	Strcat(cmd_buf,file_action->file_name);
	Strcat(cmd_buf,"\"");


	Strcat(cmd_buf," http://");
	if(http_server->ip_addr != NULL)
		Strcat(cmd_buf,http_server->ip_addr);
	else
		Strcat(cmd_buf,"127.0.0.1");
	if(http_server->port > 0)
	{
		Itoa(http_server->port,short_buf);
		Strcat(cmd_buf,":");
		Strcat(cmd_buf,short_buf);
	}
	else
		Strcat(cmd_buf,":8000");
	system(cmd_buf);
	print_cubeaudit("http_file_agent: exec %s",cmd_buf);
	return 0;
}

int proc_http_file_check(void * sub_proc,void * recv_msg,
	       RECORD(HTTP_SERVER,ACTION) * file_action)
{
	int ret;
	char cmd_buf[DIGEST_SIZE*8];
	char short_buf[DIGEST_SIZE];
	RECORD(HTTP_SERVER,SERVER) * http_server;

	DB_RECORD * db_record;

	db_record = memdb_find_byname(file_action->server_name,TYPE_PAIR(HTTP_SERVER,SERVER));
	if(db_record == NULL)
		return -EINVAL;

	http_server = db_record->record;

	Strcpy(cmd_buf,"curl -O http://");
	if(http_server->ip_addr != NULL)
		Strcat(cmd_buf,http_server->ip_addr);
	else
		Strcat(cmd_buf,"127.0.0.1");
	if(http_server->port > 0)
	{
		Itoa(http_server->port,short_buf);
		Strcat(cmd_buf,":");
		Strcat(cmd_buf,short_buf);
	}
	else
		Strcat(cmd_buf,":8000");
	Strcat(cmd_buf,"/");
	Strcat(cmd_buf,file_action->file_name);
	system(cmd_buf);
	return 0;
}

/*

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
*/
