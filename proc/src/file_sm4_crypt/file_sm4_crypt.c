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

extern struct timeval time_val={0,50*1000};
BYTE Buf[DIGEST_SIZE*32];
BYTE default_key[DIGEST_SIZE/2]="123456";
BYTE default_iv[16] = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10};

int file_sm4_crypt_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
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
        goto err;

    // get command from message

    offset = Getfiledfromstr(cmd_buf,cmd->name,":",DIGEST_SIZE/2);
    if(offset<0)
	    return -EINVAL;

    Strncpy(file_buf,cmd->name+offset+1,DIGEST_SIZE*4);
       

   if(Strncmp(file_buf,"crypt"==0))
		crypt_mode=1;   
   else if(Strncmp(file_buf,"decrypt"==0))
		crypt_mode=2;   

   if(!Isemptyuuid(cmd->return_value))
   {
	Memcpy(crypt_key,cmd->return_value,DIGEST_SIZE);
   }
   else
   {
	Memcpy(crypt_key,key,DIGEST_SIZE);
   }
	

    if ((file_key = Talloc0(sizeof(*file_key))) == NULL) {
        ret = -ENOMEM;
        goto err;
    }
    RAND_bytes(file_key->key,DIGEST_SIZE);
    RAND_bytes(file_key->iv,DIGEST_SIZE);
    file_key->crypt_start = 0;

    if ((crypt_file = Talloc0(sizeof(*crypt_file))) == NULL) {
        ret = -ENOMEM;
        goto err;
    }

    /* Start encrypting files */
    if ((ret = _datademo_file_sm4_crypt(source_file->file_name, crypt_file, file_key)) < 0)
        flag = -1;
    
    if(source_file->user_name != NULL)
    	Strncpy(crypt_file->user_name, source_file->user_name, DIGEST_SIZE);
    Memcpy(crypt_file->src_uuid, node_info->node_uuid, DIGEST_SIZE);

    /* Store encrypted files */
    db_record = memdb_store(crypt_file, TYPE_PAIR(DATADEMO_MAN,FILE), NULL);
    if(db_record == NULL)
	    printf("file_sm4_crypt: store crypt data failed \n");
    /* store key */
    db_record = memdb_store(file_key, TYPE_PAIR(DATADEMO_KEY,FILE_KEY), NULL);
    if(db_record == NULL)
	    printf("file_sm4_crypt: store file_key failed \n");
    
    // store the source file record    
    void *types_msg = message_gen_typesmsg(TYPE_PAIR(DATADEMO_MAN,FILE),NULL);
    if(types_msg == NULL) {
        ret = -EINVAL;
        goto err;
    }
	ex_module_sendmsg(sub_proc,types_msg);

    void *types_msg1 = message_gen_typesmsg(TYPE_PAIR(DATADEMO_KEY,FILE_KEY),NULL);
    if(types_msg1 == NULL) {
        ret = -EINVAL;
        goto err;
    }
    ex_module_sendmsg(sub_proc,types_msg1);

    /* Return information after building file encryption */
    if ((fileret = Talloc0(sizeof(*fileret))) == NULL){
		ret = -ENOMEM;
		goto err;
	}

	snprintf(buf, sizeof(buf), "The File action 'ACTION_REG' %s.", (flag == 0 ? "success" : "failed"));
    fileret->info = strdup(buf);
    fileret->action = qf->action;
	fileret->state = (flag == 0 ? RET_SUCCESS : RET_FAILED);
	strncpy(fileret->expand.uuid, crypt_file->uuid, sizeof(fileret->expand.uuid));
	strncpy(fileret->expand.src_uuid, crypt_file->src_uuid, sizeof(fileret->expand.src_uuid));
	strncpy(fileret->expand.user_name, crypt_file->user_name, sizeof(fileret->expand.user_name));
	strncpy(fileret->expand.file_name, crypt_file->file_name, sizeof(fileret->expand.file_name));

    new_msg = message_create(TYPE_PAIR(DATADEMO_QUERY,FILERETURN), recv_msg);
	if(new_msg == NULL) {
		ret = -EINVAL;
		goto err;
	}

	if ((ret = message_add_record(new_msg, fileret)) < 0)
		goto err;
	
	return ex_module_sendmsg(sub_proc, new_msg);
err:
    if (file_key) Free0(file_key);
    if (crypt_file) Free0(crypt_file);
    if (fileret) {
        if (fileret->info) free(fileret->info);
        Free0(fileret);
    }
    return ret;
	
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

