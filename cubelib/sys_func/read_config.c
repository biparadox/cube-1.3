#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sem.h>
#include <dlfcn.h>
#include <stdarg.h> 

#include "data_type.h"
#include "alloc.h"
#include "json.h"
#include "memfunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "basefunc.h"
#include "memdb.h"
#include "message.h"
#include "connector.h"
#include "ex_module.h"
#include "sys_func.h"

struct timeval time_val={0,5*1000};
static char * err_file="cube_err.log";
static char * audit_file="cube_audit.log";
struct timeval first_time={0,0};
struct timeval debug_time;
char buffer[DIGEST_SIZE*32];

char errinfo_buf[DIGEST_SIZE*8];
int  err_para[10];

void  set_cubeerrinfo(char * errinfo)
{
	Strncpy(errinfo_buf,errinfo,DIGEST_SIZE*8);	
	return;
}

char *get_cubeerrinfo()
{
	char * err_info;
	int errinfo_len;
	errinfo_len=Strnlen(errinfo_buf,DIGEST_SIZE*8);
	if(errinfo_len==0)
		return NULL;
	err_info=Talloc0(errinfo_len+1);
	if(err_info==NULL)
		return NULL;
	Memcpy(err_info,errinfo_buf,errinfo_len);
	return err_info;
}

void set_cubeerrnum(int err_num,int site)
{
	if((site<0)|| (site>=10))
		return;
	err_para[site]=err_num;
	return;
}  

int get_cubeerrnum(int site)
{
	if((site<0)|| (site>=10))
		return 0;
	return err_para[site];
}

int audit_file_init()
{
	int fd;
    	fd =open(audit_file,O_CREAT|O_RDWR|O_TRUNC,0666);
	if(fd<0)
		return fd;
	
    	close(fd);
    	fd =open(err_file,O_CREAT|O_RDWR|O_TRUNC,0666);
	if(fd<0)
		return fd;
    	close(fd);
	return 0;
}

int print_cubeerr(char * format,...)
{
	int fd;
	int len;
	int ret;
	int offset=0;

  	va_list args;
        gettimeofday( &debug_time, NULL );
        if(first_time.tv_sec==0)
        {
                first_time.tv_sec=debug_time.tv_sec;
                first_time.tv_usec=debug_time.tv_usec;
                sprintf(buffer,"time: 0.0 :");
        }
        else
        {
                debug_time.tv_sec-=first_time.tv_sec;
                if(debug_time.tv_usec<first_time.tv_usec)
                        debug_time.tv_usec+=1000000-first_time.tv_usec;
                else
                        debug_time.tv_usec-=first_time.tv_usec;

                sprintf(buffer,"time: %d.%6.6d :",debug_time.tv_sec,debug_time.tv_usec);
        }
        offset=Strlen(buffer);
		//sprintf(buffer+offset,"errinfo: %s %d :", get_cubeerrinfo(),get_cubeerrnum(0));
		sprintf(buffer+offset,"errinfo: ");
        offset=Strlen(buffer);

  	va_start (args, format);
  	vsprintf (buffer+offset,format, args);
  	va_end (args);
	len=Strnlen(buffer,DIGEST_SIZE*32);
	
	fd=open(err_file,O_WRONLY|O_APPEND);
	if(fd<0)
		return -EINVAL;
	ret=write(fd,buffer,len);
	return ret;
}

int print_cubewarn(char * format,...)
{
	int fd;
	int len;
	int ret;
	int offset=0;

  	va_list args;
        gettimeofday( &debug_time, NULL );
        if(first_time.tv_sec==0)
        {
                first_time.tv_sec=debug_time.tv_sec;
                first_time.tv_usec=debug_time.tv_usec;
                sprintf(buffer,"time: 0.0 :");
        }
        else
        {
                debug_time.tv_sec-=first_time.tv_sec;
                if(debug_time.tv_usec<first_time.tv_usec)
                        debug_time.tv_usec+=1000000-first_time.tv_usec;
                else
                        debug_time.tv_usec-=first_time.tv_usec;

                sprintf(buffer,"time: %d.%6.6d :",debug_time.tv_sec,debug_time.tv_usec);
        }
        offset=Strlen(buffer);
	//	sprintf(buffer+offset,"warninfo: %s %d :", get_cubeerrinfo(),get_cubeerrnum(0));
		sprintf(buffer+offset,"warninfo: ");
        offset=Strlen(buffer);

  	va_start (args, format);
  	vsprintf (buffer+offset,format, args);
  	va_end (args);
	len=Strnlen(buffer,DIGEST_SIZE*32);
	
	fd=open(err_file,O_WRONLY|O_APPEND);
	if(fd<0)
		return -EINVAL;
	ret=write(fd,buffer,len);
	return ret;
}


