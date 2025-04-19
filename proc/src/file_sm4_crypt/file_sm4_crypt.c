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
#include "cube.h"
#include "cube_define.h"
#include "cube_record.h"
#include "file_sm4_crypt.h"

BYTE Buf[DIGEST_SIZE*32];
BYTE default_key[DIGEST_SIZE/2]="123456";
BYTE default_iv[16] = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10};
char file_dir[DIGEST_SIZE*2+1];

int file_sm4_crypt_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
    	struct init_struct * init_para=para;
	print_cubeaudit("init_para %p\n",para);
    	if(para!=NULL)
        {		
		print_cubeaudit("file_sm4_crypt: set default key %s",init_para->default_key);
		Memset(default_key,0,DIGEST_SIZE);	   
		Strncpy(default_key,init_para->default_key,DIGEST_SIZE);
		Memset(file_dir,0,DIGEST_SIZE*2+1);	   
		if(init_para->file_dir!=NULL)
			Strncpy(file_dir,init_para->file_dir,DIGEST_SIZE*2);
	}
	else
	{
		print_cubeaudit("file_sm4_crypt: not set default key, use \"123456\" as default key");
	}
	return 0;
}

int file_sm4_crypt_start(void * sub_proc,void * para)
{
	int ret;
	void * recv_msg;
	int i;
	int type;
	int subtype;


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
			printf("message format (%d %d) is not registered!\n",
				message_get_type(recv_msg),message_get_subtype(recv_msg));
			ex_module_sendmsg(sub_proc,recv_msg);
			continue;
		}
		else if((type==TYPE(GENERAL_RETURN)) && (subtype==SUBTYPE(GENERAL_RETURN,UUID)))
		{
			ret=proc_crypt_file(sub_proc,recv_msg);
			if(ret==MSG_CTRL_EXIT)
				return MSG_CTRL_EXIT;
		}
		else
		{
			ret=proc_crypt_file_expand(sub_proc,recv_msg);
			if(ret==0)
				ex_module_sendmsg(sub_proc,recv_msg);
		}
	}
	return 0;
};

int proc_crypt_file(void *sub_proc, void *recv_msg)
{
    void        *new_msg;
    char        file_buf[DIGEST_SIZE*4] = {0};
    char        output_file_buf[DIGEST_SIZE*4] = {0};
    int ret;
 
    int crypt_mode=0;   // 0: do nothing
                        // 1: crypt
			// 2: decrypt    


    MSG_EXPAND  *msg_expand;
    char cmd_buf[DIGEST_SIZE/2] = {0};
    char crypt_key[DIGEST_SIZE] = {0};
    int offset;

    RECORD(GENERAL_RETURN,UUID)         *cmd;
	
    // get crypt cmd from message
    if ((ret = message_get_record(recv_msg, &cmd, 0)) < 0)
        return ret;

    // get command from message

    offset = Getfiledfromstr(cmd_buf,cmd->name,':',DIGEST_SIZE/2);
    if(offset<0)
	    return -EINVAL;

    if(file_dir[0]!=0)
    {
    	 Strncpy(file_buf,cmd->name+offset+1,DIGEST_SIZE*4);
    }
    else
    	Strncpy(file_buf,cmd->name+offset+1,DIGEST_SIZE*4);
       
   // check mode and build output file name
   if(Strncmp(cmd_buf,"encrypt",DIGEST_SIZE/2)==0)
   {
		crypt_mode=1;   
		Strncpy(output_file_buf,file_buf,DIGEST_SIZE*4-5);
		Strcat(output_file_buf,".sm4");
   }
   else if(Strncmp(cmd_buf,"decrypt",DIGEST_SIZE/2)==0)
   {
		crypt_mode=2; 
	        offset = Strnlen(file_buf,DIGEST_SIZE*4-1);
		if(Strcmp(file_buf+offset-4,".sm4")!=0)
		{
			print_cubeerr("file_sm4_crypt: decrypt file name is not *.sm4 format!");
			return -EINVAL;
		}	
		Strncpy(output_file_buf,file_buf,offset-4);
   }

   // get crypt key

   if(!Isemptyuuid(cmd->return_value))
   {
	Memcpy(crypt_key,cmd->return_value,DIGEST_SIZE);
   }
   else
   {
	Memcpy(crypt_key,default_key,DIGEST_SIZE/2);
	Memcpy(crypt_key+DIGEST_SIZE/2,default_iv,DIGEST_SIZE/2);
   }
	
    /* Start encrypt/decrypt  file */
    if ((ret = _datademo_file_sm4_crypt(file_buf, output_file_buf,crypt_key,crypt_mode,cmd->return_value)) < 0)
    	return -EINVAL;
    
    if(crypt_mode)
    {
	    Free(cmd->name);
	    cmd->name=dup_str(output_file_buf,DIGEST_SIZE*4);
    }	

    new_msg = message_create(TYPE_PAIR(GENERAL_RETURN,UUID),recv_msg);
    if(new_msg ==NULL)
        return -EINVAL;
    message_add_record(new_msg,cmd);
  
    return ex_module_sendmsg(sub_proc, new_msg);
}

