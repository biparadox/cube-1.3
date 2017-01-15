#include "../include/errno.h"
#include "../include/data_type.h"
#include "../include/alloc.h"
#include "../include/struct_deal.h"
#include "../include/string.h"
#include "../include/json.h"
#include "struct_ops.h"
#include "struct_attr.h"


static struct InitElemInfo_struct InitElemInfo [] =
{
	{CUBE_TYPE_STRING,&string_convert_ops,ELEM_ATTR_VALUE,-1},
	{CUBE_TYPE_ESTRING,&estring_convert_ops,ELEM_ATTR_VALUE|ELEM_ATTR_POINTER,0},
	{CUBE_TYPE_BINDATA,&bindata_convert_ops,0,-1},
	{CUBE_TYPE_DEFINE,&define_convert_ops,ELEM_ATTR_POINTER|ELEM_ATTR_DEFINE,0},
	{CUBE_TYPE_UUID,&uuid_convert_ops,0,DIGEST_SIZE},
	{CUBE_TYPE_UUIDARRAY,&uuidarray_convert_ops,ELEM_ATTR_POINTER|ELEM_ATTR_ARRAY,0},
	{CUBE_TYPE_DEFUUIDARRAY,&defuuidarray_convert_ops,ELEM_ATTR_POINTER|ELEM_ATTR_ARRAY|ELEM_ATTR_DEFINE,0},
	{CUBE_TYPE_DEFNAMELIST,&defnamelist_convert_ops,ELEM_ATTR_POINTER|ELEM_ATTR_DEFINE|ELEM_ATTR_ARRAY,sizeof(void *)+sizeof(int)},
	{CUBE_TYPE_INT,&int_convert_ops,ELEM_ATTR_VALUE|ELEM_ATTR_NUM,sizeof(int)},
	{CUBE_TYPE_UCHAR,&int_convert_ops,ELEM_ATTR_VALUE|ELEM_ATTR_NUM,sizeof(char)},
	{CUBE_TYPE_USHORT,&int_convert_ops,ELEM_ATTR_VALUE|ELEM_ATTR_NUM,sizeof(short)},
	{CUBE_TYPE_LONGLONG,&int_convert_ops,ELEM_ATTR_VALUE|ELEM_ATTR_NUM,sizeof(long long)},
	{TPM_TYPE_UINT64,&tpm_uint64_convert_ops,ELEM_ATTR_VALUE|ELEM_ATTR_NUM,8},
	{TPM_TYPE_UINT32,&tpm_uint32_convert_ops,ELEM_ATTR_VALUE|ELEM_ATTR_NUM,4},
	{TPM_TYPE_UINT16,&tpm_uint16_convert_ops,ELEM_ATTR_VALUE|ELEM_ATTR_NUM,2},
	{CUBE_TYPE_ENUM,&enum_convert_ops,ELEM_ATTR_NAMELIST,sizeof(int)},
	{CUBE_TYPE_SENUM,&enum_convert_ops,ELEM_ATTR_NAMELIST,sizeof(UINT16)},
	{CUBE_TYPE_BENUM,&enum_convert_ops,ELEM_ATTR_NAMELIST,sizeof(BYTE)},
	{CUBE_TYPE_FLAG,&flag_convert_ops,ELEM_ATTR_NAMELIST,sizeof(int)},
	{CUBE_TYPE_SFLAG,&flag_convert_ops,ELEM_ATTR_NAMELIST,sizeof(UINT16)},
	{CUBE_TYPE_BFLAG,&flag_convert_ops,ELEM_ATTR_NAMELIST,sizeof(BYTE)},
	{CUBE_TYPE_SUBSTRUCT,NULL,ELEM_ATTR_SUBSET,0},
	{CUBE_TYPE_ARRAY,NULL,ELEM_ATTR_POINTER|ELEM_ATTR_ARRAY|ELEM_ATTR_SUBSET,0},
	{CUBE_TYPE_ENDDATA,NULL,ELEM_ATTR_EMPTY,0},
};

static void ** struct_deal_ops;
int * struct_deal_flag;
int * struct_deal_offset;
static void * ops_list[CUBE_TYPE_ENDDATA];
static int  flag_list[CUBE_TYPE_ENDDATA];
static int  offset_list[CUBE_TYPE_ENDDATA];