int print_cubeaudit(char * format,...)
{
	int fd;
	int len;
	int ret;
	int offset=0;
  	va_list args;

        gettimeofday( &debug_time, NULL );
        if(first_time.tv_sec==0)
        {
                first_time.tv_sec=debug_time.tv_sec;
                first_time.tv_usec=debug_time.tv_usec;
                sprintf(buffer,"time: 0.0 :");
        }
        else
        {
                debug_time.tv_sec-=first_time.tv_sec;
                if(debug_time.tv_usec<first_time.tv_usec)
                        debug_time.tv_usec+=1000000-first_time.tv_usec;
                else
                        debug_time.tv_usec-=first_time.tv_usec;

                sprintf(buffer,"time: %d.%6.6d :",debug_time.tv_sec,debug_time.tv_usec);
        }
        offset=Strlen(buffer);

    va_start (args, format);
    vsprintf (buffer+offset,format, args);
    va_end (args);
    len=Strnlen(buffer,DIGEST_SIZE*32);
    buffer[len++]='\n';

	fd=open(audit_file,O_WRONLY|O_APPEND);
	if(fd<0)
		return -EINVAL;
	ret=write(fd,buffer,len);
	close(fd);
	return ret;
}
/*
int _print_cube_log(char * start, char * file,char * format,...)
{
	int fd;
	int len;
	int ret;
	int offset=0;
  	va_list args;

    gettimeofday( &debug_time, NULL );
    if(first_time.tv_sec==0)
    {
        first_time.tv_sec=debug_time.tv_sec;
        first_time.tv_usec=debug_time.tv_usec;
        sprintf(buffer,"time: 0.0 :");
    }
    else
    {
        debug_time.tv_sec-=first_time.tv_sec;
        if(debug_time.tv_usec<first_time.tv_usec)
            debug_time.tv_usec+=1000000-first_time.tv_usec;
        else
            debug_time.tv_usec-=first_time.tv_usec;

        sprintf(buffer,"%s: %d.%6.6d :",start,debug_time.tv_sec,debug_time.tv_usec);
    }
    offset=Strlen(buffer);

    va_start (args, format);
    vsprintf (buffer+offset,format, args);
    va_end (args);
    len=Strnlen(buffer,DIGEST_SIZE*32);
    buffer[len++]='\n';

	fd=open(file,O_WRONLY|O_APPEND);
	if(fd<0)
		return -EINVAL;
	ret=write(fd,buffer,len);
	close(fd);
	return ret;
}

int print_cubeaudit(char * format,...)
{
  	va_list args;
    va_start (args, format);
	_print_cube_log("start",audit_file,format,args);
    va_end (args);
}
*/
int debug_message(void * msg,char * info)
{
        char Buf[256];
        int offset;
        MSG_HEAD * msg_head;
        Buf[128]=0;
        Strncpy(Buf,info,128);
        offset=Strlen(Buf);
        msg_head=message_get_head(msg);

        sprintf(Buf+offset," message %x type %s to %s",*((UINT32 *)msg_head->nonce),message_get_typestr(msg),message_get_receiver(msg));
        print_cubeaudit("%s",Buf);
        return 0;

}