int proc_crypt_file_expand(void *sub_proc, void *recv_msg)
{
    void        *new_msg;
    char        file_buf[DIGEST_SIZE*4] = {0};
    char        output_file_buf[DIGEST_SIZE*4] = {0};
    int ret;
 
    int crypt_mode=0;   // 0: do nothing
                        // 1: crypt
			// 2: decrypt    


    MSG_EXPAND  *msg_expand;
    char cmd_buf[DIGEST_SIZE/2] = {0};
    char crypt_key[DIGEST_SIZE] = {0};
    int offset;

    RECORD(GENERAL_RETURN,UUID)         *cmd;
	
    // get crypt cmd from message
    if ((ret = message_remove_expand(recv_msg, TYPE_PAIR(GENERAL_RETURN,UUID),&msg_expand)) < 0)
        return ret;

    // get command from message
    cmd = msg_expand->expand;

    offset = Getfiledfromstr(cmd_buf,cmd->name,':',DIGEST_SIZE/2);
    if(offset<0)
	    return -EINVAL;

    if(file_dir[0]!=0)
    {
    	Strncpy(file_buf,cmd->name+offset+1,DIGEST_SIZE*4);
    }
    else
    	Strncpy(file_buf,cmd->name+offset+1,DIGEST_SIZE*4);
       
   // check mode and build output file name
   if(Strncmp(cmd_buf,"encrypt",DIGEST_SIZE/2)==0)
   {
		crypt_mode=1;   
		Strncpy(output_file_buf,file_buf,DIGEST_SIZE*4-5);
		Strcat(output_file_buf,".sm4");
   }
   else if(Strncmp(cmd_buf,"decrypt",DIGEST_SIZE/2)==0)
   {
		crypt_mode=2; 
	        offset = Strnlen(file_buf,DIGEST_SIZE*4-1);
		if(Strcmp(file_buf+offset-4,".sm4")!=0)
		{
			print_cubeerr("file_sm4_crypt: decrypt file name is not *.sm4 format!");
			return -EINVAL;
		}	
		Strncpy(output_file_buf,file_buf,offset-4);
   }

   // get crypt key

   if(!Isemptyuuid(cmd->return_value))
   {
	Memcpy(crypt_key,cmd->return_value,DIGEST_SIZE);
   }
   else
   {
	Memcpy(crypt_key,default_key,DIGEST_SIZE/2);
	Memcpy(crypt_key+DIGEST_SIZE/2,default_iv,DIGEST_SIZE/2);
   }
	
    /* Start encrypt/decrypt  file */
    if ((ret = _datademo_file_sm4_crypt(file_buf, output_file_buf,crypt_key,crypt_mode,cmd->return_value)) < 0)
    	return -EINVAL;
    
    if(crypt_mode)
    {
	    Free(cmd->name);
	    cmd->name=dup_str(output_file_buf,DIGEST_SIZE*4);



	    calculate_sm3(cmd->name,cmd->return_value);
    }	

    message_add_expand_data(recv_msg,cmd,TYPE_PAIR(GENERAL_RETURN,UUID));
  
    return ex_module_sendmsg(sub_proc, recv_msg);
	
    /*i=0;

    // get file info and sender instance info from message
    ret=message_get_record(recv_msg,&source_file,0);
    if(ret<0)
	return ret;

    ret=message_get_define_expand(recv_msg,&msg_expand,TYPE_PAIR(DATADEMO_MAN,NODE));
    if(ret<0)
        return ret;
    if(msg_expand==NULL)
        return -EINVAL;
    node_info = msg_expand->expand;

    printf("file_sm4_crypt: get node info \n");

    // get server info from share value

    // create source file record 

    // generate file_key record


    return ret;*/
}