int struct_register_ops(int value,void * pointer,int flag,int offset)
{
	if((value<0) || (value>=CUBE_TYPE_ENDDATA))
		return -EINVAL;
	ops_list[value]=pointer;
	flag_list[value]=flag;
	offset_list[value]=offset;
	return 0; 
}

static inline int _testelemattr(int type, int flag)
{
	if((type<0) || (type >=CUBE_TYPE_ENDDATA))
		return -EINVAL;
	return flag_list[type]&flag;

}

int _isdefineelem(int type)
{
	return _testelemattr(type,ELEM_ATTR_DEFINE);
}

int _isarrayelem(int type)
{
	return _testelemattr(type,ELEM_ATTR_ARRAY);
}

int _ispointerelem(int type)
{
	return _testelemattr(type,ELEM_ATTR_POINTER);
}

int _issubsetelem(int type)
{
	return _testelemattr(type,ELEM_ATTR_SUBSET);
}

int _isvalidvalue(int type)
{
	return _testelemattr(type,ELEM_ATTR_VALUE);
}

int _isnumelem(int type)
{
	return _testelemattr(type,ELEM_ATTR_NUM);
}

int _isboolelem(int type)
{
	return _testelemattr(type,ELEM_ATTR_BOOL);
}

int _isemptyelem(int type)
{
	return _testelemattr(type,ELEM_ATTR_EMPTY);
}
int _isnamelistelem(int type)
{
	return _testelemattr(type,ELEM_ATTR_NAMELIST);
}

static inline int _is_elem_in_subset(void * elem,void * subset)
{
	struct elem_template * curr_elem=elem;
	struct elem_template * curr_subset=subset;
	
	if(_issubsetelem(curr_subset->elem_desc->type))
	{
		while(curr_elem->father!=NULL)
		{
			if(curr_elem->father==subset)
				return 1;
			curr_elem=curr_elem->father;
		}
	}
	return 0;
}

ELEM_OPS * _elem_get_ops(void * elem)
{
	struct elem_template * curr_elem=elem;
	if(curr_elem==NULL)
		return NULL;
	if(curr_elem->elem_desc==NULL)
		return NULL;
	if((curr_elem->elem_desc->type<0)|| (curr_elem->elem_desc->type>CUBE_TYPE_ENDDATA))
		return NULL;
	int index=curr_elem->elem_desc->type;
	return struct_deal_ops[index];  

}

void * _elem_get_addr(void * elem,void * addr)
{
	int offset_array[10];
	int limit=0;
	int i;
	Memset(offset_array,0,sizeof(offset_array));

	struct elem_template * curr_elem=elem;

	while(curr_elem!=NULL)
	{
		if(_issubsetelem(curr_elem->elem_desc->type))
		{
			if(_ispointerelem(curr_elem->elem_desc->type))
			{
				offset_array[limit]+=curr_elem->size * curr_elem->index;
				offset_array[++limit]=curr_elem->offset;
			}
			else 
			{
				offset_array[limit]+=curr_elem->offset+curr_elem->size*curr_elem->index;
			}
		}
		else
		{
			offset_array[limit]+=curr_elem->offset;
		}
		curr_elem=curr_elem->father;
	}
	

	for(i=limit;i>0;i--)
	{
		addr=*((void **)(addr+offset_array[i]));
		if(addr==NULL)
			return NULL;
		
	}
	return addr+offset_array[0];
}

int _elem_get_offset(void * elem)
{
	int offset=0;
	struct elem_template * curr_elem=elem;

	while(curr_elem!=NULL)
	{
		if(_issubsetelem(curr_elem->elem_desc->type) &&
			_ispointerelem(curr_elem->elem_desc->type))
			offset+=curr_elem->offset +
				curr_elem->size * curr_elem->index;
		else
			offset+=curr_elem->offset;
		
		curr_elem=curr_elem->father;
	}
	return offset;	
	
}

int _elem_get_defvalue(void * elem,void * addr)
{
	struct elem_template * curr_elem=elem;
	struct elem_template * temp_elem=curr_elem->def;
	struct elem_template * temp_subset;
	ELEM_OPS * elem_ops;
	int define_value;
	void * def_addr;
	if(temp_elem==NULL)
		return -EINVAL;
	elem_ops=_elem_get_ops(temp_elem);
	if(elem_ops==NULL)
		return NULL;


	// now compute the define elem's offset

	// if define elem is the first layer child
	if(temp_elem->father!=NULL)
	{
		if(!_is_elem_in_subset(curr_elem,temp_elem->father))
			return -EINVAL;	
	}

	def_addr=_elem_get_addr(temp_elem,addr);
	// if define elem is an elem in curr_elem's father subset
	if(elem_ops->get_int_value == NULL)
	{
		define_value = *(int *)def_addr;
	}
	else
	{
		define_value=elem_ops->get_int_value(def_addr,temp_elem);
	}
	if((define_value<0) || (define_value >=1024))
		return -EINVAL;
	return define_value;
}

