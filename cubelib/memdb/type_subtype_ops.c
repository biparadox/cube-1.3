#include "../include/errno.h"
#include "../include/data_type.h"
#include "../include/memfunc.h"
#include "../include/alloc.h"
#include "../include/struct_deal.h"
#include "../include/basefunc.h"
#include "../include/memdb.h"
#include "../include/list.h"
#include "../include/attrlist.h"
#include "../struct_mod/struct_ops.h"
#include "memdb_internal.h"

int enumtype_get_text_value(void * addr,char * text,void * elem_template)
{

	struct elem_template * elem_attr=elem_template;
	struct struct_elem_attr * elem_desc = elem_attr->elem_desc;
	int enum_value;
	NAME2VALUE * enum_list;
	int len;
	int offset=0;
	int i;

	if(elemenumlist==NULL)
		return -EINVAL;
	if(elemenumlist->elemlist==NULL)
		return -EINVAL;

	enum_list=elemenumlist->elemlist;

	enum_value=*(int *)addr;
	if(enum_value<0)
		return -EINVAL;

	for(i=0;enum_list[i].name!=NULL;i++)
	{
		if(enum_list[i].value==enum_value)
		{
			len=Strlen(enum_list[i].name);
			Memcpy(text+offset,enum_list[i].name,len+1);
			offset+=len+1;
			return offset;
		}
	}
	if(enum_value==0)
	{
		len=Strlen(nulstring);
		Memcpy(text+offset,nulstring,len+1);
		offset+=len+1;
		return offset;
	}

	return -EINVAL;
}

int enumtype_set_text_value(void * addr,void * text,void * elem_template){
	struct elem_template * elem_attr=elem_template;
	struct struct_elem_attr * elem_desc = elem_attr->elem_desc;
	int enum_value;
	NAME2VALUE * enum_list;
	int len;
	int offset=0;
	int i;

	if(elemenumlist==NULL)
		return -EINVAL;
	if(elemenumlist->elemlist==NULL)
		return -EINVAL;

	enum_list=elemenumlist->elemlist;

	if(!Strcmp(nulstring,text))
	{
		enum_value=0;
		*(int *)addr=enum_value;
		return 0;
	}
	else
	{
		for(i=0;enum_list[i].name!=NULL;i++)
		{
			if(!Strcmp(enum_list[i].name,text))
			{
				enum_value=enum_list[i].value;
				*(int *)addr=enum_value;
				return enum_value;
			}
		}
	}
	return -EINVAL;
}

int recordtype_get_text_value(void * addr,char * text,void * elem_template)
{

	struct elem_template * elem_attr=elem_template;
	struct struct_elem_attr * elem_desc = elem_attr->elem_desc;
	int enum_value;
	NAME2VALUE * enum_list;
	int len;
	int offset=0;
	int i;

	if(typeenumlist==NULL)
		return -EINVAL;
	if(typeenumlist->elemlist==NULL)
		return -EINVAL;

	enum_list=typeenumlist->elemlist;

	enum_value=*(int *)addr;
	if(enum_value<0)
		return -EINVAL;

	for(i=0;i<typeenumlist->elem_no;i++)
	{
		if(enum_list[i].value==enum_value)
		{
			len=Strlen(enum_list[i].name);
			Memcpy(text+offset,enum_list[i].name,len+1);
			offset+=len+1;
			return offset;
		}
	}
	if(enum_value==0)
	{
		len=Strlen(nulstring);
		Memcpy(text+offset,nulstring,len+1);
		offset+=len+1;
		return offset;
	}

	return -EINVAL;
}

int recordtype_set_text_value(void * addr,void * text,void * elem_template){
	struct elem_template * elem_attr=elem_template;
	struct struct_elem_attr * elem_desc = elem_attr->elem_desc;
	int enum_value;
	NAME2VALUE * enum_list;
	int len;
	int offset=0;
	int i;

	if(typeenumlist==NULL)
		return -EINVAL;
	if(typeenumlist->elemlist==NULL)
		return -EINVAL;

	enum_list=typeenumlist->elemlist;

	if(!Strcmp(nulstring,text))
	{
		enum_value=0;
		*(int *)addr=enum_value;
		return 0;
	}
	else
	{
		for(i=0;i<typeenumlist->elem_no;i++)
		{
			if(!Strcmp(enum_list[i].name,text))
			{
				enum_value=enum_list[i].value;
				*(int *)addr=enum_value;
				return enum_value;
			}
		}
	}
	return -EINVAL;
}

int subtype_get_text_value(void * addr,char * text,void * elem){
	struct elem_template * elem_attr=elem;
	struct struct_elem_attr * elem_desc = elem_attr->elem_desc;
	int enum_value;
	struct struct_namelist * subtypelist;
	NAME2VALUE * enum_list;
	int len;
	int offset=0;
	int i;
	int defvalue;

	enum_value=*(int *)addr;

	if(enum_value==0)
	{
		len=Strlen(nulstring);
		Memcpy(text+offset,nulstring,len+1);
		offset+=len+1;
		return offset;
	}


	defvalue=elem_attr->index;
	if(defvalue<0)
		return -EINVAL;

	
	subtypelist=memdb_get_subtypelist(defvalue);

	if(subtypelist==NULL)
	{
		len=Itoa(enum_value,text);
		return len;
	}
	enum_list=subtypelist->elemlist;
	
	for(i=0;i<subtypelist->elem_no;i++)
	{
		if(enum_list[i].value==enum_value)
		{
			len=Strlen(enum_list[i].name);
			Memcpy(text+offset,enum_list[i].name,len+1);
			offset+=len+1;
			return offset;
		}
	}
	return -EINVAL;
}

int subtype_set_text_value(void * addr,void * text,void * elem){
	struct elem_template * elem_attr=elem;
	struct struct_elem_attr * elem_desc = elem_attr->elem_desc;
	int enum_value;
	struct struct_namelist * subtypelist;
	NAME2VALUE * enum_list;
	int len;
	int offset=0;
	int i;
	int defvalue;

	if(!Strcmp(nulstring,text))
	{
		enum_value=0;
		*(int *)addr=enum_value;
		return 0;
	}

	defvalue=elem_attr->index;
	if(defvalue<0)
		return -EINVAL;

	subtypelist=memdb_get_subtypelist(defvalue);
	if(subtypelist==NULL)
		return -EINVAL;
	enum_list=subtypelist->elemlist;
	if(enum_list==NULL)
	{
		enum_value=Atoi(text,DIGEST_SIZE);
		if(enum_value<0)
			return enum_value;
		*(int *)addr=enum_value;
		return enum_value;
		
	}
	else
	{
		for(i=0;i<subtypelist->elem_no;i++)
		{
			if(!Strcmp(enum_list[i].name,text))
			{
				enum_value=enum_list[i].value;
				*(int *)addr=enum_value;
				return enum_value;
			}
		}
	}
	return -EINVAL;
}

ELEM_OPS enumtype_convert_ops =
{
	.get_text_value = enumtype_get_text_value,
	.set_text_value = enumtype_set_text_value,
};

ELEM_OPS recordtype_convert_ops =
{
	.get_text_value = recordtype_get_text_value,
	.set_text_value = recordtype_set_text_value,
};
ELEM_OPS subtype_convert_ops =
{
	.get_text_value = subtype_get_text_value,
	.set_text_value = subtype_set_text_value,
};
