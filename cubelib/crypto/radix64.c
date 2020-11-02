//#include<linux/string.h>
//#include<linux/errno.h>

#include "../include/data_type.h"
#include "../include/string.h"
#include "../include/errno.h"

#include "../include/radix64.h"

//test

const char  radix64_tab[65] ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

int bin_to_radix64(char * radix64, int length,unsigned char * bin)
{
    unsigned char tempbyte;
    int i,j,k;
    int radix64_length;
    radix64_length=0;
    j=0;
    k=0;
    for(i=0;i<length;i+=3)
    {
	radix64[radix64_length]=bin[i]>>2;
	radix64_length++;
	radix64[radix64_length]=(bin[i]%4)<<4;	
        if(length-i ==1)
	{
		radix64_length++;
		break;
	}
	radix64[radix64_length]+=bin[i+1]>>4;	
	radix64_length++;
	radix64[radix64_length]=(bin[i+1]%16)<<2;	
        if(length-i ==2)
	{
		radix64_length++;
		break;
	}
	radix64[radix64_length]+=bin[i+2]>>6;	
	radix64_length++;
	radix64[radix64_length]=bin[i+2]%64;    
    	radix64_length++;
    }
    for(i=0;i<radix64_length;i++)
    {
	radix64[i]=radix64_tab[radix64[i]];   
    }
    radix64[radix64_length]=0;
    return radix64_length;	
}

static __inline__ unsigned char bin_from_radix64_char(char radix64)
{
	if((radix64>='A') && (radix64<='Z'))
		return radix64-'A';
	if((radix64>='a') && (radix64<='z'))
		return radix64-'a'+26;
	if((radix64>='0') && (radix64<='9'))
		return radix64-'0'+26*2;
	if(radix64=='+')
		return 62;
	if(radix64=='/') 
		return 63;
	if(radix64=='=') 
		return 64;
		return 0xFF;
}

int radix64_to_bin(unsigned char * bin,int length, char * radix64) 
{
    unsigned char bin_buffer[3];
    unsigned char tempbyte[4];
    int i,j;
    int index;

    if(length%4==1)
	return -EINVAL;
    
    for(i=0;i<length;i+=4)
    {	
	   Memset(bin_buffer,0,3);
	   for(j=i;(j<length) && (j<i+4);j++)
	   {
		 tempbyte[j-i] = bin_from_radix64_char(radix64[j]);
	         if(tempbyte[j-i] == 0xFF)
			return -EINVAL;	 
	   }
	   bin_buffer[0]=(tempbyte[0]<<2)+(tempbyte[1]>>4);
	   if(length-i == 2)
	   {
		Memcpy(bin+i/4*3,bin_buffer,1);
	     	return length/4*3+1;	
	   }
	   bin_buffer[1]=(tempbyte[1]<<4)+(tempbyte[2]>>2);
	   if(length-i == 3)
	   {
		Memcpy(bin+i/4*3,bin_buffer,2);
	     	return length/4*3+2;	
	   }
	   bin_buffer[2]=(tempbyte[2]<<6)+tempbyte[3];
	   Memcpy(bin+i/4*3,bin_buffer,3);
	   
    }
    i=length/4*3;
    return i;
}
	
int bin_to_radix64_len(int length)
{
	return (length*4+2)/3;
}
int radix_to_bin_len(int length)
{
    if(length%4==1)
	return -EINVAL;
    if(length%4==0)
	return length/4*3;	
    return length/4*3+length%4-1;	
}
