/**
 * Copyright [2015] Tianfu Ma (matianfu@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * File: buddy.c
 *
 * Created on: Jun 5, 2015
 * Author: Tianfu Ma (matianfu@gmail.com)
 */
#include <errno.h>
//#include "../include/errno.h"
#include "../include/data_type.h"
#include "../include/memfunc.h"

const int maxnamelen=DIGEST_SIZE*8+1;
void * Memcpy(void * dest,void * src, unsigned int count)
{
	if(dest == src)
		return src;
	unsigned char * d=(char *)dest;
	unsigned char * s=(char *)src;
	while(count-->0)
		*d++=*s++;
	return dest;
}

int    Memcmp(const void *s1,const void *s2,int n)
{
	unsigned char * buf1=s1;
	unsigned char * buf2 =s2;
	
	while(--n && *buf1 == *buf2)
	{
		buf1++;
		buf2++;
	}
	return *buf1-*buf2;
}

void * Memset(void * s,int c, int n)
{
	const unsigned char uc=c;
	unsigned char * su;
	for(su=s;n>0;++su,--n)
		*su=uc;
	return s;
}

char * Strcpy(char *dest,const char *src)
{
	if(dest == src)
		return src;
	char * d=(char *)dest;
	char * s=(char *)src;
	while(*s!=0)
		*d++=*s++;
        *d=0;
	return dest;
}

char * Strncpy(char *dest,const char *src,int n)
{
	if(dest == src)
		return src;
	char * d=(char *)dest;
	char * s=(char *)src;
	while((*s!=0)&&(n--)) 
		*d++=*s++;
	
	if(n>=0)
        	*d=0;
	return dest;

}
int    Strcmp(const char *s1,const char *s2)
{
	int ret = 0;
	
	while( !(ret = *(unsigned char *)s1 - *(unsigned char *)s2) && *s1)
	{
		s1++;
		s2++;
	}
	return ret;
}

int    Strncmp(const char *s1,const char *s2,int n)
{
	if(n<=0)
		return 0;
	while(--n && *s1 && *s1==*s2)
	{
		s1++;
		s2++;
	}
	return *(unsigned char *)s1 - *(unsigned char *)s2;
}

char * Strcat(char * dest,const char *src)
{
	char * cp=dest;
	while(*cp)
		cp++;
	while(*cp++=*src++);
	
	return dest;
}

char * Strncat(char * dest,const char *src,int n)
{
	char * cp=dest;
	while(n-- && *cp)
		cp++;
	if(n<0)
		return dest;

	while(n-- && (*cp++=*src++));
	
	return dest;
}

int    Strlen(char * str)
{
	int i;
	for(i=0;str[i];i++);
	return i;
}

int    Strnlen(char * str,int n)
{
	int i;
	for(i=0;i<n && str[i];i++);
	return i;
}

int Getfiledfromstr(char * name,char * str,char IFS,int maxsize)
{
	int offset=0;
	int i=0;
	int limit=maxsize;
	if(limit==0)
		limit=maxnamelen;
	while(str[i]==' ')
		i++;
	if(str[i]==0)
		return 0;
	
	for(;offset<limit;i++)
	{
		if((str[i]==IFS) || (str[i]==0))
		{
			name[offset]=0;
			return i;
		}
		name[offset++]=str[i];
	}
	name[0]=0;
	return i;
}

int Itoa(int n,char *str)
{
	int i=0,isNegative=0;
	int len=0;
	char temp;
	if((isNegative=n)<0)
		n=-n;
	do
	{
		str[len++]=n%10+'0';
		n=n/10;
	}while(n>0);
	if(isNegative<0)
		str[len++]='-';
	str[len]=0;
	for(i=0;i<len/2;i++)
	{
		temp=str[i];
		str[i]=str[len-1-i];
		str[len-1-i]=temp;	
	}
	return len;
}

static inline int _isdigit(char c)
{
	if((c>='0') && (c<='9'))
		return 1;
	return 0;
}
static inline int _ishexdigit(char c)
{
	if((c>='0') && (c<='9'))
		return 1;
	if((c>='a') && (c<='f'))
		return 1;
	if((c>='A') && (c<='F'))
		return 1;
	
	return 0;
}
static inline int _get_char_value(char c)
{
	if(_isdigit(c))
		return c-'0';
	if((c>='a') && (c<='f'))
		return c-'a'+10;
	if((c>='A') && (c<='F'))
		return c-'A'+10;
	return -EINVAL;
}
int Atoi(char * string,int maxlen)
{
	int ret=0;
	int i;
	int base=10;
	int temp_value;
	int str_len;
	if(maxlen==0)
		str_len=Strnlen(string,DIGEST_SIZE);
	else
		str_len=Strnlen(string,maxlen);

	// process the head
	for(i=0;i<str_len;i++)
	{
		if(string[i]==0)
			break;
		if(string[i]==' ')
		{
			i++;
			continue;
		}
		// change the base
		if(string[i]=='0')
		{
			switch(string[i+1])
			{
				case 0:
					return 0;
				case 'b':
				case 'B':
					i+=2;
					base=2;
					break;
				case 'x':
				case 'X':
					i+=2;
					base=16;
					break;
				default:
					i++;
					base=8;
					break;

			}
			break;
		}
		break;
	}
	for(;i<str_len;i++)
	{
		if(string[i]==0)
			break;
		temp_value=_get_char_value(string[i]);
		if((temp_value <0)||(temp_value>=base))
			return -EINVAL;
		ret=ret*base+temp_value;		
	}
	return ret;
}
static inline int _get_lowest_bit(long long value)
{
	int i;
	int ret=0;
	int offset=sizeof(long long)*8/2;

	long long mask[6]=
	{
		0x00000000FFFFFFFF,
		0x0000FFFF0000FFFF,
		0x00FF00FF00FF00FF,
		0x0F0F0F0F0F0F0F0F,
		0x3333333333333333,
		0x5555555555555555,
	};
//	long long mask=-1;
	if(value==0)
		return 0;
	for(i=0;i<6;i++)
	{
		if(!(mask[i]&value))
		{
			ret+=offset;
		}
		else
		{
			value&=mask[i];
		}
		offset/=2;
	}
	return ret+1;	
}

