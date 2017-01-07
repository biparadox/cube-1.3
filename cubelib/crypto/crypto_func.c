/*#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
*/
//#include "common.h"
#include "../include/data_type.h"
#include "../include/string.h"
#include "../include/errno.h"
//#include "../list.h"
#include "sm3.h"
#include "sha1.h"
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