int _elem_set_defvalue(void * elem,void * addr,int value)
{
	struct elem_template * curr_elem=elem;
	struct elem_template * temp_elem=curr_elem->def;
	ELEM_OPS * elem_ops;
	void * def_addr;
	int ret;
	if(temp_elem==NULL)
		return -EINVAL;
	
	if((value<0)|| (value>1024))
		return -EINVAL;
	

	elem_ops=_elem_get_ops(temp_elem);
	if(elem_ops==NULL)
		return NULL;

	// now compute the define elem's offset

	def_addr=_elem_get_addr(temp_elem,addr);
	// if define elem is an elem in curr_elem's father subset
	
	char buffer[DIGEST_SIZE];
	if((temp_elem->elem_desc->type == CUBE_TYPE_STRING)
		||(temp_elem->elem_desc->type == CUBE_TYPE_ESTRING))
	{
		ret=Itoa(value,buffer);
		if(ret<0)
			return -EINVAL;
		if(elem_ops->set_text_value==NULL)
		{
			int str_len=Strlen(buffer);
			if(_ispointerelem(temp_elem->elem_desc->type))
			{
				int tempret=Palloc0(def_addr,str_len+1);
				if(tempret<0)
					return tempret;
				Memcpy(*(char **)def_addr,buffer,str_len+1);
			}
			else
			{
				Memcpy(def_addr,buffer,str_len);
			}
			
		}
		else
		{
			ret=elem_ops->set_text_value(def_addr,buffer,temp_elem);
			if(ret<0)
				return -EINVAL;
		}
	}
	else
	{
		if(elem_ops->set_bin_value==NULL)
		{
			ret=get_fixed_elemsize(temp_elem->elem_desc->type);
			if(ret<=0)
				return -EINVAL;
			Memcpy(def_addr,&value,ret);
		}
		else
		{
			ret=elem_ops->set_bin_value(def_addr,&value,temp_elem);
			if(ret<0)
				return -EINVAL;
		}
	}
	return value;
}

int struct_deal_init()
{
	int count;
	int ret;
	int i;
	struct_deal_ops=&ops_list;
	for(i=0;i<CUBE_TYPE_ENDDATA;i++)
	{
		struct_deal_ops[i]=NULL;
	}
	
	for(i=0;InitElemInfo[i].type!=CUBE_TYPE_ENDDATA;i++)
	{
		ret=struct_register_ops(InitElemInfo[i].type,
			InitElemInfo[i].enum_ops,
			InitElemInfo[i].flag,
			InitElemInfo[i].offset);
		if(ret<0)
			return ret;
	}
	return 0;		
}

int _count_struct_num(struct struct_elem_attr * struct_desc)
{
	int i=0;
	while(struct_desc[i].name!=NULL)
	{
		if(struct_desc[i].type==CUBE_TYPE_ENDDATA)
			break;
		i++;
	}
	return i;
}

const int max_name_len=DIGEST_SIZE;

STRUCT_NODE * _get_root_node(STRUCT_NODE * start_node)
{
	while( start_node->parent != NULL)
		start_node=start_node->parent;
	return start_node;
}

char * _get_next_name(char * name,char * buffer)
{
	int i;
	for(i=0;i<max_name_len+1;i++)
	{
		if(name[i]==0)
		{
			buffer[i]=0;
			return NULL;
		}
		if(name[i]=='.')
		{
			buffer[i]=0;
			return name+i+1;
		}
		buffer[i]=name[i];
	}
	buffer[0]=0;
	return NULL;
}

static inline struct elem_template * _get_elem_from_struct (STRUCT_NODE * node, char * name)
{
	int i;
	struct struct_elem_attr * curr_desc=node->struct_desc;
	for(i=0;i<node->elem_no;i++)
	{
		if(Strcmp(curr_desc[i].name,name)==0)
			return &(node->elem_list[i]);
	}	
	return NULL;
}

