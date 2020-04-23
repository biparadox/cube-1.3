/*#include <stdlib.h>
#include <memfunc.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
*/
//#include "common.h"
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/data_type.h"
#include "../include/alloc.h"
#include "../include/memfunc.h"
#include "../include/errno.h"
//#include "../list.h"
#include "sm3.h"
#include "sha1.h"
#include "sm4.h"
#include "../include/crypto_func.h"

//int file_to_hash(int argc, char *argv[])
int digest_to_uuid(BYTE *digest,char *uuid)
{
	int i,j,k,retval;
	unsigned char char_value;
	retval=DIGEST_SIZE;
	k=0;
	for(i=0;i<retval;i++)
	{
		int tempdata;
		char_value=digest[i];

		for(j=0;j<2;j++)
		{
			tempdata=char_value>>4;
			if(tempdata>9)
				*(uuid+k)=tempdata-10+'a';
			else
				*(uuid+k)=tempdata+'0';
			k++;
			if(j!=1)
				char_value<<=4;
				
		}
	}
	return 0;
}

int uuid_to_digest(char * uuid,BYTE * digest)
{
	int i;
	BYTE char_value;
	for(i=0;i<DIGEST_SIZE*2;i++)
	{
		if((uuid[i]>='0')&&(uuid[i]<='9'))
			char_value=uuid[i]-'0';
		else if((uuid[i]>='a')&&(uuid[i]<='f'))
			char_value=uuid[i]-'a'+10;
		else if((uuid[i]>='A')&&(uuid[i]<='F'))
			char_value=uuid[i]-'A'+10;
		else
			return -EINVAL;
		if(i%2==0)
			digest[i/2]=char_value<<4;
		else
			digest[i/2]+=char_value;
	}
	return 0;
}
#define PCR_SIZE 20
int extend_pcr_sm3digest(BYTE * pcr_value,BYTE * sm3digest)
{
	BYTE buffer[DIGEST_SIZE*2];
	BYTE digest[DIGEST_SIZE];
	Memcpy(buffer,pcr_value,PCR_SIZE);
	Memcpy(buffer+PCR_SIZE,sm3digest,DIGEST_SIZE);
	calculate_context_sha1(buffer,PCR_SIZE+DIGEST_SIZE,digest);
	Memcpy(pcr_value,digest,PCR_SIZE);
	return 0;
}

int comp_proc_uuid(BYTE * dev_uuid,char * name,BYTE * conn_uuid)
{
	int len;
	int i;
	BYTE buffer[DIGEST_SIZE*2];
	BYTE digest[DIGEST_SIZE];
	Memset(buffer,0,DIGEST_SIZE*2);
	Memcpy(buffer,dev_uuid,DIGEST_SIZE);
	if(name!=NULL)
	{
		len=Strlen(name);
		if(len<DIGEST_SIZE)
		{
			Memcpy(buffer+DIGEST_SIZE,name,len);
		}
		else 
		{
			Memcpy(buffer+DIGEST_SIZE,name,DIGEST_SIZE);
		}
	}
	calculate_context_sm3(buffer,DIGEST_SIZE*2,conn_uuid);
	return 0;
}

static unsigned char iv[16] = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10};
int sm4_context_crypt( BYTE * input, BYTE ** output, int size,char * passwd)
{
	int i;
	int out_size;
        sm4_context ctx;
	char keypass[DIGEST_SIZE];
	
	BYTE * out_blob;
        if(size<=0)
                return -EINVAL;

	out_size=size;

	out_blob=malloc(out_size);
	if(out_blob==NULL)
		return -ENOMEM;
	memset(keypass,0,DIGEST_SIZE);
	strncpy(keypass,passwd,DIGEST_SIZE);

	sm4_setkey_enc(&ctx,keypass);
	for(i=0;i<=out_size-16;i+=16)
	{
		sm4_crypt_ecb(&ctx,1,16,input+i,out_blob+i);
	}	
	for(;i<out_size;i++)
		out_blob[i]=input[i]^iv[i%16];		
	*output=out_blob;
	return out_size;
}

int sm4_context_decrypt( BYTE * input, BYTE ** output, int size,char * passwd)
{
	int i;
	int out_size;
        sm4_context ctx;
	char keypass[DIGEST_SIZE];
	
	BYTE * out_blob;
        if(size<=0)
                return -EINVAL;

	out_size=size;

	out_blob=malloc(out_size);
	if(out_blob==NULL)
		return -ENOMEM;
	memset(keypass,0,DIGEST_SIZE);
	strncpy(keypass,passwd,DIGEST_SIZE);

	sm4_setkey_dec(&ctx,keypass);
	for(i=0;i<=out_size-16;i+=16)
	{
		sm4_crypt_ecb(&ctx,1,16,input+i,out_blob+i);
	}	
	for(;i<out_size;i++)
		out_blob[i]=input[i]^iv[i%16];		
	*output=out_blob;
	return out_size;
}

void sm4_data_prepare(int input_len,BYTE * input_data,int * output_len,BYTE * output_data)
{
	int pad_len;
	BYTE pad_value;
        const int block_size=16;   

	pad_len=block_size-(input_len%block_size);
	pad_value=(BYTE)pad_len;

	*output_len=input_len+pad_len;
	Memcpy(output_data,input_data,input_len);
	Memset(output_data+input_len,pad_value,pad_len);
	return;
}

