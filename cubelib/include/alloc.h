#ifndef  CUBE_ALLOC_H
#define  CUBE_ALLOC_H

int alloc_init();

int alloc_pointer_type(void * pointer);

void * Calloc(int size);
void * Calloc0(int size);

void * Dalloc(int size,void * base);
void * Dalloc0(int size,void * base);

void * Talloc(int size);
void * Talloc0(int size);

void * Salloc(int size);
void * Salloc0(int size);

void * Palloc(int size,void * base);
int Palloc0(int size,void * base);

int Free(void * pointer);
int Free0(void * pointer);
#endif