void * _get_elem_by_name(void * start_node, char * name)
{
	
	char buffer[65];
	STRUCT_NODE * node = start_node;
	
	struct elem_template *  curr_elem;
	
	if(name[0]=='/')
	{
		node=_get_root_node(node);
		name = name+1;
	}
	do
	{
		name=_get_next_name(name,buffer);
		if(buffer[0]==0)
			return NULL;
		curr_elem=_get_elem_from_struct(node,buffer);
		if(name==NULL)	
			return curr_elem;
		if(curr_elem==NULL)
			return -EINVAL;
		if(!_issubsetelem(curr_elem->elem_desc->type))
			return NULL;
		node=curr_elem->ref;
	}while(node != NULL);
	return NULL;
}

int get_fixed_elemsize(int type)
{
	if(offset_list[type]>0)
		return offset_list[type];
	if(_ispointerelem(type))
		return sizeof(void *);
	return offset_list[type];
	
}


int _elem_get_bin_length(void * value,void * elem,void * addr)
{
	int ret;
	struct elem_template * curr_elem=elem;
	if(_ispointerelem(curr_elem->elem_desc->type))
	{
		if(_isdefineelem(curr_elem->elem_desc->type))
		{
			if(addr==NULL)
				return -EINVAL;
			ret=_elem_get_defvalue(curr_elem,addr);
			if(ret<0)
				return ret;
			if(_isarrayelem(curr_elem->elem_desc->type))
				ret*=curr_elem->elem_desc->size;
		}
		else 
		{ 
			if(_isarrayelem(curr_elem->elem_desc->type))
			{
				ret=curr_elem->elem_desc->size*(int)curr_elem->elem_desc->ref;
				if((ret<0) ||(ret>DIGEST_SIZE*32))
					return -EINVAL;
			}
			else
			{
				if(value==NULL)
					return 0;
				ret=Strnlen(value,DIGEST_SIZE*32);
				if(ret<DIGEST_SIZE*32)
						ret+=1;
			}
		}
	}
	else
	{
		if( (ret=get_fixed_elemsize(curr_elem->elem_desc->type))<0)
			ret=curr_elem->size;
	}
	return ret;
}

struct elem_deal_ops
{
	int (*default_func)(void * addr,void * data,void * elem);
	int (*elem_ops)(void * addr,void * data,void * elem);		
	int (*def_array_init)(void * addr,void * data,void * elem,int def_value);		
	int (*def_array)(void * addr,void * data,void * elem,void * ops,int def_value);		
};


int _elem_to_serial_defarray(void * addr,void * data, void * elem,void * ops,int def_value);

int  _elem_process_func(void * addr,void * data,void * elem,
	struct elem_deal_ops * funcs)
{
	int ret;
	void * elem_addr;
	
	// get this elem's addr

	elem_addr=_elem_get_addr(elem,addr);
	struct elem_template * curr_elem=elem;
	
	// judge if this func is empty

	if(funcs->elem_ops==NULL)
	{
		ret=funcs->default_func(addr,data,elem);
		return ret;

	}
	else
	{
		// judge if this elem is a define elem	
		if(_isdefineelem(curr_elem->elem_desc->type))
		{
			int def_value=_elem_get_defvalue(curr_elem,addr);
			if(def_value<0)
				return def_value;

		
			// define + array elem means we need to repeat 
			// def_value times 	
			if(_isarrayelem(curr_elem->elem_desc->type))
			{
				int addroffset=get_fixed_elemsize(curr_elem->elem_desc->type);
				if(addroffset<0)
					return addroffset;

				if(funcs->def_array_init!=NULL)
				{
					ret=funcs->def_array_init(elem_addr,data,curr_elem,def_value);
					if(ret<0)
						return ret;
				}
				if(funcs->def_array==NULL)
				{
					return _elem_to_serial_defarray(elem_addr,data,curr_elem,funcs->elem_ops,def_value);
				}
				else
				{
					return funcs->def_array(elem_addr,data,curr_elem,funcs->elem_ops,def_value);
				}
			}
				
			else
			{
				// or we should use the normal process func,
				// let the function deal the defvale itself
				curr_elem->index=def_value;
				ret=funcs->elem_ops(elem_addr,data,curr_elem);
				if(ret<0)
					return ret;
					
			}
		}
		else
		{
			// use the normal process func 
			ret=funcs->elem_ops(elem_addr,data,curr_elem);
			if(ret<0)
				return ret;
		}
	}
	return ret;
}

