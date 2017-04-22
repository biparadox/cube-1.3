#ifndef  CUBE_MEMORY_H
#define  CUBE_MEMORY_H

#ifndef NULL
	#define NULL 0
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
#endif
