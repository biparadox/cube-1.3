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

#include "data_type.h"
#include "alloc.h"
#include "string.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"

int lib_read(int fd,int type,int subtype,void ** record)
{
	int ret;
	int offset;
	void * struct_template;
	DB_RECORD * db_record;
	BYTE buffer[2048];
	ret=read(fd,buffer,2048);
	if(ret<0)
		return ret;
	if(ret==0)
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
	lseek(fd,ret-offset,SEEK_CUR);
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