int sm4_data_recover(int input_len,BYTE * input_data,int * output_len,BYTE * output_data)
{
	int pad_len;
	BYTE pad_value;
        const int block_size=16;  

	pad_value=input_data[input_len-1];
        if(pad_value>=block_size)
		return -EINVAL;
	pad_len=(int)pad_value; 
             
	*output_len=input_len-pad_len;
	Memcpy(output_data,input_data,*output_len);
	return *output_len;
}

int sm4_text_crypt( BYTE * input, BYTE ** output,BYTE * passwd)
{
	int i;
	int ret;
	int in_size;
	int crypt_size;
	int out_size;
        sm4_context ctx;
	const int blocksize=16;
	char keypass[DIGEST_SIZE];
	BYTE *crypt_buf;
	char * output_buf;
	
	in_size=Strlen(input)+1;
	if(in_size==0)
		return 0;
		
	crypt_size=((in_size/blocksize)+1)*blocksize;
	out_size=bin_to_radix64_len(crypt_size)+1;
	
	output_buf=Dalloc0(out_size,output);
	if(output_buf==NULL)
		return -EINVAL;
	crypt_buf=Talloc0(crypt_size);
	if(crypt_buf==NULL)
		return -EINVAL;
	sm4_data_prepare(in_size,input,&ret,output_buf);
	if(ret!=crypt_size)
		return -EINVAL;
	memset(keypass,0,DIGEST_SIZE);
	Strncpy(keypass,passwd,DIGEST_SIZE);

	sm4_setkey_enc(&ctx,keypass);
	for(i=0;i<=crypt_size-16;i+=16)
	{
		sm4_crypt_ecb(&ctx,1,16,output_buf+i,crypt_buf+i);
	}	
	ret=bin_to_radix64(output_buf,crypt_size,crypt_buf);
	if(ret!=out_size-1)
	{
		return -EINVAL;
	}
	output_buf[out_size-1]=0;
	Free(crypt_buf);
	*output=output_buf;
	return out_size;
}

int sm4_text_decrypt( BYTE * input, BYTE ** output,BYTE * passwd)
{
	int i;
	int ret;
	int in_size;
	int crypt_size;
	int out_size;
        sm4_context ctx;
	const int blocksize=16;
	char keypass[DIGEST_SIZE];
	BYTE *crypt_buf;
	BYTE *decrypt_buf;
	char * output_buf;
	
	in_size=Strlen(input);
	if(in_size==0)
		return 0;
	crypt_size=radix_to_bin_len(in_size);
	if(crypt_size%16!=0)
		return -EINVAL;
	
	crypt_buf=Talloc0(crypt_size*2);
	if(crypt_buf==NULL)
		return -EINVAL;
	
	decrypt_buf=crypt_buf+crypt_size;

	sm4_setkey_dec(&ctx,keypass);
	for(i=0;i<=crypt_size-16;i+=16)
	{
		sm4_crypt_ecb(&ctx,1,16,crypt_buf+i,decrypt_buf+i);
	}	
	ret=sm4_data_recover(crypt_size,decrypt_buf,&out_size,crypt_buf);
	if(ret<0)
		return ret;
	output_buf=Dalloc0(out_size,output);
	if(output_buf==NULL)
		return -ENOMEM;
	Memcpy(output_buf,crypt_buf,out_size);
	*output=output_buf;
	return out_size;	
}

int calculate_sm3(char* filename, UINT32 *SM3_hash)
{
    int fd1;
    int result;
    int bytes_to_copy = 0;
    int filesize = 0;
    struct stat attribute;
    SM3_context index;
    char *sm3buffer;
    sm3buffer = (char*) malloc(4096);

    /* Open the input file */
    fd1 = open(filename,O_RDONLY);

    if (fd1 < 0)
    {
        printf("Error opening file\n");
        return -1;
    }
    stat(filename,&attribute);
    filesize = attribute.st_size;
        //printf("filesize:%d\n",filesize);

    /* Initialise sm3-Context */
    result = SM3_init(&index);
    if (result)
    {
        close (fd1);
        free(sm3buffer);
        return -1;
    }


	 bytes_to_copy = filesize;
	    while (bytes_to_copy > 4096)
	    {

		read(fd1,sm3buffer,4096);
    		result = SM3_update(&index,sm3buffer,4096);
		if (result)
	        {
		    close (fd1);
		    free(sm3buffer);
    		    return -1;
		}
    		bytes_to_copy = bytes_to_copy - 4096;
		
	    }

	



            // And the last one
            memset(sm3buffer,0,4096);
            read(fd1,sm3buffer,bytes_to_copy);
            result = SM3_update(&index,sm3buffer,bytes_to_copy);
            if (result)
            {
                close (fd1);
                free(sm3buffer);
                return -1;
            }

    close (fd1);
    free(sm3buffer);
    result = SM3_final(&index,SM3_hash);
    if (result)
        return -1;
    return 0;
}

int get_random_uuid(BYTE * uuid)
{
	int fd;
	int ret;
	fd=open("/dev/urandom",O_RDONLY);
	if(fd<0)
		return fd;
	ret=read(fd,uuid,DIGEST_SIZE);	
	if(ret!=DIGEST_SIZE)
		return 0;
	close(fd);
	return ret;
}

int calculate_context_sha1(BYTE *context,int context_size,UINT32 *SM3_hash)
{
	int result;
//	SHA1_CTX index;
//	SHA1Init(&index);
//	SHA1Update(&index,context,context_size);
//	SHA1Final(SM3_hash,&index);
	tpm_sha1_ctx_t index;
	tpm_sha1_init(&index);
	tpm_sha1_update(&index,context,context_size);
	tpm_sha1_final(&index,SM3_hash);
	return 0;
}
