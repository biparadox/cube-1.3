#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "basetype.h"
#include "message.h"
#include "ex_module.h"
#include "sys_func.h"
#include "channel.h"
#include "output_channel.h"

CHANNEL * output_channel;
BYTE * Buffer;
char filename[DIGEST_SIZE*8];

int output_channel_init(void * sub_proc,void * para)
{

    int ret;
    struct output_init_para * init_para=para;

    char local_uuid[DIGEST_SIZE];
    char proc_name[DIGEST_SIZE];
 	
    if(init_para==NULL)
    {
	   print_cubeerr("can't find websocket port address!\n");
	   return -EINVAL;
    }	


    Buffer=Dalloc(4096,NULL);
    Memset(Buffer,0,4096);


    output_channel=channel_register(init_para->channel_name,CHANNEL_RDWR,sub_proc);
    if(output_channel==NULL)
        return -EINVAL;

    if(init_para->datafile==NULL)
	return -EINVAL;
	
    Strncpy(filename,init_para->datafile,DIGEST_SIZE*8);
	
   return 0;
}

int hexstr_2_bin(BYTE * hex,int size,BYTE * bin)
{
        int i,j;
        int len;
        unsigned char var;
	int flip=0;
        len=Strnlen(hex,size);

	j=0;
        for(i=0;i<len;i++)
        {
                if((hex[i]>='0') &&(hex[i]<='9'))
                        var=hex[i]-'0';
                else if((hex[i]>='a')&&(hex[i]<='f'))
                        var=hex[i]-'a'+10;
                else if((hex[i]>='A')&&(hex[i]<='F'))
                        var=hex[i]-'A'+10;
		else if(flip==1)
			return -EINVAL;
		else if(hex[i]=='\n')
		{
			if(flip!=0)
				return -EINVAL;
			break;
		}
                else if(hex[i]==' ')
			continue;
		else
                        return -EINVAL;
		flip++;
		if(flip==1)
                        bin[j]=var*0x10;
		if(flip==2)
		{
			bin[j++]+=var;
			flip=0;
		}
        }
        return j;
}

int output_channel_start(void * sub_proc,void * para)
{
    int ret;
    int retval;
    void * message_box;
    void * context;
    int i;
    struct timeval conn_val;
    conn_val.tv_usec=time_val.tv_usec;

    char local_uuid[DIGEST_SIZE];
    char proc_name[DIGEST_SIZE];
    int out_offset;
	 int offset=0;
    char line[DIGEST_SIZE*8];
    FILE * fp;		
    print_cubeaudit("begin output channel process!\n");
    ret=proc_share_data_getvalue("uuid",local_uuid);
    if(ret<0)
        return ret;
    ret=proc_share_data_getvalue("proc_name",proc_name);

    if(ret<0)
	return ret;

    fp=fopen(filename,"r");
    if(fp==NULL)
	return -EINVAL;	
	
   int flip=0; // 0 start or a segment finished 1 read first char 2 read next char 3 read file finished
   int flop=0; // 0 ready to send data 1 send data 2 return data
    
    while(1)
    {
        usleep(time_val.tv_usec);
        // read ex_channel 
	char *pstr;
	do
	{
		int len;
		int count=0;
		if(flip>3)
			break;
		if(flop>0)
			break;
		pstr=fgets(line,DIGEST_SIZE*8,fp);
		if(pstr==NULL)
		{
			flip=3;
			break;
		}
		if(pstr<0)
		{
			flip=-1;
			break;
		}
		
		len=Strnlen(line,DIGEST_SIZE*8);
		if(len>0)
		{
			if(line[0]=='#')
				continue;
			ret=hexstr_2_bin(line,len,Buffer+offset);
			if(offset>0)
			{
				if(ret==0)
				{
					break;
				}
			}
			offset+=ret;
		}
		else if(len==0)
		{
			if(offset>0)
				break;	
		}
	}while(1);	

	if((flip>=0)&&(flip<=3))
	{
		if(offset>0)
		{
			//print_cubeaudit("write %d data!\n",offset);
			printf("write %d data!\n",offset);
			ret=channel_inner_write(output_channel,Buffer,offset);	
			flop=1;
			if(flip==3)
				flip++;
			offset=0;
		}
	}
	// send message to the remote
	if(flop==0)
		out_offset=0;
	if(flop>=1)
	{
		
		ret=channel_inner_read(output_channel,Buffer+out_offset,DIGEST_SIZE*16);
		if((out_offset==0)&&(ret==0))
			continue;	
		if(flop==1)
		{
			out_offset+=ret;
			flop++;
		}
		else if(flop==2)
		{
			flop=0;
			out_offset+=ret;
			//print_cubeaudit("echo %d data!\n",out_offset);
			printf("output_channel echo %d data!\n",out_offset);
			print_bin_data(Buffer,out_offset,8);	
			out_offset=0;
			flip=0;
		}
	}			

	
    }
    return 0;
}