char *  get_temp_filename(char * tag )
{
	char buf[128];
	int len;
	pid_t pid=getpid();
	sprintf(buf,"/tmp/temp.%d",pid);
	len=strlen(buf);
	if(tag!= NULL)
	{
		len+=strlen(tag);
		if(strlen(tag)+strlen(tag)>=128)
			return -EINVAL;
		strcat(buf,tag);
	}
	char * tempbuf = Talloc(len+1);
	if(tempbuf==NULL)
		return -EINVAL;
	memcpy(tempbuf,buf,len+1);
	return tempbuf;
}
	
int convert_machine_uuid(char * uuidstr,BYTE * uuid)
{
	int i=0;
	int j=0;
	int len;
	int hflag=0;
	BYTE digit_value;
	char digit;
	BYTE *temp;

	
	len=Strlen(uuidstr);
	if(len<0)
		return 0;	
	if(len>DIGEST_SIZE*2)
		return -EINVAL;
	
	temp=Talloc0(DIGEST_SIZE);
	if(temp==NULL)
		return -ENOMEM;

	for(i=0;i<len;i++)
	{
		digit=uuidstr[i];
		if((digit>='0') &&(digit<='9'))
			digit_value= digit-'0';
		else if((digit>='a') &&(digit<='f'))
			digit_value= digit-'a'+10;
		else if((digit>='A') &&(digit<='F'))
			digit_value =digit-'A'+10;
		else
			continue;
		if(hflag==0)
		{
			temp[j]=digit_value;
			hflag=1;
		}
		else
		{
			temp[j]=(temp[j++]<<4)+digit_value;
			hflag=0;
		}
	}
	calculate_context_sm3(temp,DIGEST_SIZE,uuid);
	Free(temp);
	return DIGEST_SIZE;
}

int get_local_uuid(BYTE * uuid)
{
	FILE *fi,*fo;
	int i=0,j=0;
	char *s,ch;
	int len;
	int ret;

	char uuidstr[DIGEST_SIZE*2];
	
	char cmd[128];
	char *tempfile1,*tempfile2;
	char filename[DIGEST_SIZE*4];
	char *libpath;

	tempfile1=get_temp_filename(".001");
	if((tempfile1==NULL) || IS_ERR(tempfile1)) 
		return tempfile1;

	sprintf(cmd,"dmidecode | grep UUID | awk '{print $2}' > %s",tempfile1);
	ret=system(cmd);

	fi=fopen(tempfile1,"r");
	memset(uuidstr,0,DIGEST_SIZE*2);
	len=fread(uuidstr,1,36,fi);
	sprintf(cmd,"rm -f %s",tempfile1);
	ret=system(cmd);

	if(len==0)
	{
		libpath=getenv("CUBE_SYS_PLUGIN");
		if(libpath==NULL)
			return 0;	
		Strncpy(filename,libpath,DIGEST_SIZE*3);
		Strcat(filename,"/uuid");
		fi=fopen(filename,"r");
		if(fi==NULL)
		{
			print_cubeerr("can't get node's machine uuid,perhaps you need buildi"
				"an uuid file in cube-1.3/proc/plugin\n");
		}
		memset(uuidstr,0,DIGEST_SIZE);
		len=fread(uuidstr,1,36,fi);
		if(len<=0)
			return 0;
	}

	len=convert_machine_uuid(uuidstr,uuid);
	return len;
}

int read_json_node(int fd, void ** node)
{
	int ret;

	int readlen;
	int leftlen;
	int json_offset;

	void * root_node;
	char json_buffer[4097];

	readlen=read(fd,json_buffer,4096);
	if(readlen<0)
		return -EIO;

	if(readlen<2)
	{
		*node=NULL;
		return 0;
	}

	json_buffer[readlen]=0;

	ret=json_solve_str(&root_node,json_buffer);
	if(ret<0)
	{
		print_cubeerr("solve json str error!\n");
		return -EINVAL;
	}
	leftlen=readlen-ret;

	lseek(fd,-leftlen,SEEK_CUR);	
	*node=root_node;
	return ret;
}

