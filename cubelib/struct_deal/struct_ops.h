/*************************************************
*       Hige security Linux Operating System Project
*
*	File description: 	Definition of data describe struct header file
*	File name:		struct_deal.h
*	date:    	2008-05-09
*	Author:    	Hu jun
*************************************************/
#ifndef  STRUCT_OPS_H
#define  STRUCT_OPS_H

enum elem_attr_flag
{
	ELEM_ATTR_POINTER=0x01,
	ELEM_ATTR_DEFINE=0x02,
	ELEM_ATTR_ARRAY=0x04,
	ELEM_ATTR_ENUM=0x10,
	ELEM_ATTR_FLAG=0x20,
	ELEM_ATTR_SUBSET=0x40,
	ELEM_ATTR_VALUE=0x80,
	ELEM_ATTR_NUM=0x100,
	ELEM_ATTR_BOOL=0x200,
	ELEM_ATTR_NAMELIST=0x400,
	ELEM_ATTR_REF=0x800,
	ELEM_ATTR_EMPTY=0x1000,

};

struct InitElemInfo_struct
{
	enum cube_struct_elem_type type;
	void * enum_ops; 
	int  flag;
	int  offset;
};
typedef struct tagvalue2pointer
{
	int value;
	void * pointer;
}VALUE2POINTER;

int struct_register_ops(int value,void * pointer,int flag,int offset);
int _isdefineelem(int type);
int _isarrayelem(int type);
int _ispointerelem(int type);
int _issubsetelem(int type);
int _isvalidvalue(int type);
int _isnumelem(int type);
int _isboolelem(int type);
int _isnamelistelem(int type);
void * _elem_get_addr(void * elem,void * addr);
ELEM_OPS * _elem_get_ops(void * elem);
int _elem_get_offset(void * elem);
int _elem_get_defvalue(void * elem,void * addr);
int _elem_set_defvalue(void * elem,void * addr,int value);
int get_fixed_elemsize(int type);
int _getelemjsontype(int type);
void * _elem_get_ref(void * elem);
int _elem_set_ref(void * elem,void * ref);

int _elem_get_bin_length(void * value,void * elem,void * addr);
int _elem_get_bin_value(void * addr,void * data,void * elem);
int _elem_set_bin_value(void * addr,void * data,void * elem);
int _elem_clone_value(void * addr,void * clone,void * elem);
int _elem_compare_value(void * addr,void * dest,void * elem);
int _elem_get_text_value(void * addr,char * text,void * elem);
int _elem_set_text_value(void * addr,char * text,void * elem);

#define DEFINE_TAG  0x00FFF000
struct elem_template
{
	struct struct_elem_attr * elem_desc;
	int offset;    // the offset of this elem compare to the father
	int size;      // this elem's size in the struct
	void * ref;    	
	void * def;	
	int flag;
	int index;
	int limit;
	struct elem_template * father;
};

typedef struct struct_template_node
{
	void * parent;
	int offset;
	int size;
	int elem_no;
	int flag;
	struct struct_elem_attr * struct_desc;
	struct elem_template * elem_list;
	int temp_var;
	
}STRUCT_NODE;

int _count_struct_num(struct struct_elem_attr * struct_desc);
STRUCT_NODE * _get_root_node(STRUCT_NODE * start_node);
char * _get_next_name(char * name,char * buffer);
void * _get_elem_by_name(void * start_node, char * name);

#endif
