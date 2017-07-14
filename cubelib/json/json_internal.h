#include"../include/data_type.h"
#include"../include/list.h"
#include"../include/attrlist.h"
#define os210_print_buf 4096
#define OS210_MAX_BUF   4096

//#define nulstring "NULL"

#define  MAX_ARRAY_ELEM_NUM  128

#define  JSON_ELEM_VALUE 	0x1000
#define  JSON_ELEM_MASK 	(JSON_ELEM_VALUE-1)
static inline int _jsonelem_get_type(int type)
{
	return type & JSON_ELEM_MASK;
}
static inline int _jsonelem_isvalue(int type)
{
	return type & JSON_ELEM_VALUE;
}

typedef struct json_elem_node
{
    int elem_type;            //  this json elem's type, it can be
                             //  NUM,STRING,BOOL,MAP,ARRAY or NULL

    char name[DIGEST_SIZE]; // this json elem's name,
                              // if this json elem is the root elem,
                              //  its name is "__ROOT__"
    char * value_str;         // this json elem's value string
    long long  value;         //  if solve_state is JSON_SOLVE_PROCESS,
                              //  it is this json elem str's offset, if
                              // solve_state is JSON_SOLVE_FINISH,it is
                              // this json elem str's length                         // this
    int elem_no;              // how many elem in this json_node

    Record_List childlist;    // this json's child list
    Record_List * curr_child; // when this json elme is solved, this
                              // pointer point to the child it was
                              // processing.
    struct json_elem_node * father;
    int    comp_no;
    void *  comp_pointer;	
}JSON_NODE;
