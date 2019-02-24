/*************************************************
*       Hige security Linux Operating System Project
*
*	File description: 	Definition of data describe struct header file
*	File name:		struct_deal.h
*	date:    	2008-05-09
*	Author:    	Hu jun
*************************************************/
#ifndef  _CUBE_STRUCT_DEAL_H
#define  _CUBE_STRUCT_DEAL_H
enum cube_struct_elem_type   // describe types could be used in the struct
{
	CUBE_TYPE_STRING=0x01,   // an string with fixed size
	CUBE_TYPE_UUID,     // an string with fixed size
	CUBE_TYPE_INT,      // an 32-bit int
	CUBE_TYPE_ENUM,      // a 32-bit enum
	CUBE_TYPE_SENUM,      // a 16-bit enum
	CUBE_TYPE_BENUM,      // an 8-bit enum
	CUBE_TYPE_FLAG,      // a 32-bit flag
	CUBE_TYPE_SFLAG,      // a 16-bit flag
	CUBE_TYPE_BFLAG,      // an 8-bit flag
	CUBE_TYPE_TIME,     // an struct of time_t
	CUBE_TYPE_UCHAR,    // a unsigned octet
	CUBE_TYPE_USHORT,   // a 16-bit unsigned word
	CUBE_TYPE_LONGLONG, // a 64-bit longlong integer
	CUBE_TYPE_BINDATA,  // a sequence of octets with fixed size
	CUBE_TYPE_BITMAP,   // a sequence of octets with fixed size(just like BINDATA),but we use eight bin string (like 01001010) to show them	 
	CUBE_TYPE_HEXDATA,   // a sequence of octets with fixed size(just like BINDATA),but we use 2 hex string (like ce) to show them	 
	CUBE_TYPE_BINARRAY,   // an array of sequence of octets with fixed size, attr is the sequence's size, size is array's length	 
	CUBE_TYPE_UUIDARRAY,   // an array of sequence of octets with fixed size, attr is the sequence's size, size is array's length	 
	CUBE_TYPE_DEFUUIDARRAY,   // an array of sequence of octets with fixed size, attr is the sequence's size, size is array's length	 
	CUBE_TYPE_DEFNAMELIST,   // an array of sequence of octets with fixed size, attr is the sequence's size, size is array's length	 
	CUBE_TYPE_BOOL,  
	CUBE_TYPE_ESTRING,  // a variable length string ended with '\0'
	CUBE_TYPE_JSONSTRING,  // a variable length string encluded in "{}", "[]" or "\"\"" or "\'\'", it is only special in struct_json, other times,
       			        // it is same as ESTRING	
	CUBE_TYPE_NODATA,   // this element has no data
	CUBE_TYPE_ARRAY,   // this element use an pointer point to a fixed number element
	CUBE_TYPE_DEFARRAY,   // this element has no data
	CUBE_TYPE_DEFINE,	//an octets sequence whose length defined by a forhead element (an uchar, an ushort or a int element), the attr parameter 
				//show the element's name, 
	CUBE_TYPE_DEFSTR,	//a string whose length defined by a forhead element (an uchar, an ushort or a int element), the attr parameter 
				//show the element's name, 
	CUBE_TYPE_DEFSTRARRAY,	//a fixed string' s array whose elem number defined by a forhead element (an uchar, an ushort,a int element or 
				//a string like "72", the attr parameter show the forhead element's name, the elem_attr->size show how
			 	// the string's fixed length.
				// NOTE: there should not be any ' ' in the string.
				//
	CUBE_TYPE_SUBSTRUCT,    // this element describes a new struct in this site, attr points to the description of the new struct
        CUBE_TYPE_CHOICE,
		
	TPM_TYPE_UINT64,
	TPM_TYPE_UINT32,
	TPM_TYPE_UINT16,
	CUBE_TYPE_USERDEF=0x80,
	CUBE_TYPE_ENDDATA=0x100,
};

enum cube_struct_elem_attr
{
	CUBE_ELEM_FLAG_INDEX=0x01,
	CUBE_ELEM_FLAG_KEY=0x02,
	CUBE_ELEM_FLAG_INPUT=0x04,
	CUBE_ELEM_FLAG_DESC=0x10,
	CUBE_ELEM_FLAG_TEMP=0x1000,
};