int read_json_file(char * file_name)
{
	int ret;

	int fd;
	int readlen;
	int leftlen;
	int json_offset;

	int struct_no=0;
	void * root_node;
	void * findlist;
	void * memdb_template ;
	BYTE uuid[DIGEST_SIZE];
	char json_buffer[4096];

	fd=open(file_name,O_RDONLY);
	if(fd<0)
		return fd;

	ret=read_json_node(fd,&root_node);
	if(ret>0)
	{
		while(root_node!=NULL)
		{
			ret=memdb_read_desc(root_node,uuid);
			if(ret<0)
			{	
				print_cubeerr("read %d record format from %s failed!\n",struct_no,file_name);
				break;
			}
			struct_no++;
			ret=read_json_node(fd,&root_node);
			if(ret<0)
			{
				print_cubeerr("read %d record json node from %s failed!\n",struct_no,file_name);
				break;
			}
			if(ret<32)
				break;
		}
	}
	close(fd);
	return struct_no;
}

int read_record_file(char * record_file)
{
	int type,subtype;
	void * head_node;
	void * record_node;	
	void * elem_node;	
	int fd;
	int ret;
	void * record_template;
	int count=0;
	fd=open(record_file,O_RDONLY);
	if(fd<0)
		return -EIO;
	ret=read_json_node(fd,&head_node);
	if(ret<0)
		return -EINVAL;
	if(head_node==NULL)
		return -EINVAL;
	elem_node=json_find_elem("type",head_node);
	if(elem_node==NULL)
	{
		close(fd);
		return -EINVAL;
	}
	type=memdb_get_typeno(json_get_valuestr(elem_node));
	if(type<=0)
	{
		close(fd);
		return -EINVAL;
	} 			
	elem_node=json_find_elem("subtype",head_node);
	if(elem_node==NULL)
	{
		close(fd);
		return -EINVAL;
	}
	subtype=memdb_get_subtypeno(type,json_get_valuestr(elem_node));
	if(subtype<=0)
	{
		close(fd);
		return -EINVAL;
	} 
	record_template=memdb_get_template(type,subtype);
	if(record_template==NULL)
	{
		close(fd);
		return -EINVAL;
	}		
	
	ret=read_json_node(fd,&record_node);
	int record_size=struct_size(record_template);	
	void * record=NULL;

	while(ret>0)
	{
		if(record==NULL)
			record=Talloc(record_size);
		if(record_node!=NULL)
		{
			ret=json_2_struct(record_node,record,record_template);
			if(ret>=0)
			{
				char * record_name=NULL;
				void * temp_node=json_find_elem("name",record_node);
				if(temp_node!=NULL)
					record_name=json_get_valuestr(temp_node);
				ret=memdb_store(record,type,subtype,record_name);
				if(ret>=0)
				{
					record=NULL;
					count++;
				}
				
			}
		}	
		ret=read_json_node(fd,&record_node);
	}
	close(fd);
	return count; 	
}

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *arry;
};
int set_semvalue(int sem_id,int value)
{
    /* 用于初始化信号量，在使用信号量前必须这样做 */
    union semun sem_union;
 
    sem_union.val = value;
    if(semctl(sem_id, 0, SETVAL, sem_union) == -1)
    {
        return 0;
    }
    return 1;
}
 
