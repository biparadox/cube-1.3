/*************************************************
*       Hige security Linux Operating System Project
*
*	File description: 	Definition of data describe struct header file
*	File name:		struct_deal.h
*	date:    	2008-05-09
*	Author:    	Hu jun
*************************************************/
#ifndef  _CUBE_JSON_H
#define  _CUBE_JSON_H

enum json_elem_type
{
    JSON_ELEM_NULL=0,
    JSON_ELEM_NUM,
    JSON_ELEM_STRING,
    JSON_ELEM_BOOL,
    JSON_ELEM_MAP,
    JSON_ELEM_ARRAY,
};

// pointer stack function


void * json_find_elem(char * name, void * root);
void * json_get_first_child(void * father);
void * json_get_next_child(void * father);
void * json_get_father(void * child);
int json_node_getvalue(void * node, void * value, int max_len);
int json_node_getname(void * node, char * name);
int json_is_value(void * node);

int json_solve_str(void ** root, char *str);
int json_print_str(void * root, char *str);
int json_get_type(void * node);
int json_get_elemno(void * node);
char * json_get_valuestr(void * node);
char * json_set_valuestr(void * node,char * str);
int json_set_type(void * node,int datatype,int isvalue);

int  json_node_set_no(void * node,int no);
int  json_node_get_no(void * node);
int  json_node_set_pointer(void * node,void * pointer);
void * json_node_get_pointer(void * node);
int json_remove_node(void * node);
void * json_add_child(void * node,void * child);
#endif
