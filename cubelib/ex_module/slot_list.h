#ifndef SLOT_LIST_FUNC_H
#define SLOT_LIST_FUNC_H

int slot_list_addslot(void * list,void * port);
void * slot_list_init( );
void slot_port_free(void * slot_port);
void * slot_list_getfirst(void * list);
void * slot_list_getnext(void * list);

void * slot_list_removesock(void * list,BYTE * uuid);
void * slot_list_findport(void * list,char * name);

void * slot_list_findsock(void * list,BYTE * uuid);
void * slot_create_sock(void * port,BYTE * uuid);

#endif 