int _elem_to_serial_defarray(void * addr,void * data, void * elem,void * ops,int def_value)
{
	int (*elem_ops)(void * addr,void * data,void * elem)=ops;		
	int offset=0;
	int i;
	int ret;
	struct elem_template * curr_elem=elem;
	int addroffset=get_fixed_elemsize(curr_elem->elem_desc->type);
	if(addroffset<0)
		return addroffset;
		
	
	for(i=0;i<def_value;i++)
	{
		ret=elem_ops(*(void **)addr+addroffset*i,data+offset,curr_elem);
		if(ret<0)
			return ret;
//		offset+=ret-1;
		offset+=ret;
	}
	curr_elem->index=0;
	return offset;
}

int _elem_to_text_defarray(void * addr,void * data, void * elem,void * ops,int def_value)
{
	int (*elem_ops)(void * addr,void * data,void * elem)=ops;		
	int offset=0;
	int i;
	int ret;
	struct elem_template * curr_elem=elem;
	int addroffset=get_fixed_elemsize(curr_elem->elem_desc->type);
	if(addroffset<0)
		return addroffset;
		
	
	for(i=0;i<def_value;i++)
	{
		ret=elem_ops(*(void **)addr+addroffset*i,data+offset,curr_elem);
		if(ret<0)
			return ret;
		offset+=ret-1;
	}
	curr_elem->index=0;
	return offset;
}

int _elem_get_bin_deffunc(void * addr,void * data,void * elem)
{
	int ret;
	void * elem_addr;
	struct elem_template * curr_elem=elem;
	
	// get this elem's addr

	elem_addr=_elem_get_addr(elem,addr);
	// if this func is empty, we use default func
	if((ret=_elem_get_bin_length(*(char **)elem_addr,elem,addr))<0)
		return ret;
	if(_ispointerelem(curr_elem->elem_desc->type))
		Memcpy(data,*(char **)elem_addr,ret);
	else
		Memcpy(data,elem_addr,ret);
	return ret;

}
int  _elem_get_bin_value(void * addr,void * data,void * elem)
{
	struct elem_deal_ops myfuncs;
	ELEM_OPS * elem_ops=_elem_get_ops(elem);
	myfuncs.default_func=&_elem_get_bin_deffunc;
	myfuncs.elem_ops=elem_ops->get_bin_value;
	myfuncs.def_array_init=NULL;
	myfuncs.def_array=NULL;

	return _elem_process_func(addr,data,elem,&myfuncs);
}

int _elem_set_bin_deffunc(void * addr,void * data,void * elem)
{
	int ret;
	void * elem_addr;
	struct elem_template * curr_elem=elem;
	
	// get this elem's addr

	elem_addr=_elem_get_addr(elem,addr);
	// if this func is empty, we use default func
	if((ret=_elem_get_bin_length(data,elem,addr))<0)
		return ret;
	if(_ispointerelem(curr_elem->elem_desc->type))
	{
		int tempret=Palloc0(elem_addr,ret);
		if(tempret<0)
			return tempret;
		Memcpy(*(char **)elem_addr,data,ret);
	}
	else
		Memcpy(elem_addr,data,ret);
	return ret;

}

int _elem_set_bin_defarray(void * addr,void * text,void * elem,int def_value)
{
	int ret;
	struct elem_template * curr_elem=elem;
	int addroffset=get_fixed_elemsize(curr_elem->elem_desc->type);
	if(addroffset<0)
		return addroffset;
	ret=Palloc0(addr,addroffset*def_value);
	return ret;
	
}

int  _elem_set_bin_value(void * addr,void * data,void * elem)
{
	struct elem_deal_ops myfuncs;
	ELEM_OPS * elem_ops=_elem_get_ops(elem);
	myfuncs.default_func=&_elem_set_bin_deffunc;
	myfuncs.elem_ops=elem_ops->set_bin_value;
	myfuncs.def_array_init=_elem_set_bin_defarray;
	myfuncs.def_array=NULL;

	return _elem_process_func(addr,data,elem,&myfuncs);
}

