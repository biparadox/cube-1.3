#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <dirent.h>

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

//extern struct timeval time_val={0,50*1000};

int recordlib_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	struct types_pair * types;
	char * type_str;
	char * subtype_str;
	char filename[DIGEST_SIZE*3];
	int  fd;
	DB_RECORD * record;
	
	types=memdb_get_first_record(DTYPE_MESSAGE,SUBTYPE_TYPES);
	
	while(types!=NULL)
	{
		type_str=memdb_get_typestr(types->type);
		if(type_str==NULL)
			return -EINVAL;
		subtype_str=memdb_get_subtypestr(types->type,types->subtype);
		if(subtype_str==NULL)
			return -EINVAL;
		sprintf(filename,"lib/%s-%s.lib",type_str,subtype_str);
		if((fd=open(filename,O_RDONLY))<0)
		{
			types=memdb_get_next_record(DTYPE_MESSAGE,SUBTYPE_TYPES);
			continue;
		}

		while((ret=lib_read(fd,types->type,types->subtype,&record))>0)	
		{
			ret=memdb_store_record(record);
			if(ret<0)
				break;
		}
		close(fd);
		types=memdb_get_next_record(DTYPE_MESSAGE,SUBTYPE_TYPES);
	}	
	return 0;
}

int recordlib_start(void * sub_proc,void * para)
{
	int ret;
	void * recv_msg;
	int i;
	int type;
	int subtype;

	proc_lib_init(sub_proc,NULL);

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
		if(!memdb_find_recordtype(type,subtype))
		{
			print_cubeerr("message format (%d %d) is not registered!\n",
				message_get_type(recv_msg),message_get_subtype(recv_msg));
			continue;
		}
		proc_store_message(sub_proc,recv_msg);
	}
	return 0;
};

int lib_gettype(char * libname, int * typeno,int * subtypeno)
{
		char buffer[DIGEST_SIZE*3];
		char typename[DIGEST_SIZE];
		char subtypename[DIGEST_SIZE];
		int ret;
		int offset=0;

		Strncpy(buffer,libname,DIGEST_SIZE*3);
		ret=Getfiledfromstr(typename,buffer+offset,'-',DIGEST_SIZE*2);
		if(ret<=0)
			return -EINVAL;
		*typeno=memdb_get_typeno(typename);
		if(*typeno<=0)
			return -EINVAL;
		offset+=ret;
		offset++;
		ret=Getfiledfromstr(subtypename,buffer+offset,'.',DIGEST_SIZE*2);
		*subtypeno=memdb_get_subtypeno(*typeno,subtypename);
		if(*subtypeno<0)
			return -EINVAL;
		return 0;
};

int proc_lib_init(void * sub_proc,void * para)
{
	DIR * dir;
	struct dirent *ptr;
	int ret;
	char filename[DIGEST_SIZE*3];
	char buffer[DIGEST_SIZE];
	int typeno;
	int subtypeno;
	int filecount=0;
	int recordcount=0;
	int namelen;
	DB_RECORD * record;
	int fd;

	if ((dir = opendir("lib")) == NULL)
	{
		print_cubeerr("Open lib's dir error...\n");
		return -EIO;
	}
	while ((ptr = readdir(dir)) != NULL)
	{
		if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)    //去除当前目录和上级目录
			continue;
		else
		{
			namelen=Strlen(ptr->d_name);
			if(Strcmp(ptr->d_name+namelen-4,".lib")!=0)
				continue;
			ret=lib_gettype(ptr->d_name,&typeno,&subtypeno);
			if(ret<0)
				return ret;
			Strcpy(filename,"lib/");
			Strcat(filename,ptr->d_name);
			fd=open(filename,O_RDONLY);
			if(fd<0)
			{
				print_cubeerr("read lib file %s error!\n",filename);
				continue;
			}
			recordcount=0;
			while((ret=lib_read(fd,typeno,subtypeno,&record))>0)
			{
				ret=memdb_store_record(record);
				if(ret<0)
				{
					print_cubeerr("read %d record in lib file %s error!\n",recordcount,filename);
					
					break;
				}
				recordcount++;
			}
			print_cubeaudit("read %d record from lib file %s!\n",recordcount,filename);
			filecount++;
		}
	}
	closedir(dir);	
	print_cubeaudit("read %d lib files!\n",filecount);
	return filecount;
}

int proc_store_message(void * sub_proc,void * message)
{
	int type;
	int subtype;
	int i;
	int ret;
	print_cubeaudit("recordlib begin proc store\n");

	type=message_get_type(message);
	subtype=message_get_subtype(message);
	if(type!=DTYPE_MESSAGE)
		return -EINVAL;
	if(subtype!=SUBTYPE_TYPES)
		return -EINVAL;

	struct types_pair * types;
	DB_RECORD * record;
	char * type_str;
	char * subtype_str;
	char filename[DIGEST_SIZE*3];
	int fd;

	i=0;

	ret=message_get_record(message,&types,i++);
	if(ret<0)
		return ret;
	while(types!=NULL)
	{
		type_str=memdb_get_typestr(types->type);
		if(type_str==NULL)
			return -EINVAL;
		subtype_str=memdb_get_subtypestr(types->type,types->subtype);
		if(subtype_str==NULL)
			return -EINVAL;
		sprintf(filename,"lib/%s-%s.lib",type_str,subtype_str);
		fd=open(filename,O_WRONLY|O_CREAT|O_TRUNC,0666);
		if(fd<0)
			return fd;
		
		record=memdb_get_first(types->type,types->subtype);
		while(record!=NULL)		
		{
			ret=lib_write(fd,types->type,types->subtype,record);
			if(ret<0)
				break;
			record=memdb_get_next(types->type,types->subtype);
		}
		close(fd);
		ret=message_get_record(message,&types,i++);
		if(ret<0)
			return ret;
	}
	return ret;
}