int _datademo_file_sm4_crypt(char * file_name, char * output_file_name, BYTE * file_key,
		int crypt_mode, BYTE * uuid) 
{
    int ret;
    int fd;
    int target_fd;
    int file_size;
    int block_size;
    int crypt_size;
    int total_crypt_size;
    int i;
    int offset;
    BYTE * plain_buf;
    BYTE * crypt_buf;
    BYTE digest[DIGEST_SIZE];
    struct stat statbuf;
    char file_buf[DIGEST_SIZE*6+1];


    Strncpy(file_buf,file_dir,DIGEST_SIZE*2);
    Strcat(file_buf,"/");
    offset=Strlen(file_buf);
    Strncpy(file_buf+offset,file_name,DIGEST_SIZE*4);

    fd=open(file_buf,O_RDONLY);
    if(fd<0)
        return fd;
    if(fstat(fd,&statbuf)<0)
    {
        print_cubeerr("fstat error\n");
        return -EINVAL;
    }
    file_size=statbuf.st_size;

    if(crypt_mode ==0)
    {
    	calculate_sm3(file_buf,uuid);
	return  0;
    }

    block_size = DIGEST_SIZE*16;

    plain_buf = Buf;
    crypt_buf=plain_buf+block_size;

    Strncpy(file_buf+offset,output_file_name,DIGEST_SIZE*4);
    target_fd=open(file_buf,O_WRONLY|O_TRUNC|O_CREAT,0644);
    if(target_fd<0)
        return target_fd;
    
    offset=0;
    total_crypt_size=0;
    while(offset<file_size)
    {
        int left_size=file_size-offset;
        if(left_size-offset > block_size)
        {
            ret = read(fd,plain_buf,block_size);
            if(ret!=block_size)
                return -EINVAL;
            offset+=block_size;
        }
        else
        {
            ret=read(fd,plain_buf,left_size);
            if(ret!=left_size)
                return -EINVAL;
            offset+=left_size;
        }
	switch(crypt_mode)
	{
		case 1:

       		 	crypt_size=sm4_context_crypt(plain_buf,&crypt_buf,ret,file_key);
        	 	ret=write(target_fd,crypt_buf,crypt_size);
        	 	if(ret!=crypt_size)
            			return -EINVAL;
			break;
		case 2:
       		 	crypt_size=sm4_context_decrypt(plain_buf,&crypt_buf,ret,file_key);
        	 	ret=write(target_fd,crypt_buf,crypt_size);
        	 	if(ret!=crypt_size)
            			return -EINVAL;
			break;
		default:
			return -EINVAL;
	}

        total_crypt_size+=crypt_size;

    }
    close(fd);
    close(target_fd);
    
    calculate_sm3(file_buf,uuid);
    return total_crypt_size;
}