int    Getlowestbit(BYTE  * addr,int size,int bit)
{
	long long test=0;
	int ret=0;
	int i;
	if(size<=0)
		return 0;
	if(size<=8)
	{
		Memcpy(&test,addr,size);	
		if(bit)
			return _get_lowest_bit(test);	
		else
			return _get_lowest_bit(~test);
	}
	for(i=0;i<size;i+=8)
	{
		test=0;
		if(i+8>size)
			Memcpy(&test,addr+i,size-i);
		else
			Memcpy(&test,addr+i,8);
		if(bit)
		{
			if(test==0)
			{
				ret+=64;
				continue;
			}
			return ret+_get_lowest_bit(test);
		}
		else
		{
			if(test==-1)
			{
				ret+=64;
				continue;
			}
			return ret+_get_lowest_bit(~test);
		}
	}
	return 0;
} 

int Isvaliduuid(char * uuid)
{
	int i;
	for(i=0;i<DIGEST_SIZE*2;i++)
	{
		if(_ishexdigit(uuid[i]))
			continue;
		return 0;
	}	
	return 1;
}

// if a DIGEST_SIZE's bin array's last half are all zero, then it should be a string
int Isstrinuuid(BYTE * uuid)
{

	BYTE compvalue[DIGEST_SIZE];
	Memset(compvalue,0,DIGEST_SIZE);
	int i;	
	int len;

	len=Strnlen(uuid,DIGEST_SIZE);

	if(len>DIGEST_SIZE/4*3)
		return 0;

	if(Memcmp(compvalue,uuid+len,DIGEST_SIZE-len)==0)
	{
		if(len<=DIGEST_SIZE/4*3)
			return 1;
		for(i=0;i<len;i++)
		{
			if(uuid[i]>=127)
				return 0;
		}
		return 1;
	}
	return 0;
}

int Isemptyuuid(BYTE * uuid)
{

	BYTE compvalue[DIGEST_SIZE];
	Memset(compvalue,0,DIGEST_SIZE);
	
	if(Memcmp(compvalue,uuid,DIGEST_SIZE)==0)
		return 1;
	return 0;
}


int bitmap_set(BYTE * bitmap, int site)
{
	unsigned char c=1;
	c<<=site%8;
        bitmap[site/8] |=c;
	return 0;
}
int bitmap_clear(BYTE * bitmap, int site)
{
	unsigned char c=1;
	c<<=site%8;
        bitmap[site/8] &=~c;
	return 0;
}

int bitmap_get(BYTE * bitmap,int site)
{
	unsigned char c=1;
	c<<=site%8;
        return bitmap[site/8] &c;
}

int bitmap_is_allset(BYTE * bitmap,int size)
{
	unsigned char c=0x7f;

	int i;
	for(i=0;i<size/8;i++)
	{
		if(bitmap[i] !=0xff)
			return 0;
	}
	if(size%8==0)
		return 1;
	c>>=7-size%8;
	if(bitmap[i]!=c)
		return 0;
	return 1;
}
int bin_2_hex(BYTE * bin,int size,BYTE * hex)
{
	int i;
	BYTE tempdata;
	for(i=0;i<size;i++)
	{
		tempdata=bin[i]>>4;
		if(tempdata>9)
			hex[i*2]=tempdata-10+'a';
		else
			hex[i*2]=tempdata+'0';
		tempdata=bin[i] &0x0f;
		if(tempdata>9)
			hex[i*2+1]=tempdata-10+'a';
		else
			hex[i*2+1]=tempdata+'0';
	}
	return size*2;
}

int hex_2_bin(BYTE * hex,int size,BYTE * bin)
{
	int i;
	int len;
	unsigned char var;
	len=Strnlen(hex,size);

	if(len%2!=0)
		return -EINVAL;

	for(i=0;i<len;i++)
	{
		if((hex[i]>='0') &&(hex[i]<='9'))
			var=hex[i]-'0';
		else if((hex[i]>='a')&&(hex[i]<='f'))
			var=hex[i]-'a';
		else if((hex[i]>='A')&&(hex[i]<='F'))
			var=hex[i]-'A';
		else
			return -EINVAL;
		if(i%2==0)
			bin[i/2]=var*0x10;
		else
			bin[i/2]+=var;
								
	}
	return len/2;
}
