/*************************************************
*       project:        973 trust demo, zhongan secure os 
*                       and trust standard verify
*	name:		data_type.h
*	write date:    	2011-08-04
*	auther:    	Hu jun
*       content:        this file describe some basic data type 
*       changelog:       
*************************************************/
#ifndef CUBE_DATA_TYPE_H
#define CUBE_DATA_TYPE_H

#ifndef TSS_PLATFORM_H
typedef unsigned char         BYTE;
typedef unsigned short int  UINT16;
typedef unsigned int        UINT32;
typedef unsigned long int   UINT64;
typedef unsigned short int    WORD;
typedef unsigned int         DWORD;

static BYTE EMPTY_UUID[32] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
							 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
							 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
						     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

#endif

#define BITSTRING (unsigned char *)
#define CUBE_DEBUG  

#define DIGEST_SIZE	32
#define PAGE_SIZE	4096
#ifndef NULL
#define NULL 		0
#endif

#define IS_ERR(ptr) (ptr-4096 <0)

#ifdef CUBE_DEBUG
#define cube_dbg(format, arg...)		\
	printk(KERN_DEBUG, format , ## arg)
#else
#define cube_dbg(format, arg...)  do { } while (0)
#endif
#endif