void del_semvalue(int sem_id)
{
    /* 删除信号量 */
    union semun sem_union;
 
    if(semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
    {
         fprintf(stderr, "Failed to delete semaphore\n");
    }
}

int semaphore_p(int sem_id,int value)
{
    /* 对信号量做减1操作，即等待P（sv）*/
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = -value;//P()
    sem_b.sem_flg = SEM_UNDO;
    if(semop(sem_id, &sem_b, 1) == -1)
    {
        printf("semaphore_p failed\n");
        return 0;
    }
    return 1;
}
 
int semaphore_v(int sem_id,int value)
{
    /* 这是一个释放操作，它使信号量变为可用，即发送信号V（sv）*/
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = value;//V()
    sem_b.sem_flg = SEM_UNDO;
    if(semop(sem_id, &sem_b, 1) == -1)
    {
        fprintf(stderr, "semaphore_v failed\n");
        return 0;
    }
    return 1;
}

int get_short_uuidstr(int len,BYTE * digest,char * uuidstr)
{
	int ret;
	char uuid_buf[DIGEST_SIZE*2];
	
	if(len<0)
		return -EINVAL;
	if(len>32)
		len=32;
	ret=len*2;
	digest_to_uuid(digest,uuid_buf);
	Memcpy(uuidstr,uuid_buf,ret);
	uuidstr[ret]=0;
	return ret;
}

int convert_uuidname(char * name,int len,BYTE * digest,char * newfilename)
{
	int i;
	int lastsplit;
	int offset;
	int ret;
	char uuidstr[DIGEST_SIZE*2];
	char filename[DIGEST_SIZE*4];

	if((len<0)||(len>32))
		return -EINVAL;
	if(len==0)
		len=DIGEST_SIZE;

	lastsplit=0;
	for(i=0;name[i]!=0;i++)
	{
		if(name[i]=='/')
			lastsplit=i;	
	}
	
	ret=calculate_sm3(name,digest);
	if(ret<0)
		return ret;

	len=get_short_uuidstr(len,digest,uuidstr);

	offset=lastsplit;
	if(offset!=0)
	{
		Memcpy(newfilename,name,offset+1);
		offset++;
	}
	Strncpy(newfilename+offset,uuidstr,DIGEST_SIZE*2);
	
	ret=rename(name,newfilename);
	if(ret<0)
		return ret;
	return 1;	
} 
int RAND_bytes(unsigned char *buffer, size_t len) 
{
    int ret, fd;
    const char * randomfile = "/dev/urandom";
    fd = open(randomfile, O_RDONLY);
    if (fd < 0) { 
        perror("open urandom device:");
        return fd;
    }    
    int readn = 0; 
    while (readn != len) {
        ret = read(fd, buffer + readn, len - readn);
        if (ret < 0) { 
            perror("read urandom device:");
            return ret; 
        }    
        readn += ret; 
    }    
    return 0;
}                                                                                                                                                                                                                
void print_bin_data(BYTE * data,int len,int width)
{
    int i;
    for(i=0;i<len;i++){
        printf("%.2x ",data[i]);
        if (width>0)
        {
            if((i+1)%width==0)
                printf("\n");
        }
    }
    printf("\n");
}

static int json_tab=4;
static int max_len=80;
int print_pretty_text(char * json_str,int fd)
{
	int i=0,j=0;
	int inlen=0;
	int outlen=0;
	char print_str[256];
	char tab_len[128];
	int print_state=0;
	int deep=-1;
	int jump=0;
	inlen=Strlen(json_str);
	if(inlen>4096)
		return -EINVAL;

	for(i=0;i<inlen;i++)
	{
		print_str[j++]=json_str[i];
		if((json_str[i]=='{')||(json_str[i]=='['))
		{
			if(deep==-1)
			{
				print_str[j++]='\n';
				write(fd,print_str,j);
				deep++;
				j=0;
			}
			else if(print_state==0)
			{
				print_state=1;
				if(deep<=0)
					jump=0;
				else
			    	jump=deep;
				deep++;
			}
			else if(print_state==1)
			{
				i--;
				print_str[j-1]='\n';
				Memset(tab_len,' ',jump*json_tab);
				write(fd,tab_len,jump*json_tab);
				write(fd,print_str,j);
				j=0;
				print_state=0;	
			}
		}
		else if((json_str[i]=='}')||(json_str[i]==']'))
		{
			if(print_state==0)
			{
				if(deep<=0)
					jump=0;
				else
			    	jump=deep;
			}

			if(json_str[i+1]==',')
			{
				i++;
				print_str[j++]=json_str[i];
			}
			print_str[j++]='\n';
			Memset(tab_len,' ',jump*json_tab);
			write(fd,tab_len,jump*json_tab);
			write(fd,print_str,j);
			j=0;
			print_state=0;
			deep--;
		}
		else if(j==max_len)
		{
			if(print_state==0)
			{
				if(deep<=0)
					jump=0;
				else
			    	jump=deep;
			}
			print_str[j++]='\n';
			Memset(tab_len,' ',jump*json_tab);
			write(fd,tab_len,jump*json_tab);
			write(fd,print_str,j);
			j=0;
			print_state=0;
		}
			
	}
	return 0;
}
