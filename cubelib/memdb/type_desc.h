#ifndef TYPE_DESC_H
#define TYPE_DESC_H

struct struct_elem_attr uuid_head_desc[] =
{
	{"uuid",CUBE_TYPE_UUID,DIGEST_SIZE,NULL,NULL},
	{"name",CUBE_TYPE_STRING,DIGEST_SIZE,NULL,NULL},
	{"type",CUBE_TYPE_RECORDTYPE,sizeof(int),NULL,NULL},
	{"subtype",CUBE_TYPE_RECORDSUBTYPE,sizeof(int),NULL,"type"},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};


struct struct_elem_attr elem_attr_octet_desc[] =
{
	{"name",CUBE_TYPE_ESTRING,sizeof(char *),NULL,NULL},
	{"type",CUBE_TYPE_ELEMTYPE,sizeof(int),NULL,NULL},
	{"size",CUBE_TYPE_INT,sizeof(int),NULL,NULL},
	{"ref",CUBE_TYPE_UUID,DIGEST_SIZE,NULL,NULL},
	{"ref_name",CUBE_TYPE_STRING,DIGEST_SIZE,NULL,NULL},
	{"def",CUBE_TYPE_STRING,DIGEST_SIZE,NULL,NULL},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL}
};

struct struct_elem_attr struct_define_desc[] =
{
	{"elem_no",CUBE_TYPE_INT,sizeof(int),NULL,NULL},
	{"elem_desc",CUBE_TYPE_DEFARRAY,sizeof(void *),&elem_attr_octet_desc,"elem_no"},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};
struct struct_elem_attr struct_namelist_desc[] =
{
	{"elem_no",CUBE_TYPE_INT,sizeof(int),NULL,NULL},
	{"elemlist",CUBE_TYPE_DEFNAMELIST,sizeof(void *),NULL,"elem_no"},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};

struct struct_elem_attr struct_typelist_desc[] =
{
	{"elem_no",CUBE_TYPE_INT,sizeof(int),NULL,NULL},
	{"uuid",CUBE_TYPE_UUID,DIGEST_SIZE,NULL,NULL},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};

struct struct_elem_attr struct_subtypelist_desc[] =
{
	{"type",CUBE_TYPE_RECORDTYPE,sizeof(int),NULL},
	{"elem_no",CUBE_TYPE_INT,sizeof(int),NULL,NULL},
	{"uuid",CUBE_TYPE_UUID,DIGEST_SIZE,NULL,NULL},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};

struct struct_elem_attr index_list_desc[] =
{
	{"flag",CUBE_TYPE_FLAG,sizeof(int),&elem_attr_flaglist_array,NULL},
	{"elemlist",CUBE_TYPE_ESTRING,sizeof(void *),NULL,NULL},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};
struct struct_elem_attr struct_recordtype_desc[] =
{
	{"type",CUBE_TYPE_RECORDTYPE,sizeof(int),NULL,NULL},
	{"subtype",CUBE_TYPE_RECORDSUBTYPE,sizeof(int),NULL,"type"},
	{"uuid",CUBE_TYPE_UUID,DIGEST_SIZE,NULL,NULL},
	{"flag_no",CUBE_TYPE_INT,sizeof(int),NULL,NULL},
	{"index",CUBE_TYPE_DEFARRAY,sizeof(void *),&index_list_desc,"flag_no"},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};
#endif
