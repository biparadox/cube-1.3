#include "../include/errno.h"
#include "../include/data_type.h"
#include "../include/alloc.h"
#include "../include/string.h"
#include "../include/struct_deal.h"
#include "struct_ops.h"
#include "struct_attr.h"


//const int deftag=0x00FFF000;
//const char * nulstring="NULL";
int enum_get_text_value(void * addr,char * text,void * elem_template){
	struct elem_template * elem_attr=elem_template;
	struct struct_elem_attr * elem_desc = elem_attr->elem_desc;
	int enum_value;
	NAME2VALUE * enum_list;
	int len;
	int offset=0;
	int i;

	enum_list=elem_attr->ref;
	if(enum_list==NULL)
		enum_list=elem_desc->ref;

	switch(elem_desc->type)
	{
		case 	CUBE_TYPE_ENUM:
			enum_value=*(int *)addr;
			break;
		case 	CUBE_TYPE_SENUM:
			enum_value=*(UINT16 *)addr;
			break;
		case 	CUBE_TYPE_BENUM:
			enum_value=*(BYTE *)addr;
			break;
		default:
			return -EINVAL;
	}

	if(enum_value<0)
		return -EINVAL;

	if(enum_list==NULL)
	{
		len=Itoa(enum_value,text);
		return len;
	}
	
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

int enum_set_text_value(void * addr,void * text,void * elem_template){
	struct elem_template * elem_attr=elem_template;
	struct struct_elem_attr * elem_desc = elem_attr->elem_desc;
	int enum_value;
	NAME2VALUE * enum_list;
	int len;
	int offset=0;
	int i;
	enum_list=elem_attr->ref;
	if(enum_list==NULL)
		enum_list=elem_desc->ref;
	
	if(!Strcmp(nulstring,text))
	{
		enum_value=0;
		*(int *)addr=enum_value;
		return 0;
	}
	else if(enum_list==NULL)
	{
		enum_value=Atoi(text,DIGEST_SIZE);
		if(enum_value<0)
			return enum_value;
		*(int *)addr=enum_value;
		
	}
	else
	{
		for(i=0;enum_list[i].name!=NULL;i++)
		{
			if(!Strcmp(enum_list[i].name,text))
			{
				enum_value=enum_list[i].value;
			}
		}
	}
	switch(elem_desc->type)
	{
		case 	CUBE_TYPE_ENUM:
			*(int *)addr=enum_value;
			break;
		case 	CUBE_TYPE_SENUM:
			if(enum_value>32767)
				return -EINVAL;
			*(UINT16 *)addr=enum_value;
			break;
		case 	CUBE_TYPE_BENUM:
			if(enum_value>127)
				return -EINVAL;
			*(BYTE *)addr=enum_value;
			break;
		default:
			return -EINVAL;
	}

	return enum_value;
}


int flag_get_text_value(void * addr, char * text, void * elem_template){

	struct elem_template * elem_attr=elem_template;
	struct struct_elem_attr * elem_desc = elem_attr->elem_desc;
	int flag_value;
	int retval;
	int offset=0;
	NAME2VALUE * flag_list;
	int len;
	int i,j;

	flag_list=elem_attr->ref;
	if(flag_list==NULL)
		flag_list=elem_desc->ref;
	if(flag_list==NULL)
		return -EINVAL;
	

	switch(elem_desc->type)
	{
		case 	CUBE_TYPE_FLAG:
			flag_value=*(int *)addr;
			break;
		case 	CUBE_TYPE_SFLAG:
			flag_value=*(UINT16 *)addr;
			break;
		case 	CUBE_TYPE_BFLAG:
			flag_value=*(BYTE *)addr;
			break;
		default:
			return -EINVAL;
	}
	
	for(i=0;flag_list[i].name!=NULL;i++)
	{
		j=0;
		if(flag_list[i].value & flag_value)
		{
			if(j!=0)
			{
				text[offset++]='|';
			}
			len=Strlen(flag_list[i].name);
			Memcpy(text+offset,flag_list[i].name,len);
			offset+=len;
		}
	}
	if(offset==0)
	{
		len=Strlen(nulstring);
		Memcpy(text+offset,nulstring,len);
		offset+=len;
	}
	text[offset++]=0;
	return offset;
}

int flag_set_text_value(void * addr, char * text, void * elem_template){

	struct elem_template * elem_attr=elem_template;
	struct struct_elem_attr * elem_desc = elem_attr->elem_desc;
	int flag_value;
	int retval;
	int offset=0;
	NAME2VALUE * flag_list;
	int i,j;

	flag_list=elem_attr->ref;
	if(flag_list==NULL)
		flag_list=elem_desc->ref;
	if(flag_list==NULL)
		return -EINVAL;
	
	flag_value=0;
	if(!Strcmp(nulstring,text))
	{
		*(int *)addr=flag_value;
		return 0;
	}
	char temp_string[DIGEST_SIZE];
	for(i=0;i<Strlen(text);i++)
	{
		if(text[i]=='|')
			continue;
		temp_string[offset++]=text[i];
		if((text[i+1]!='|') && (text[i+1]!=0))
			continue;
		temp_string[offset]=0;
		offset=0;
		for(j=0;flag_list[j].name!=NULL;j++)
		{
			if(Strcmp(flag_list[j].name,temp_string))
				continue;
			flag_value |= flag_list[j].value;
			break;
		}
		if(flag_list[j].name==NULL)
			return -EINVAL;
	}
	switch(elem_desc->type)
	{
		case 	CUBE_TYPE_FLAG:
			*(int *)addr=flag_value;
			break;
		case 	CUBE_TYPE_SFLAG:
			*(UINT16 *)addr=flag_value;
			break;
		case 	CUBE_TYPE_BFLAG:
			*(BYTE *)addr=flag_value;
			break;
		default:
			return -EINVAL;
	}

	return 0;
}


ELEM_OPS enum_convert_ops =
{
//	.calculate_offset=Calculate_offset,
	.get_text_value = enum_get_text_value,
	.set_text_value = enum_set_text_value,
};

ELEM_OPS flag_convert_ops =
{
//	.calculate_offset=Calculate_offset,
	.get_text_value = flag_get_text_value,
	.set_text_value = flag_set_text_value,
};