/*
    server_info = Talloc0(sizeof(*server_info));
    if(server_info==NULL)
        return -ENOMEM;

    proc_share_data_getvalue("uuid",server_info->node_uuid);
    proc_share_data_getvalue("proc_name",server_info->proc_name);
    proc_share_data_getvalue("user_name",server_info->user_name);
*/
    // create source file record 

    // generate file_key record

   /* file_key = Talloc0(sizeof(*file_key));
    if(file_key == NULL)
        return -ENOMEM;

    RAND_bytes(file_key->key,DIGEST_SIZE);
    RAND_bytes(file_key->iv,DIGEST_SIZE);
    file_key->crypt_start=0;

    crypt_file = Talloc0(sizeof(*crypt_file));
    if(crypt_file == NULL)
        return -ENOMEM;

    ret = _datademo_file_sm4_crypt(source_file->file_name,crypt_file,file_key);
    if(ret<0)
        return ret;
    if(source_file->user_name !=NULL)
    	Strncpy(crypt_file->user_name,source_file->user_name,DIGEST_SIZE);

    Memcpy(crypt_file->src_uuid,node_info->node_uuid,DIGEST_SIZE);	

    db_record = memdb_store(crypt_file,TYPE_PAIR(DATADEMO_MAN,FILE),NULL);
    if(db_record ==NULL)
	    printf("file_sm4_crypt: store crypt data failed \n");

    db_record = memdb_store(file_key,TYPE_PAIR(DATADEMO_KEY,FILE_KEY),NULL);
    if(db_record ==NULL)
	    printf("file_sm4_crypt: store file_key failed \n");
    // store the source file record    
    void * types_msg =message_gen_typesmsg(TYPE_PAIR(DATADEMO_MAN,FILE),NULL);
    if(types_msg==NULL)
        return -EINVAL;
	ex_module_sendmsg(sub_proc,types_msg);

    void * types_msg1 =message_gen_typesmsg(TYPE_PAIR(DATADEMO_KEY,FILE_KEY),NULL);
    if(types_msg1==NULL)
        return -EINVAL;
    ex_module_sendmsg(sub_proc,types_msg1);
     
    new_msg=message_create(TYPE_PAIR(DATADEMO_MAN,FILE),recv_msg);
    message_add_record(new_msg,crypt_file);
    ex_module_sendmsg(sub_proc,new_msg);

    printf("file_sm4_crypt: send crpyt_file msg \n");

    return ret;*/
}

int proc_ctrl_message(void * sub_proc,void * message)
{
	int ret;
	int i=0;
	printf("begin proc echo \n");

	struct ctrl_message * ctrl_msg;

	
	ret=message_get_record(message,&ctrl_msg,i++);
	if(ret<0)
		return ret;
	if(ctrl_msg!=NULL)
	{
		ret=ctrl_msg->ctrl; 
	}

	ex_module_sendmsg(sub_proc,message);

	return ret;
}

int _datademo_file_sm4_crypt(char * file_name, RECORD(DATADEMO_MAN,FILE) * crypt_file,
    RECORD(DATADEMO_KEY,FILE_KEY) * file_key)
{
    int ret;
    int fd;
    int target_fd;
    int block_size;
    int crypt_size;
    int total_crypt_size;
    int i;
    int offset;
    BYTE * plain_buf;
    BYTE * crypt_buf;
    BYTE digest[DIGEST_SIZE];
    struct stat statbuf;

    fd=open(file_name,O_RDONLY);
    if(fd<0)
        return fd;
    if(fstat(fd,&statbuf)<0)
    {
        print_cubeerr("fstat error\n");
        return -EINVAL;
    }
    file_key->file_size=statbuf.st_size;

    block_size = DIGEST_SIZE*16;

    plain_buf = Buf;
    crypt_buf=plain_buf+block_size;

    Strncpy(crypt_file->file_name,file_name,DIGEST_SIZE);
    Strncat(crypt_file->file_name,".sec",DIGEST_SIZE);
    target_fd=open(crypt_file->file_name,O_WRONLY|O_TRUNC|O_CREAT,0644);
    if(target_fd<0)
        return target_fd;
    
    offset=0;
    total_crypt_size=0;
    while(offset<file_key->file_size)
    {
        int left_size=file_key->file_size-offset;
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
        crypt_size=sm4_context_crypt(plain_buf,&crypt_buf,ret,file_key->key);
        ret=write(target_fd,crypt_buf,crypt_size);
        if(ret!=crypt_size)
            return -EINVAL;
        total_crypt_size+=crypt_size;

    }
    close(fd);
    close(target_fd);
    
    calculate_sm3(crypt_file->file_name, crypt_file->uuid);
    // digest_to_uuid(digest, crypt_file->uuid);

    file_key->crypt_size=total_crypt_size; 
    Memcpy(file_key->file_uuid,crypt_file->uuid,DIGEST_SIZE);

    return total_crypt_size;
}
