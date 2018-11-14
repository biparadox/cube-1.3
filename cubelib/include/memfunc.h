#ifndef  CUBE_MEMORY_H
#define  CUBE_MEMORY_H

#ifndef NULL
	#define NULL 0

#define Struct_elem_offset(ptr,type,member) \
	((unsigned long)(&((type *)0)->member))

#define Struct_elemtail_offset(ptr,type,member) \
	((unsigned long)(&((type *)0)->member)+sizeof(((type *)ptr)->member))

#define Struct_elem_addr(ptr,type,member) \
	((void *)((char *)(ptr)+(unsigned long)(&((type *)0)->member)))

#endif
void * Memcpy(void * dest,void * src, unsigned int count);
void * Memset(void * s,int c, int n);
int    Memcmp(const void *s1,const void *s2,int n);

char * Strcpy(char *dest,const char *src);
char * Strncpy(char *dest,const char *src,int n);
int    Strcmp(const char *s1,const char *s2);
int    Strncmp(const char *s1,const char *s2,int n);
char * Strcat(char * dest,const char *src);
char * Strncat(char * dest,const char *src,int n);
int    Strlen(char * str);
int    Strnlen(char * str,int n);

int    Getfiledfromstr(char * name,char * str,char IFS,int maxsize);
int    Itoa(int n,char * str);
int    Atoi(char * str,int maxlen);
int    Getlowestbit(BYTE * addr,int size,int bit); 
static inline int  Ischarinset(BYTE char_value,char * set)
{
	int i=0;
	while(set[i]!=0)
	{
		if(char_value==set[i++])
			return 1;
	}	
	return 0;
}

int   Isvaliduuid(char * uuid);
int   Isstrinuuid(BYTE * uuid);
int   Isemptyuuid(BYTE * uuid);

int bitmap_set(BYTE * bitmap,int site);
int bitmap_clear(BYTE * bitmap,int site);
int bitmap_get(BYTE * bitmap,int site);
int bitmap_is_allset(BYTE * bitmap,int size);
int bin_2_hex(BYTE * bin,int size,BYTE * hex);
int hex_2_bin(BYTE * hex,int size,BYTE * bin);
int bin_to_radix64_len(int length);
int radix_to_bin_len(int length);
int bin_to_radix64(char * radix64,int length,unsigned char * bin); 
int radix64_to_bin(unsigned char * bin,int length, char * radix64); 

#endif