#define nulstring "NULL"

// pointer stack function
struct struct_elem_attr
{
	char * name;  // this element's name
	enum cube_struct_elem_type type;  // this element's type
	int size;     // the size of this elem(only in fixed state	
	void * ref;   // a pointer to the reference of this elem
	void * def;   
};

typedef struct tagnameofvalue
{
	char * name;
	int value;
}__attribute__((packed)) NAME2VALUE;

struct struct_namelist
{
	int elem_no;
	NAME2VALUE * elemlist;
}__attribute__((packed));

typedef struct tagnameofpointer
{
	char * name;
	void * pointer;
}NAME2POINTER;

typedef struct elem_convert_ops
{
	int (*elem_size)(void * elem_attr);
	int (*elem_get_length)(void * value,void * elem_attr);
	int (*clone_elem)(void * elem,void * clone,void * elem_attr);
	int (*get_bin_value)(void * elem,void * addr,void * elem_attr);
	int(*set_bin_value)(void * elem,void * addr,void * elem_attr);
	int(*get_text_value)(void * elem,char * text,void * elem_attr);
	int(*set_text_value)(void * elem,char * text,void * elem_attr);
	int(*get_int_value)(void * elem,void * elem_attr);
	int(*comp_elem)(void * elem,void * tag,void * struct_template);
	int(*comp_elem_text)(void * elem,char * str,void * struct_template);
}ELEM_OPS;

int struct_deal_init(void );

// alloc and free the struct
int get_fixed_elemsize(int type);
int iselemneeddef(int type);
void * create_struct_template(struct struct_elem_attr * struct_desc);
void * clone_struct_template(void * struct_template);
void free_struct_template(void * struct_template);
void * struct_get_ref(void * struct_template, char * name);

int struct_set_ref(void * struct_template, char * name,void * ref);

int struct_free(void * addr, void * struct_template);
int struct_free_alloc(void * addr, void * struct_template);
int struct_size(void * struct_template);

int struct_set_flag(void * struct_template,int flag, char * namelist);
int struct_set_allflag(void * struct_template,int flag);
int struct_clear_flag(void * struct_template,int flag,char * namelist);
int struct_clear_allflag(void * struct_template,int flag);
int struct_get_flag(void * struct_template,char * name);

int struct_2_blob(void * addr, void * blob, void * struct_template);
int struct_2_part_blob(void * addr, void * blob, void * struct_template, int flag);
int part_blob_2_struct(void * blob, void * addr, void * struct_template, int flag);
int blob_2_struct(void * blob, void * addr, void * struct_template);
int struct_2_text(void * blob, char * string, void * struct_template);
int text_2_struct(char * string, void * blob, void * struct_template);


int struct_clone(void * src, void * destr, void * struct_template);
int struct_part_clone(void * src, void * destr, void * struct_template,int flag);
int struct_comp_elem_value(char * name,void * src, void * value, void * struct_template);
int struct_compare(void * src, void * destr, void * struct_template);
int struct_part_compare(void * src, void * destr, void * struct_template,int flag);


int struct_read_elem(char * name, void * addr, void * elem_data, void * struct_template);
int struct_write_elem(char * name, void * addr, void * elem_data, void * struct_template);
int struct_read_elem_text(char * name, void * addr, char * text, void * struct_template);
int struct_write_elem_text(char * name, void * addr, char * string, void * struct_template);
void * get_desc_from_template(void * struct_template);

char * dup_str(char * src, int size);
void * clone_struct(void * addr, void * struct_template);
//void * struct_get_elem_attr(char * name, void * struct_template);
//int struct_set_elem_var(char * name, void * attr, void * struct_template);
//void * read_elem_addr(char * name, void * template);

int struct_2_json(void * addr, char * json_str, void * template);
int struct_2_part_json(void * addr, char * json_str, void * struct_template, int flag);
int json_2_struct(void * root, void * addr, void * struct_template);
int json_2_part_struct(void * root, void * addr, void * struct_template,int flag);
int json_marked_struct(void * root, void * struct_template,int flag);

extern int deep_debug;
#endif