int _elem_clone_deffunc(void * addr, void * clone,void * elem)
{
	int ret;
	void * elem_clone;
	void * elem_src;
	struct elem_template * curr_elem=elem;
	
	// get this elem's addr

	elem_src=_elem_get_addr(elem,addr);
	elem_clone=_elem_get_addr(elem,clone);
	// if this func is empty, we use default func
	if((ret=_elem_get_bin_length(elem_src,elem,addr))<0)
		return ret;
	if(_ispointerelem(curr_elem->elem_desc->type))
	{
		int tempret=Palloc0(elem_clone,ret);
		if(tempret<0)
			return tempret;
		
		Strncpy(*(char **)elem_clone,*(char **)elem_src,DIGEST_SIZE*32);
	}
	else
		Memcpy(elem_clone,elem_src,ret);
	return ret;
}

int _elem_clone_defarray_init(void * addr,void * clone,void * elem,int def_value)
{
	int ret;
	struct elem_template * curr_elem=elem;
	int addroffset=get_fixed_elemsize(curr_elem->elem_desc->type);
	if(addroffset<0)
		return addroffset;
	void * clone_addr= _elem_get_addr(elem,clone);
	ret=Palloc0(clone_addr,addroffset*def_value);
	return ret;
}

int _elem_clone_defarray(void * addr,void * clone, void * elem,void * ops,int def_value)
{
	int (*elem_ops)(void * addr,void * data,void * elem)=ops;		
	int offset=0;
	int i;
	int ret;
	struct elem_template * curr_elem=elem;
	int addroffset=get_fixed_elemsize(curr_elem->elem_desc->type);
	if(addroffset<0)
		return addroffset;
	void * clone_addr= _elem_get_addr(elem,clone);
	
	for(i=0;i<def_value;i++)
	{
		ret=elem_ops(*(void **)addr+addroffset*i,*(void **)clone_addr+addroffset*i,curr_elem);
		if(ret<0)
			return ret;
	}
	return ret;
}


int  _elem_clone_value(void * addr,void * clone,void * elem)
{
	struct elem_deal_ops myfuncs;
	ELEM_OPS * elem_ops=_elem_get_ops(elem);
	myfuncs.default_func=&_elem_clone_deffunc;
	myfuncs.elem_ops=elem_ops->clone_elem;
	myfuncs.def_array_init=_elem_clone_defarray_init;
	myfuncs.def_array=_elem_clone_defarray;

	return _elem_process_func(addr,clone,elem,&myfuncs);
}
	
int _elem_compare_deffunc(void * addr, void * dest,void * elem)
{
	int ret;
	void * elem_dest;
	void * elem_src;
	struct elem_template * curr_elem=elem;
	
	// get this elem's addr

	elem_src=_elem_get_addr(elem,addr);
	elem_dest=_elem_get_addr(elem,dest);
	// if this func is empty, we use default func
	if((ret=_elem_get_bin_length(elem_src,elem,addr))<0)
		return ret;
	if(_ispointerelem(curr_elem->elem_desc->type))
	{
		ret=Strncmp(*(char **)elem_src,*(char **)elem_dest,DIGEST_SIZE*32);
	}
	else
		ret=Memcmp(elem_dest,elem_src,ret);
	if(ret!=0)
		return ret;
	return ret;
}

int _elem_compare_defarray(void * addr,void * clone, void * elem,void * ops,int def_value)
{
	int (*elem_ops)(void * addr,void * data,void * elem)=ops;		
	int offset=0;
	int i;
	int ret;
	struct elem_template * curr_elem=elem;
	int addroffset=get_fixed_elemsize(curr_elem->elem_desc->type);
	if(addroffset<0)
		return addroffset;
	void * clone_addr= _elem_get_addr(elem,clone);
	
	for(i=0;i<def_value;i++)
	{
		ret=elem_ops(*(void **)addr+addroffset*i,*(void **)clone_addr+addroffset*i,curr_elem);
		if(ret<0)
			return ret;
	}
	return ret;
}


