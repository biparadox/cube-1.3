#ifndef  CUBE_ALLOC_H
#define  CUBE_ALLOC_H

//int alloc_init(void * start_addr,int page_num);
//void * get_cube_pointer(UINT32 addr);
//UINT32 get_cube_addr(void * pointer);
//UINT32 get_cube_data(UINT32 addr); 
//UINT16 get_page();
//UINT32 page_get_addr(UINT16 page);
//UINT16 addr_get_page(UINT32 addr);
//UINT32 free_page(UINT16 page);

//UINT32 salloc(int size);
void * Calloc(int size);
//int Cgetfreecount(void);

int Galloc(void ** pointer,int size);
int Galloc0(void ** pointer,int size);
//int Ggetfreecount(void);

void * Talloc(int size);
void * Talloc0(int size);
//void Treset(void);
//void Tclear(void);
//int  Tisinmem(void *);
//int Tgetfreecount(void);

int Palloc(void ** pointer,int size);
int Palloc0(void ** pointer,int size);

int Free(void * pointer);
int Free0(void * pointer);
//void Cmemdestroy(void);
//void Gmemdestroy(void);
//void Tmemdestroy(void);
#endif