int  _elem_compare_value(void * addr,void * clone,void * elem)
{
	struct elem_deal_ops myfuncs;
	ELEM_OPS * elem_ops=_elem_get_ops(elem);
	myfuncs.default_func=&_elem_compare_deffunc;
	myfuncs.elem_ops=elem_ops->comp_elem;
	myfuncs.def_array_init=NULL;
	myfuncs.def_array=_elem_compare_defarray;

	return _elem_process_func(addr,clone,elem,&myfuncs);
}
int _elem_get_text_deffunc(void * addr,void * data,void * elem)
{
	int ret;
	void * elem_addr;
	struct elem_template * curr_elem=elem;
	
	// get this elem's addr

	elem_addr=_elem_get_addr(elem,addr);
	// if this func is empty, we use default func
	if((ret=_elem_get_bin_length(*(char **)elem_addr,elem,addr))<0)
		return ret;
	if(_ispointerelem(curr_elem->elem_desc->type))
		Memcpy(data,*(char **)elem_addr,ret);
	else
		Memcpy(data,elem_addr,ret);
	return ret;

}
int  _elem_get_text_value(void * addr,char * text,void * elem)
{
	struct elem_deal_ops myfuncs;
	ELEM_OPS * elem_ops=_elem_get_ops(elem);
	myfuncs.default_func=&_elem_get_text_deffunc;
	myfuncs.elem_ops=elem_ops->get_text_value;
	myfuncs.def_array_init=NULL;
	myfuncs.def_array=_elem_to_text_defarray;

	return _elem_process_func(addr,text,elem,&myfuncs);
}

int _elem_set_text_deffunc(void * addr,void * data,void * elem)
{
	int ret;
	void * elem_addr;
	struct elem_template * curr_elem=elem;
	char * text=(char *)data;
	
	// get this elem's addr

	elem_addr=_elem_get_addr(elem,addr);
	// if this func is empty, we use default func
	if((ret=_elem_get_bin_length(text,elem,addr))<0)
		return ret;
	if(_ispointerelem(curr_elem->elem_desc->type))
	{
		int tempret=Palloc0(elem_addr,ret);
		if(tempret<0)
			return tempret;
		Memcpy(*(char **)elem_addr,text,ret);
	}
	else
	{
		int str_len=Strlen(text);
		if(str_len>=ret)
		{
			Memcpy(elem_addr,text,ret);
		}
		else
		{
			Memcpy(elem_addr,text,str_len);
			Memset(elem_addr+str_len,0,ret-str_len);	
		}
	}
	return ret;

}

int _elem_set_text_defarray(void * addr,void * text,void * elem,int def_value)
{
	int ret;
	struct elem_template * curr_elem=elem;
	int addroffset=get_fixed_elemsize(curr_elem->elem_desc->type);
	if(addroffset<0)
		return addroffset;
	ret=Palloc0(addr,addroffset*def_value);
	return ret;
	
}

int    _elem_set_text_value(void * addr,char * text,void * elem)
{
	struct elem_deal_ops myfuncs;
	ELEM_OPS * elem_ops=_elem_get_ops(elem);
	myfuncs.default_func=&_elem_set_text_deffunc;
	myfuncs.elem_ops=elem_ops->set_text_value;
	myfuncs.def_array_init=&_elem_set_text_defarray;
	myfuncs.def_array=NULL;

	return _elem_process_func(addr,text,elem,&myfuncs);

}

int _getelemjsontype(int type)
{
	if((type<0)|| (type>CUBE_TYPE_ENDDATA))
		return -EINVAL; 
	if(_isnumelem(type))
		return JSON_ELEM_NUM;
	if(_isboolelem(type))
		return JSON_ELEM_BOOL;
	if(_issubsetelem(type))
		return JSON_ELEM_MAP;
	if(_isarrayelem(type))
		return JSON_ELEM_ARRAY;
	if(_isemptyelem(type))
		return 0;
	return JSON_ELEM_STRING;
		
}

void * _elem_get_ref(void * elem)
{
	struct elem_template *  curr_elem=elem;
	if(curr_elem==NULL)	
		return NULL;
	if(curr_elem->ref!=NULL)
		return curr_elem->ref;
	return curr_elem->elem_desc->ref;
}

int _elem_set_ref(void * elem,void * ref)
{
	struct elem_template *  curr_elem=elem;
	if(curr_elem==NULL)	
		return -EINVAL;
	curr_elem->ref=ref;
	return 0;
}

void * struct_get_ref(void * struct_template,char * name)
{
	struct elem_template *  curr_elem;
	curr_elem=_get_elem_by_name(struct_template,name);
	return _elem_get_ref(curr_elem);
}

int struct_set_ref(void * struct_template,char * name,void * ref)
{
	struct elem_template *  curr_elem;
	curr_elem=_get_elem_by_name(struct_template,name);
	return _elem_set_ref(curr_elem,ref);
}

