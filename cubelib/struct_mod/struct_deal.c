#include "errno.h"
#include "data_type.h"
#include "memfunc.h"
#include "alloc.h"
#include "json.h"
#include "struct_deal.h"
#include "struct_ops.h"
#include "struct_attr.h"

int deep_debug=0;
struct struct_deal_ops
{
	int (*start)(void * addr, void * data,void *elem,void * para);
	int (*testelem)(void * addr, void * data,void *elem,void * para);
	int (*enterstruct)(void * addr,void * data, void * elem,void * para);
	int (*exitstruct)(void * addr,void * data,void * elem,void * para);
	int (*proc_func)(void * addr, void * data, void * elem,void * para);
	int (*finish)(void * addr, void * data,void *elem,void * para);
};

struct struct_namelist * _namevalue_2_namelist(NAME2VALUE * namevalue)
{
	struct struct_namelist * namelist;
	int i;
	int ret;
	const int max_namelist_no=1024;
	namelist=Salloc0(sizeof(*namelist));
	if(namevalue==NULL)
	{
		namelist->elem_no=0;
		namelist->elemlist=NULL;
		return namelist;
	}
	for(namelist->elem_no=0;namelist->elem_no<max_namelist_no;namelist->elem_no++)
	{
		if(namevalue[namelist->elem_no].name==NULL)
			break;	
	}
	if(namelist->elem_no==max_namelist_no)
		return NULL;
	namelist->elemlist=namevalue;
	return namelist;
};

int  _convert_frame_func (void *addr, void * data, void * struct_template,
	struct struct_deal_ops * funcs,void * para)
{
	STRUCT_NODE * root_node=struct_template;
	STRUCT_NODE * curr_node=root_node;
	STRUCT_NODE * temp_node;
	curr_node->temp_var=0;
	struct elem_template * curr_elem=NULL;


	int offset=0;
	ELEM_OPS * elem_ops;
	int def_value;
	int ret;
	if(funcs->start!=NULL)
	{
		ret=funcs->start(addr,data,struct_template,para);
		if(ret<0)
			return ret;
	}


	do{
		if(curr_node->temp_var>curr_node->elem_no)
			break;
		if(curr_node->temp_var == curr_node->elem_no)
		{
			if(curr_node==root_node)
			{
				break;
			}
			temp_node=curr_node;
			curr_node=curr_node->parent;
			curr_elem=&curr_node->elem_list[curr_node->temp_var];
			curr_elem->index++;
			if(funcs->exitstruct!=NULL)
			{
				ret=funcs->exitstruct(addr,data,curr_elem,
					para);
				if(ret<0)
					return ret;
			}
			if(_issubsetelem(curr_elem->elem_desc->type))
			{	
				if(curr_elem->index>=curr_elem->limit)
				{
					curr_node->temp_var++;
					curr_elem->limit=0;
				}
			}
			else
				return -EINVAL;
			continue;
		}

		curr_elem=&curr_node->elem_list[curr_node->temp_var];
		if(funcs->testelem!=NULL)
		{
			if(!funcs->testelem(addr,data,curr_elem,para))
			{
				curr_node->temp_var++;
				continue;
			}
		}
			// throughout the node tree: into the sub_struct
		if(_issubsetelem(curr_elem->elem_desc->type))
		{
			if(funcs->enterstruct!=NULL)
			{
				ret=funcs->enterstruct(addr,data,curr_elem,
					para);
				if(ret<0)
					return ret;
			}
			if(_isarrayelem(curr_elem->elem_desc->type))
			{
				if(curr_elem->limit==0)
				{
					// add func for json string exit
					if(funcs->exitstruct!=NULL)
						ret=funcs->exitstruct(addr,data,curr_elem,para);
					if(ret<0)
						return ret;
                                        //
					curr_node->temp_var++;
					continue;
				}
			}
			curr_node=curr_elem->ref;
			curr_node->temp_var=0;
			continue;
		}
		// get this elem's ops
		elem_ops=_elem_get_ops(curr_elem);
		if(elem_ops==NULL)
			return -EINVAL;

		ret=funcs->proc_func(addr,data,curr_elem,para);
		if(ret<0)
			return ret;
		curr_node->temp_var++;
	}while(1);
	if(funcs->finish==NULL)
		return 0;
	return funcs->finish(addr,data,struct_template,para);
}


struct create_para
{
	int offset;
	int curr_offset;
	STRUCT_NODE * curr_node;
	struct elem_template * parent_elem;
};

int _create_template_start(void * addr,void * data,void * elem, void * para)
{
	int ret;
	int i;
	struct create_para * my_para = para;
	my_para->offset=0;
	my_para->curr_offset=0;
	STRUCT_NODE * root_node=elem;
	STRUCT_NODE * temp_node;
	my_para->parent_elem=NULL;
	my_para->curr_node=root_node;
	// prepare the root node's elem_list
	root_node->elem_no=_count_struct_num(root_node->struct_desc);
	
	root_node->elem_list=Dalloc0(sizeof(struct elem_template)*root_node->elem_no,root_node);
	if(root_node->elem_list==NULL)
		return -ENOMEM;

	// prepare the struct node for SUBSTRUCT elem and ARRAY elem

	for( i=0;i<root_node->elem_no;i++)
	{
		root_node->elem_list[i].elem_desc=&(root_node->struct_desc[i]);
		if(_issubsetelem(root_node->struct_desc[i].type))
		{
			
			temp_node = Dalloc0(sizeof(STRUCT_NODE),root_node); 
			if(temp_node==NULL)
				return -ENOMEM;
			root_node->elem_list[i].ref=temp_node;
			temp_node->struct_desc=root_node->struct_desc[i].ref;
			temp_node->parent=root_node;
		}
	} 
	return 0;
}

int _create_template_enterstruct(void * addr,void * data,void * elem, void * para)
{
	int ret;
	int i;
	struct create_para * my_para = para;
	struct elem_template * curr_elem=elem;
	STRUCT_NODE * curr_node = curr_elem->ref;
	STRUCT_NODE * temp_node;
	// prepare the root node's elem_list
//	if(curr_elem->elem_desc->ref==NULL)
//		return -EINVAL;
//	curr_node->struct_desc=curr_elem->elem_desc->ref;	
	curr_node->elem_no=_count_struct_num(curr_node->struct_desc);
	
	curr_node->elem_list=Dalloc0(sizeof(struct elem_template)*curr_node->elem_no,curr_node);
	if(curr_node->elem_list==NULL)
		return -ENOMEM;

	// prepare the struct node for SUBSTRUCT elem and ARRAY elem

	for( i=0;i<curr_node->elem_no;i++)
	{
		curr_node->elem_list[i].elem_desc=&(curr_node->struct_desc[i]);
		if(_issubsetelem(curr_node->struct_desc[i].type))
		{
			
			temp_node = Dalloc0(sizeof(STRUCT_NODE),curr_node); 
			if(temp_node==NULL)
				return -ENOMEM;
			curr_node->elem_list[i].ref=temp_node;
			temp_node->struct_desc=curr_node->struct_desc[i].ref;
			temp_node->parent=curr_node;
		}
	} 
	curr_elem->offset=my_para->curr_offset;
	if(_isarrayelem(curr_elem->elem_desc->type))
		my_para->curr_offset=0;
	if(curr_elem->father==NULL)
		curr_elem->father=my_para->parent_elem;

	if((!_isarrayelem(curr_elem->elem_desc->type)) && (curr_elem->elem_desc->size>0))
		curr_elem->limit=curr_elem->elem_desc->size;
	else
		curr_elem->limit=1;
	my_para->parent_elem=curr_elem;
	my_para->curr_offset=0;
	return 0;
}

int _create_template_exitstruct(void * addr,void * data,void * elem, void * para)
{
	int ret;
	int i;
	struct create_para * my_para = para;
	struct elem_template * curr_elem=elem;
	if(_isarrayelem(curr_elem->elem_desc->type))
	{
		if(_isdefineelem(curr_elem->elem_desc->type))
		{
			struct elem_template * temp_elem;
			temp_elem= _get_elem_by_name(my_para->curr_node,curr_elem->elem_desc->def);
			if(temp_elem==NULL)
				return -EINVAL;
			if(!_isvalidvalue(temp_elem->elem_desc->type))
				return -EINVAL;
			curr_elem->def=temp_elem;
		}
		curr_elem->size=my_para->curr_offset;
		my_para->curr_offset=curr_elem->offset+sizeof(void *);
		curr_elem->limit=0;
	}
	else
	{
		curr_elem->size=my_para->curr_offset;
		curr_elem->limit=curr_elem->elem_desc->size;
		if(curr_elem->limit==0)
			curr_elem->limit=1;
		my_para->curr_offset=curr_elem->offset+curr_elem->size*curr_elem->limit;
	}
	my_para->parent_elem=curr_elem->father;
	curr_elem->index=curr_elem->limit;
	return 0;
}

int _create_template_proc_func(void * addr,void * data,void * elem, void * para)
{
	ELEM_OPS * elem_ops;
	struct create_para * my_para = para;
	struct elem_template * curr_elem=elem;
	STRUCT_NODE * curr_node=my_para->curr_node;
	if(_isdefineelem(curr_elem->elem_desc->type))
	{
		struct elem_template * temp_elem;
		STRUCT_NODE * father_node;
		if(my_para->parent_elem==NULL)
		{
			father_node=curr_node;
		}
		else
		{
			father_node=my_para->parent_elem->ref;
		}
		
		temp_elem= _get_elem_by_name(father_node,curr_elem->elem_desc->def);
		if(temp_elem==NULL)
			return -EINVAL;
		if(!_isvalidvalue(temp_elem->elem_desc->type))
			return -EINVAL;
		curr_elem->def=temp_elem;
		curr_elem->offset=my_para->curr_offset;
		if(_ispointerelem(curr_elem->elem_desc->type)||
			_isarrayelem(curr_elem->elem_desc->type))
			curr_elem->size=sizeof(void *);
		else
		{
			curr_elem->size=get_fixed_elemsize(curr_elem->elem_desc->type);
			if(curr_elem->size<0)
				return -EINVAL;	
		}
		my_para->curr_offset+=curr_elem->size;
	}
	else
	{
		elem_ops=_elem_get_ops(curr_elem);
		if(elem_ops==NULL)
			return NULL;
		curr_elem->ref=curr_elem->elem_desc->ref;
		curr_elem->offset=my_para->curr_offset;
		if(elem_ops->elem_size==NULL)
		{
			if(_ispointerelem(curr_elem->elem_desc->type)
				||_isarrayelem(curr_elem->elem_desc->type))
			{
				curr_elem->size=sizeof(void *);
			}
			else if((curr_elem->size=get_fixed_elemsize(curr_elem->elem_desc->type))<0)
			{
				curr_elem->size=curr_elem->elem_desc->size;
			}
		}
		else
		{
			curr_elem->size=elem_ops->elem_size(curr_elem->elem_desc);
		}
		my_para->curr_offset+=curr_elem->size;
	}
	if(_isnamelistelem(curr_elem->elem_desc->type))
	{
		struct struct_namelist * namelist;
		namelist=_namevalue_2_namelist(curr_elem->elem_desc->ref);
		curr_elem->ref=namelist;
				
	}

	curr_elem->father=my_para->parent_elem;
	return 0;
}

int _create_template_finish(void * addr,void * data,void * elem, void * para)
{
	struct create_para * my_para = para;
	return 0;
}

void * create_struct_template(struct struct_elem_attr * struct_desc)
{
	int ret;
	struct struct_deal_ops create_template_ops =
	{
		.start=_create_template_start,
		.enterstruct=_create_template_enterstruct,
		.exitstruct=_create_template_exitstruct,
		.proc_func=_create_template_proc_func,
		.finish=_create_template_finish,
	};

	static struct create_para my_para;
	
	STRUCT_NODE * root_node = Calloc0(sizeof(STRUCT_NODE));
	if(root_node==NULL)
		return NULL;
	root_node->struct_desc=struct_desc;
	ret = _convert_frame_func(NULL,NULL,root_node,&create_template_ops,
		&my_para);
	if(ret<0)
		return NULL;
	return root_node;
}

struct clone_template_para
{
	int offset;
	int curr_offset;
	STRUCT_NODE * clone_node;
	STRUCT_NODE * source_node;
	struct elem_template * parent_elem;
};

int _clone_template_start(void * addr,void * data,void * elem, void * para)
{
	int ret;
	int i;
	struct clone_template_para * my_para = para;
	my_para->offset=0;
	my_para->curr_offset=0;
	my_para->parent_elem=NULL;
	STRUCT_NODE * root_node=elem;
	STRUCT_NODE * clone_node=my_para->clone_node;
	STRUCT_NODE * temp_node;
	my_para->source_node=root_node;
	// prepare the root node's elem_list
	clone_node->elem_no=root_node->elem_no;
	clone_node->struct_desc=root_node->struct_desc;

	clone_node->elem_list=Dalloc0(sizeof(struct elem_template)*clone_node->elem_no,clone_node);
	if(clone_node->elem_list==NULL)
		return -ENOMEM;

	// prepare the struct node for SUBSTRUCT elem and ARRAY elem

	for( i=0;i<clone_node->elem_no;i++)
	{
		clone_node->elem_list[i].elem_desc=&(clone_node->struct_desc[i]);
		if(_issubsetelem(clone_node->struct_desc[i].type))
		{
			
			temp_node = Dalloc0(sizeof(STRUCT_NODE),clone_node); 
			if(temp_node==NULL)
				return -ENOMEM;
			clone_node->elem_list[i].ref=temp_node;
			temp_node->struct_desc=clone_node->struct_desc[i].ref;
			temp_node->parent=clone_node;
		}
	} 
	clone_node->offset=my_para->source_node->offset;
	clone_node->size=my_para->source_node->size;
	clone_node->flag=my_para->source_node->flag;
	return 0;
}

int _clone_template_enterstruct(void * addr,void * data,void * elem, void * para)
{
	int ret;
	int i;
	struct clone_template_para * my_para = para;
	struct elem_template * curr_elem=elem;
	int index;
	STRUCT_NODE * clone_node;
	STRUCT_NODE * temp_node;

	index=my_para->source_node->temp_var;
	
	my_para->parent_elem=&my_para->clone_node->elem_list[index];
	clone_node=my_para->parent_elem->ref;
	clone_node->parent=my_para->clone_node;
	my_para->clone_node=clone_node;

	my_para->source_node=curr_elem->ref;

	clone_node->temp_var=0;	

	// prepare the root node's elem_list
	clone_node->elem_no=my_para->source_node->elem_no;
	
	clone_node->elem_list=Dalloc0(sizeof(struct elem_template)*clone_node->elem_no,clone_node);
	if(clone_node->elem_list==NULL)
		return -ENOMEM;

	// prepare the struct node for SUBSTRUCT elem and ARRAY elem

	for( i=0;i<clone_node->elem_no;i++)
	{
		clone_node->elem_list[i].elem_desc=&(clone_node->struct_desc[i]);
		clone_node->elem_list[i].father=my_para->parent_elem;
		if(_issubsetelem(clone_node->struct_desc[i].type))
		{
			
			temp_node = Dalloc0(sizeof(STRUCT_NODE),clone_node); 
			if(temp_node==NULL)
				return -ENOMEM;
			clone_node->elem_list[i].ref=temp_node;
			temp_node->struct_desc=clone_node->struct_desc[i].ref;
			temp_node->parent=clone_node;
		}
	} 
	clone_node->offset=my_para->source_node->offset;
	clone_node->size=my_para->source_node->size;
	clone_node->flag=my_para->source_node->flag;
	return 0;
}

int _clone_template_exitstruct(void * addr,void * data,void * elem, void * para)
{
	int ret;
	int i;
	struct clone_template_para * my_para = para;
	
	my_para->clone_node=my_para->clone_node->parent;
	my_para->source_node=my_para->source_node->parent;
	my_para->parent_elem=my_para->parent_elem->father;

	return 0;
}

int _clone_template_proc_func(void * addr,void * data,void * elem, void * para)
{
	struct clone_template_para * my_para = para;
	struct elem_template * curr_elem = elem;
	int   index = my_para->source_node->temp_var;
	struct elem_template * clone_elem=&my_para->clone_node->elem_list[index];
	STRUCT_NODE * clone_node=my_para->clone_node; 
 
	clone_elem->offset=curr_elem->offset;	
	clone_elem->size=curr_elem->size;	
	clone_elem->flag=curr_elem->flag;	
	clone_elem->index=curr_elem->index;	
	clone_elem->limit=curr_elem->index;	
	clone_elem->ref=curr_elem->ref;

//	clone_elem->father=my_para->parent_elem;

	if(_isdefineelem(clone_elem->elem_desc->type))
	{
		struct elem_template * temp_elem;
		temp_elem= _get_elem_by_name(clone_node,clone_elem->elem_desc->def);
		if(temp_elem==NULL)
			return -EINVAL;
		if(!_isvalidvalue(temp_elem->elem_desc->type))
			return -EINVAL;
		clone_elem->def=temp_elem;
	}
	return 0;
}

int _clone_template_finish(void * addr,void * data,void * elem, void * para)
{
	struct clone_template_para * my_para = para;
	return 0;
}

void * clone_struct_template(void * struct_template)
{
	int ret;
	struct struct_deal_ops clone_template_ops =
	{
		.start=_clone_template_start,
		.enterstruct=_clone_template_enterstruct,
		.exitstruct=_clone_template_exitstruct,
		.proc_func=_clone_template_proc_func,
		.finish=_clone_template_finish,
	};

	static struct clone_template_para my_para;
	
	STRUCT_NODE * root_node = Calloc0(sizeof(STRUCT_NODE));
	if(root_node==NULL)
		return NULL;
	my_para.source_node=(STRUCT_NODE *)struct_template;
	my_para.clone_node=root_node;
	root_node->struct_desc=my_para.source_node->struct_desc;
	ret = _convert_frame_func(NULL,NULL,struct_template,&clone_template_ops,
		&my_para);
	if(ret<0)
		return NULL;
	return root_node;
}
void free_struct_template(void * struct_template)
{
	STRUCT_NODE * root_node=struct_template;
	STRUCT_NODE * curr_node=root_node;
	STRUCT_NODE * temp_node;
	curr_node->temp_var=0;
	struct elem_template * curr_elem;

	do{
		if(curr_node->temp_var == curr_node->elem_no)
		{
			if(curr_node==root_node)
				break;
			temp_node=curr_node;
			curr_node=curr_node->parent;
			if(temp_node->elem_list!=NULL)
				Free(temp_node->elem_list);
			if(temp_node!=root_node)
				Free(temp_node);
			continue;
		}
		curr_elem=&curr_node->elem_list[curr_node->temp_var];
		if(curr_elem->elem_desc->type==CUBE_TYPE_SUBSTRUCT)
		{
			curr_node->temp_var++;
			curr_node=curr_elem->ref;
			curr_node->temp_var=0;
			continue;
		}
		curr_node->temp_var++;
	}while(1);
	Free(root_node);
}

struct elem_template * _get_last_elem(void * struct_template)
{
	STRUCT_NODE * root_node=struct_template;
	struct elem_template * last_elem;
	STRUCT_NODE * temp_node;

	last_elem=&root_node->elem_list[root_node->elem_no-1];
/*
	while(last_elem->elem_desc->type==CUBE_TYPE_SUBSTRUCT)
	{
		last_elem=&root_node->elem_list[root_node->elem_no-1];
	}
*/
	return last_elem;
}

int struct_size(void * struct_template)
{
	struct elem_template * last_elem=_get_last_elem(struct_template);
	ELEM_OPS * elem_ops;
	int total_size;

	if(last_elem->elem_desc->type == CUBE_TYPE_SUBSTRUCT)
	{
		if(last_elem->elem_desc->size==0)
			return last_elem->offset+last_elem->size;
		return last_elem->offset+last_elem->size*last_elem->elem_desc->size;
	}

	elem_ops=_elem_get_ops(last_elem);
	if((elem_ops==NULL)|| (elem_ops->elem_size==NULL))
		return last_elem->offset+last_elem->size;
	return last_elem->offset+elem_ops->elem_size(last_elem);
		
} 

int struct_free_alloc(void * addr,void * struct_template)
{
	STRUCT_NODE * root_node=struct_template;
	STRUCT_NODE * curr_node=root_node;
	STRUCT_NODE * temp_node;
	curr_node->temp_var=0;
	struct elem_template * curr_elem;
	int ret;

	do{
		// throughout the node tree: back
		if(curr_node->temp_var == curr_node->elem_no)
		{
			if(curr_node==root_node)
				break;
			temp_node=curr_node;
			curr_node=curr_node->parent;
			continue;
		}
		curr_elem=&curr_node->elem_list[curr_node->temp_var];
		// throughout the node tree: into the sub_struct
		if(curr_elem->elem_desc->type==CUBE_TYPE_SUBSTRUCT)
		{
			curr_node->temp_var++;
			curr_node=curr_elem->ref;
			curr_node->temp_var=0;
			continue;
		}
		// get this elem's ops
		if(_ispointerelem(curr_elem->elem_desc->type))
		{
			Free(*(void **)(addr+curr_elem->offset)); 
		}
		curr_node->temp_var++;
	}while(1);
	return 0;
}


struct default_para
{
	int offset;
};

int struct_free_enterstruct(void * addr,void * data,void * elem,void * para)
{
	struct default_para  * my_para = para;
	struct elem_template	* curr_elem=elem;

	return 0;
}

int struct_free_exitstruct(void * addr,void * data,void * elem,void * para)
{
	struct default_para  * my_para = para;
	struct elem_template	* curr_elem=elem;
	if(_ispointerelem(curr_elem->elem_desc->type))
	{
		void * elem_addr=_elem_get_addr(elem,addr);
		Free(*(void **)elem_addr);
	}

	return 0;
}

int proc_struct_free(void * addr,void * data,void * elem,void * para)
{
	struct default_para  * my_para = para;
	struct elem_template	* curr_elem=elem;
	if(_ispointerelem(curr_elem->elem_desc->type))
	{
		void * elem_addr=_elem_get_addr(elem,addr);
		Free(*(void **)elem_addr);
	}
	return 0;	
}

int struct_free(void * addr, void * struct_template)
{
	int ret;
	struct struct_deal_ops struct_free_ops =
	{
		.enterstruct=&struct_free_enterstruct,
		.exitstruct=&struct_free_exitstruct,
		.proc_func=&proc_struct_free,
	};	
	ret = _convert_frame_func(addr,NULL,struct_template,&struct_free_ops,		NULL);
	Free(addr);
	return ret;
}

int struct_2_blob_enterstruct(void * addr, void * data, void * elem,void * para)
{
	struct default_para  * my_para = para;
	struct elem_template	* curr_elem=elem;
	if(curr_elem->limit==0)
	{
		curr_elem->index=0;
		if(_isdefineelem(curr_elem->elem_desc->type))
			curr_elem->limit=_elem_get_defvalue(curr_elem,addr);
		else 
		{
			curr_elem->limit=curr_elem->elem_desc->size;
			if(curr_elem->limit==0)
				curr_elem->limit=1;
		}
	}
	return 0;		
}

int struct_2_blob_exitstruct(void * addr, void * data, void * elem,void * para)
{
	struct default_para  * my_para = para;
	struct elem_template	* curr_elem=elem;

	return 0;		
}


int proc_struct_2_blob(void * addr,void * data,void * elem,void * para)
{
	struct default_para  * my_para = para;
	struct elem_template	* curr_elem=elem;
	int ret;
	
	ret = _elem_get_bin_value(addr,data+my_para->offset,elem);
	if(ret>=0)
		my_para->offset+=ret;
	return ret;
} 

int struct_2_blob(void * addr, void * blob, void * struct_template)
{
	int ret;
	struct struct_deal_ops struct_2_blob_ops =
	{
		.enterstruct=&struct_2_blob_enterstruct,
		.exitstruct=&struct_2_blob_exitstruct,
		.proc_func=&proc_struct_2_blob,
	};	
	static struct default_para my_para;
	my_para.offset=0;
	ret = _convert_frame_func(addr,blob,struct_template,&struct_2_blob_ops,		&my_para);
	if(ret<0)
		return ret;
	return my_para.offset;
}

struct part_deal_para
{
	int offset;
	int flag;	
};

int part_deal_test(void * addr,void * data,void * elem,void *para)
{
	struct part_deal_para * my_para=para;
	struct elem_template * curr_elem=elem;
	return curr_elem->flag & my_para->flag;	
}

int struct_2_part_blob(void * addr,void * blob, void * struct_template,int flag)
{
	int ret;
	struct struct_deal_ops struct_2_blob_ops =
	{
		.testelem=part_deal_test,
		.enterstruct=&struct_2_blob_enterstruct,
		.exitstruct=&struct_2_blob_exitstruct,
		.proc_func=&proc_struct_2_blob,
	};	
	static struct part_deal_para my_para;
	my_para.flag=flag;
	my_para.offset=0;
	ret= _convert_frame_func(addr,blob,struct_template,&struct_2_blob_ops,		&my_para);
	if(ret<0)
		return ret;
	return my_para.offset;
}


int blob_2_struct_enterstruct(void * addr, void * data, void * elem,void * para)
{
	
	int ret;
	struct default_para  * my_para = para;
	struct elem_template	* curr_elem=elem;
	void * elem_addr;
	if(curr_elem->limit==0)
	{
		curr_elem->index=0;
		if(_isarrayelem(curr_elem->elem_desc->type))
		{
			if(_isdefineelem(curr_elem->elem_desc->type))
			{
				curr_elem->limit=_elem_get_defvalue(curr_elem,addr);
			}
			else
			{
				curr_elem->limit=curr_elem->elem_desc->size;
				if(curr_elem->limit==0)
					curr_elem->limit=1;
			}
			if(curr_elem->father==NULL)
				elem_addr=addr;
			else
				elem_addr=_elem_get_addr(curr_elem->father,addr);
			elem_addr+=curr_elem->offset;
			
			if(curr_elem->limit>0)
			{
				*(BYTE **)elem_addr=Dalloc0(curr_elem->size*curr_elem->limit,elem_addr);
				if(*(BYTE **)elem_addr==NULL)
					return -ENOMEM;
			}
		}
		else 
		{
			curr_elem->limit=curr_elem->elem_desc->size;
			if(curr_elem->limit==0)
				curr_elem->limit=1;
		}
	}
	return 0;		
}
int proc_blob_2_struct(void * addr,void * data,void * elem,void * para)
{
	struct default_para  * my_para = para;
	struct elem_template	* curr_elem=elem;
	int ret;
	ret = _elem_set_bin_value(addr,data+my_para->offset,elem);
	if(ret>=0)
		my_para->offset+=ret;
	return ret;
} 

int blob_2_struct(void * blob, void * addr, void * struct_template)
{
	int ret;
	struct struct_deal_ops blob_2_struct_ops =
	{
		.enterstruct=&blob_2_struct_enterstruct,
		.proc_func=&proc_blob_2_struct,
	};	
	static struct default_para my_para;
	my_para.offset=0;
	ret = _convert_frame_func(addr,blob,struct_template,&blob_2_struct_ops,&my_para);
	if(ret<0)
		return ret;
	return my_para.offset;
}


int blob_2_part_struct(void * blob,void * addr, void * struct_template,int flag)
{
	int ret;
	struct struct_deal_ops blob_2_struct_ops =
	{
		.testelem=part_deal_test,
		.enterstruct=&blob_2_struct_enterstruct,
		.proc_func=&proc_blob_2_struct,
	};	
	static struct part_deal_para my_para;
	my_para.flag=flag;
	my_para.offset=0;
	ret= _convert_frame_func(addr,blob,struct_template,&blob_2_struct_ops,		&my_para);
	if(ret<0)
		return ret;
	return my_para.offset;
}
// clone struct area
int clone_struct_enterstruct(void * src, void * destr, void * elem,void * para)
{
	int ret;
	struct default_para  * my_para = para;
	struct elem_template	* curr_elem=elem;
	void * elem_addr;
	if(curr_elem->limit==0)
	{
		curr_elem->index=0;
		if(_isarrayelem(curr_elem->elem_desc->type))
		{
			if(_isdefineelem(curr_elem->elem_desc->type))
			{
				curr_elem->limit=_elem_get_defvalue(curr_elem,destr);
			}
			else
			{
				curr_elem->limit=curr_elem->elem_desc->size;
				if(curr_elem->limit==0)
					curr_elem->limit=1;
			}
			if(curr_elem->father==NULL)
				elem_addr=destr;
			else
				elem_addr=_elem_get_addr(curr_elem->father,destr);
			elem_addr+=curr_elem->offset;
			
			if(curr_elem->limit>0)
			{
				*(BYTE **)elem_addr=Dalloc0(curr_elem->size*curr_elem->limit,elem_addr);
				if(*(BYTE **)elem_addr==NULL)
					return -ENOMEM;
			}
		}
		else 
		{
			curr_elem->limit=curr_elem->elem_desc->size;
			if(curr_elem->limit==0)
				curr_elem->limit=1;
		}
	}
	return 0;		
}
int proc_clone_struct(void * src,void * destr,void * elem,void * para)
{
	struct default_para  * my_para = para;
	struct elem_template	* curr_elem=elem;
	int ret;
	ret = _elem_clone_value(src,destr,elem);
	return ret;
} 

int struct_clone(void * src, void * destr, void * struct_template)
{
	int ret;
	void * dest;
	struct struct_deal_ops clone_struct_ops =
	{
		.enterstruct=&clone_struct_enterstruct,
		.proc_func=&proc_clone_struct,
	};	
	static struct default_para my_para;
	my_para.offset=0;

	ret = _convert_frame_func(src,destr,struct_template,&clone_struct_ops,		&my_para);
	if(ret<0)
		return ret;
	return my_para.offset;
}

void * clone_struct(void * addr,void * struct_template)
{
	int ret;
	void * new_struct;
	
	new_struct=Dalloc0(struct_size(struct_template),addr);
	if(new_struct==NULL)
		return NULL;
	ret=struct_clone(addr,new_struct,struct_template);
	if(ret<0)
		return NULL;
	return new_struct;
}

int struct_part_clone(void * src,void *destr, void * struct_template,int flag)
{
	int ret;
	struct struct_deal_ops clone_struct_ops =
	{
		.testelem=part_deal_test,
		.enterstruct=&clone_struct_enterstruct,
		.proc_func=&proc_clone_struct,
	};	
	static struct part_deal_para my_para;
	my_para.flag=flag;
	my_para.offset=0;
	ret= _convert_frame_func(destr,src,struct_template,&clone_struct_ops,		&my_para);
	if(ret<0)
		return ret;
	return my_para.offset;
}

// compare struct area
int compare_struct_enterstruct(void * src, void * destr, void * elem,void * para)
{
	int ret;
	struct default_para  * my_para = para;
	struct elem_template	* curr_elem=elem;
	void * elem_addr;
	int defvalue;
        void * comp_addr;
	if(curr_elem->limit==0)
	{
		curr_elem->index=0;
		if(_isdefineelem(curr_elem->elem_desc->type))
		{
			curr_elem->limit=_elem_get_defvalue(curr_elem,destr);
			defvalue=_elem_get_defvalue(curr_elem,src);
			if(defvalue!=curr_elem->limit)
				return -EINVAL;		
		}
		else 
		{
			curr_elem->limit=curr_elem->elem_desc->size;
			if(curr_elem->limit==0)
				curr_elem->limit=1;
		}
	}
	return 0;		
}

int proc_compare_struct(void * src,void * destr,void * elem,void * para)
{
	struct default_para  * my_para = para;
	struct elem_template	* curr_elem=elem;
	int ret;
	ret = _elem_compare_value(src,destr,elem);
	if(ret!=0)
		return -1;
	return ret;
} 

int struct_comp_elem_value(char * name,void * addr, void * elem_value,void * template)
{
	struct elem_template * elem;
	void * elem_addr;
	BYTE value[DIGEST_SIZE*64];
	int ret;

	ret=struct_read_elem(name,addr,value,template);
	if(ret<0)
		return ret;
	if(Memcmp(value,elem_value,ret)==0)
		return 1;
	return 0;
}

int struct_compare(void * src, void * destr, void * struct_template)
{
	int ret;
	void * dest;
	struct struct_deal_ops clone_struct_ops =
	{
		.enterstruct=&compare_struct_enterstruct,
		.proc_func=&proc_compare_struct,
	};	
	static struct default_para my_para;
	my_para.offset=0;

	ret = _convert_frame_func(src,destr,struct_template,&clone_struct_ops,		&my_para);
	return ret;
}


int struct_part_compare(void * src,void *destr, void * struct_template,int flag)
{
	int ret;
	struct struct_deal_ops clone_struct_ops =
	{
		.testelem=part_deal_test,
		.enterstruct=&compare_struct_enterstruct,
		.proc_func=&proc_compare_struct,
	};	
	static struct part_deal_para my_para;
	my_para.flag=flag;
	my_para.offset=0;

	ret = _convert_frame_func(src,destr,struct_template,&clone_struct_ops,		&my_para);
	return ret;
}
int struct_read_elem(char * name,void * addr, void * elem_data,void * struct_template)
{
	STRUCT_NODE * curr_node=struct_template;
	ELEM_OPS * elem_ops;
	int ret;
	struct elem_template * curr_elem= _get_elem_by_name(curr_node,name);
	if(curr_elem==NULL)
		return -EINVAL;
	
	return _elem_get_bin_value(addr,elem_data,curr_elem);
}

int struct_read_elem_text(char * name,void * addr, char * text,void * struct_template)
{
	STRUCT_NODE * curr_node=struct_template;
	ELEM_OPS * elem_ops;
	struct struct_elem_attr * curr_desc;
	int ret;
	struct elem_template * curr_elem= _get_elem_by_name(curr_node,name);
	if(curr_elem==NULL)
		return -EINVAL;
	return _elem_get_text_value(addr,text,curr_elem);
}

int struct_write_elem(char * name,void * addr, void * elem_data,void * struct_template)
{
	STRUCT_NODE * curr_node=struct_template;
	int ret;
	struct elem_template * curr_elem= _get_elem_by_name(curr_node,name);
	if(curr_elem==NULL)
		return -EINVAL;
	return _elem_set_bin_value(addr,elem_data,curr_elem);
}

int struct_write_elem_text(char * name,void * addr, char * text,void * struct_template)
{
	STRUCT_NODE * curr_node=struct_template;
	struct struct_elem_attr * curr_desc;
	int ret;
	struct elem_template * curr_elem= _get_elem_by_name(curr_node,name);
	if(curr_elem==NULL)
		return -EINVAL;
	return _elem_set_text_value(addr,text,curr_elem);
}
int _struct_get_elem_value(char * name, void * addr,void * struct_template)
{
	char elem_data[64];
	STRUCT_NODE * curr_node=struct_template;
	ELEM_OPS * elem_ops;
	struct struct_elem_attr * curr_desc;
	int ret;
	struct elem_template * curr_elem= _get_elem_by_name(curr_node,name);
	if(curr_elem==NULL)
		return -EINVAL;
	curr_desc=curr_elem->elem_desc;
	elem_ops=_elem_get_ops(curr_elem);
	if(elem_ops==NULL)
		return -EINVAL;
	if(elem_ops->get_int_value==NULL)
	{
		return -EINVAL;
	}
	else
	{
		ret=elem_ops->get_int_value(addr+curr_elem->offset,curr_elem);
	}
	return ret;
}

int _getjsonstr(char * json_str,char * text,int text_len,int json_type)
{
	int str_offset=0;
	int str_len=0;
	int i=0;
	switch(json_type)
	{
		case JSON_ELEM_NUM:
			str_len=Strnlen(text,text_len);
			Memcpy(json_str,text,str_len);
			str_offset=str_len;
			break;
		case JSON_ELEM_STRING:
			str_len=Strnlen(text,text_len);
			*json_str='\"';
			Memcpy(json_str+1,text,str_len);
			*(json_str+str_len+1)='\"';
			str_offset=str_len+2;
			break;
		case JSON_ELEM_ARRAY:
			{
				int isinelem=0;
				int array_no=0;
				*json_str='[';
				str_offset++;
				for(i=0;i<text_len;i++)
				{
					if((text[i]==0) || (text[i]==','))
					{	
						if(isinelem)
						{
							*(json_str+str_offset++)='\"';
							isinelem=0;
						}
					}

					else
					{
						if(!isinelem)
						{
							if(array_no>0)
								*(json_str+str_offset++)=',';
							
							*(json_str+str_offset++)='\"';
							isinelem=1;
							array_no++;
						}
						*(json_str+str_offset++)=*(text+i);
					}
				}
				if((*(text+i-1)!=0)&&(*(text+i-1)!=','))
				{
					if(isinelem)
						*(json_str+str_offset++)='\"';
				}
				*(json_str+str_offset++)=']';
				*(json_str+str_offset)=0;
			}
			break;
	
		default:
			return -EINVAL;

	}
	return str_offset;	
}

int _stripjsonarraychar(char * text,int len)
{
	int i;
	int offset=0;
	int bracket_order=0;
	int quota_order=0;
	enum stat_list
	{
		STRIP_START,
		STRIP_INBRACKET,
		STRIP_INELEM,
		STRIP_FINISH,
	};
	int state=STRIP_START;

	for(i=0;i<len;i++)
	{
		switch(state)
		{
			case STRIP_START:
				if((*(text+i)==' ')||(*(text+i)=='\t'))
				{
					offset++;
					break;
				}
				if(*(text+i)=='[')
				{
					offset++;
					bracket_order++;
					state=STRIP_INBRACKET;
				}
				else
					return -EINVAL;
				break;
			case STRIP_INBRACKET:
				if(Ischarinset(*(text+i)," \t\n\r,")
					||(*(text+i)==','))
				{
					offset++;
					break;
				}
				if(*(text+i)=='\"')
				{
					if(quota_order==0)
					{
						quota_order++;
						state=STRIP_INELEM;
						offset++;
						break;
					}
					return -EINVAL;
				}
				if(*(text+i)==']')
				{
					bracket_order--;
					if(bracket_order==0)
					{
						offset++;
						state=STRIP_FINISH;
						break;
					}
					return -EINVAL;
				}
				if(Ischarinset(*(text+i),":{}"))
					return -EINVAL;
				state=STRIP_INELEM;
				break;
			case 	STRIP_INELEM:
				if(*(text+i)==',')
				{
					*(text+i-offset)=*(text+i);
					if((quota_order==0)&&
						(bracket_order==1))
						state=STRIP_INBRACKET;
				}	
				else if(*(text+i)=='\"')
				{
					if(bracket_order==1)
					{
						if(quota_order==1)
						{
							quota_order--;
							*(text+i-offset)=',';
							state=STRIP_INBRACKET;
						}
						else
							return -EINVAL;
					}
					else
						*(text+i-offset)=*(text+i);
				}
				else if(*(text+i)=='[')
				{
					if(quota_order==0)
						bracket_order++;
					*(text+i-offset)=*(text+i);
				}
				else if(*(text+i)==']')
				{
					if(quota_order==0)
					{
						bracket_order--;
						offset++;
					}
				}
				else
				{
					*(text+i-offset)=*(text+i);
				}
				break;
			default:
				return -EINVAL;
		}
		if(state==STRIP_FINISH)
		{
			*(text+i-offset+1)=0;
			break;
		}
						
	}
	if(state!=STRIP_FINISH)
		return -EINVAL;
	return i-offset;
	
}

int _setvaluefromjson(void * addr,void * node,void * elem)
{
	struct elem_template * curr_elem=elem;
	int ret;
	char buf[DIGEST_SIZE*64];
	switch(json_get_type(node))
	{
		case JSON_ELEM_STRING:
			ret=_elem_set_text_value(addr,json_get_valuestr(node),curr_elem);
			break;		
		case JSON_ELEM_NUM:
		case JSON_ELEM_BOOL:
			{
				long long tempdata;
				ret=json_node_getvalue(node,&tempdata,sizeof(long long));
				if(ret<0)
					return ret;
				ret=_elem_set_bin_value(addr,&tempdata,curr_elem);
			}
			break;		
		case JSON_ELEM_ARRAY:
			{
				ret=json_print_str(node,buf);
				if(ret<0)
					return ret;
				ret=_stripjsonarraychar(buf,ret);	
				if(ret<0)
					return ret;
				ret=_elem_set_text_value(addr,buf,curr_elem);
			}
			
			break;
		case JSON_ELEM_MAP:
		default:
			return -EINVAL;
	}
	return 0;
}

int _print_elem_name(void * data,void * elem,void * para)
{
	struct default_para  * my_para = para;
	char * json_str=data;
	struct elem_template	* curr_elem=elem;
	int ret,text_len;
	// print this elem's name
	text_len=Strnlen(curr_elem->elem_desc->name,64);
	*(json_str+my_para->offset)='\"';
	Memcpy(json_str+my_para->offset+1,curr_elem->elem_desc->name,text_len);
	ret=text_len+1;	
	my_para->offset+=ret;
	*(json_str+my_para->offset)='\"';
	*(json_str+my_para->offset+1)=':';
	ret+=2;
	my_para->offset+=2;
	return ret;
}

struct tojson_para
{
	int offset;
};

int _tojson_start(void * addr, void * data,void *elem,void * para)
{
	struct default_para * my_para=para;
	char * json_str=data;
	
	*json_str='{';
	my_para->offset=1;
	return 1;	
}

int _tojson_enterstruct(void * addr,void * data, void * elem,void * para)
{
	struct default_para * my_para=para;
	char * json_str=data;
	struct elem_template * curr_elem=elem;
	if(curr_elem->limit==0)
	{
		_print_elem_name(data,elem,para);
		curr_elem->index=0;
		if(_isdefineelem(curr_elem->elem_desc->type))
			curr_elem->limit=_elem_get_defvalue(curr_elem,addr);
		else 
		{
			curr_elem->limit=curr_elem->elem_desc->size;
			if(curr_elem->limit==0)
				curr_elem->limit=1;
		}
	}

	if(curr_elem->limit==0)
	{
		*(json_str+my_para->offset)='[';
		my_para->offset++;
	}
	else if(curr_elem->limit==1)
	{
		if(_isdefineelem(curr_elem->elem_desc->type))
		{
			*(json_str+my_para->offset)='[';
			my_para->offset++;
		}	
		*(json_str+my_para->offset)='{';
		my_para->offset++;
	}
	else if(curr_elem->limit>1)
	{
		if(curr_elem->index==0)
		{
			*(json_str+my_para->offset)='[';
			my_para->offset++;
		}
		*(json_str+my_para->offset)='{';
		my_para->offset++;
	}
	return my_para->offset;
}
int _tojson_exitstruct(void * addr,void * data, void * elem,void * para)
{
	struct default_para * my_para=para;
	char * json_str=data;
	struct elem_template * curr_elem=elem;

	if(curr_elem->limit==0)
	{
		*(json_str+my_para->offset)=']';
		my_para->offset++;
	}
	else
	{
		my_para->offset--;
	
		*(json_str+my_para->offset)='}';
		my_para->offset++;
	
		if((curr_elem->limit==1)&&
			_isdefineelem(curr_elem->elem_desc->type))
		{
			*(json_str+my_para->offset)=']';
			my_para->offset++;
		}
		else if((curr_elem->limit>1)&&
			(curr_elem->index>=curr_elem->limit))
		{
			*(json_str+my_para->offset)=']';
			my_para->offset++;
		}
	}
	*(json_str+my_para->offset)=',';
	my_para->offset++;

	return my_para->offset;
}

int _tojson_proc_func(void * addr, void * data, void * elem,void * para)
{
	struct default_para  * my_para = para;
	char * json_str=data;
	struct elem_template	* curr_elem=elem;
	int ret=0,text_len;
	char buf[4096];
	// get this elem's ops
	ELEM_OPS * elem_ops=_elem_get_ops(curr_elem);
	if(elem_ops==NULL)
		return -EINVAL;
	ret=_print_elem_name(data,elem,para);
	text_len=_elem_get_text_value(addr,buf,curr_elem);
	if(text_len<0)
		return text_len;
	text_len = _getjsonstr(json_str+my_para->offset,buf,text_len,_getelemjsontype(curr_elem->elem_desc->type));
	*(json_str+my_para->offset+text_len)=',';
	text_len+=1;
	my_para->offset+=text_len;
	return ret+text_len;
}

int _tojson_finish(void * addr, void * data,void *elem,void * para)
{
	struct default_para * my_para=para;
	char * json_str=data;
	
	if(my_para->offset>1)
		my_para->offset--;
	*(json_str+my_para->offset)='}';
	my_para->offset++;
	*(json_str+my_para->offset)='\0';
	return 1;	
}

int struct_2_json(void * addr, char * json_str, void * struct_template)
{
	int ret;
	struct struct_deal_ops struct_2_json_ops =
	{
		.start=&_tojson_start,
		.enterstruct=&_tojson_enterstruct,
		.exitstruct=&_tojson_exitstruct,
		.proc_func=&_tojson_proc_func,
		.finish=&_tojson_finish,
	};	
	static struct default_para my_para;
	my_para.offset=0;
	ret = _convert_frame_func(addr,json_str,struct_template,&struct_2_json_ops,		&my_para);
	if(ret<0)
		return ret;
	return my_para.offset;
}
int struct_2_part_json(void * addr, char * json_str, void * struct_template,int flag)
{
	int ret;
	struct struct_deal_ops struct_2_part_json_ops =
	{
		.start=&_tojson_start,
		.testelem=part_deal_test,
		.enterstruct=&_tojson_enterstruct,
		.exitstruct=&_tojson_exitstruct,
		.proc_func=&_tojson_proc_func,
		.finish=&_tojson_finish,
	};	
	static struct part_deal_para my_para;
	my_para.offset=0;
	my_para.flag=flag;
	ret = _convert_frame_func(addr,json_str,struct_template,&struct_2_part_json_ops,		&my_para);
	if(ret<0)
		return ret;
	return my_para.offset;
}

struct jsonto_para
{
	void * json_node;
};

int _jsonto_start(void * addr, void * data,void *elem,void * para)
{
	// json node init
	if(json_get_type(data) != JSON_ELEM_MAP)
		return -EINVAL;
	return 1;	
}

int _jsonto_enterstruct(void * addr,void * data, void * elem,void * para)
{
	struct jsonto_para * my_para=para;
	struct elem_template	* curr_elem=elem;
	int ret;
	void * temp_json_node;
	void * elem_addr;

	if(curr_elem->limit==0)
	{
		temp_json_node=json_find_elem(curr_elem->elem_desc->name,my_para->json_node);
		if(temp_json_node==NULL)
			return 0;
		if((json_get_type(temp_json_node) != JSON_ELEM_MAP)
			&&(json_get_type(temp_json_node) != JSON_ELEM_ARRAY))
			return -EINVAL;

		// change the current elem)
		my_para->json_node=temp_json_node;
		curr_elem->index=0;
		if(_isarrayelem(curr_elem->elem_desc->type))
		{
			if(_isdefineelem(curr_elem->elem_desc->type))
			{
				curr_elem->limit=_elem_get_defvalue(curr_elem,addr);
			}
			else
			{
				curr_elem->limit=curr_elem->elem_desc->size;
				if(curr_elem->limit==0)
					curr_elem->limit=1;
			}
			if(curr_elem->limit==0)
			{
			
				if(json_get_type(temp_json_node)==JSON_ELEM_MAP)
				{
					curr_elem->limit=1;
				}
				else
					curr_elem->limit=json_get_elemno(temp_json_node);
				if(curr_elem->limit<0)
					return -EINVAL;
				ret=_elem_set_defvalue(curr_elem,addr,curr_elem->limit);
				if(ret<0)
					return -EINVAL;
				
			}
			if(curr_elem->father==NULL)
				elem_addr=addr;
			else
				elem_addr=_elem_get_addr(curr_elem->father,addr);
			elem_addr+=curr_elem->offset;
			
			if(curr_elem->limit>0)
			{
				*(BYTE **)elem_addr=Dalloc0(curr_elem->size*curr_elem->limit,elem_addr);
				if(*(BYTE **)elem_addr==NULL)
					return -ENOMEM;
			}
		}
		else
		{
			curr_elem->limit=curr_elem->elem_desc->size;
			if(curr_elem->limit==0)
				curr_elem->limit=1;
		}
	}
	

	if(curr_elem->index==0)
	{
		if(json_get_type(my_para->json_node)==JSON_ELEM_ARRAY)
		{
			my_para->json_node=json_get_first_child(my_para->json_node);
			if(my_para->json_node==NULL)
				return -EINVAL;
		}
	}
	else
	{	
		if(curr_elem->limit>1)
		{
			my_para->json_node=json_get_next_child(my_para->json_node);
			if(my_para->json_node==NULL)
				return -EINVAL;

		}
	}
	return 1;
}
int _jsonto_exitstruct(void * addr,void * data, void * elem,void * para)
{
	struct jsonto_para * my_para=para;
	void * temp_json_node=json_get_father(my_para->json_node);
	struct elem_template	* curr_elem=elem;
	if(curr_elem->index>=curr_elem->limit)
	{
		if(json_is_value(my_para->json_node))
			temp_json_node=json_get_father(temp_json_node);
	}
	my_para->json_node=temp_json_node;
	return 1;
}

int _jsonto_proc_func(void * addr, void * data, void * elem,void * para)
{
	struct jsonto_para * my_para=para;
	struct elem_template	* curr_elem=elem;
	int ret,text_len;
	void * temp_json_node=json_find_elem(curr_elem->elem_desc->name,my_para->json_node);
	if(temp_json_node==NULL)
		return 0;

	return _setvaluefromjson(addr,temp_json_node,curr_elem);
}

int json_2_struct(void * root, void * addr, void * struct_template)
{
	int ret;
	struct struct_deal_ops json_2_struct_ops =
	{
		.start=&_jsonto_start,
		.enterstruct=&_jsonto_enterstruct,
		.exitstruct=&_jsonto_exitstruct,
		.proc_func=&_jsonto_proc_func,
	};	
	static struct jsonto_para my_para;
	my_para.json_node=root;
	ret = _convert_frame_func(addr,root,struct_template,&json_2_struct_ops,		&my_para);
	if(ret<0)
		return ret;
	return 0;
}

struct part_jsonto_para
{
	void * json_node;
	int flag;
};
int part_json_test(void * addr,void * data,void * elem,void *para)
{
	struct part_jsonto_para * my_para=para;
	struct elem_template * curr_elem=elem;
	return curr_elem->flag & my_para->flag;	
}

int json_2_part_struct(void * root, void * addr, void * struct_template,int flag)
{
	int ret;
	struct struct_deal_ops json_2_part_struct_ops =
	{
		.start=&_jsonto_start,
		.testelem=part_json_test,
		.enterstruct=&_jsonto_enterstruct,
		.exitstruct=&_jsonto_exitstruct,
		.proc_func=&_jsonto_proc_func,
	};	
	static struct part_jsonto_para my_para;
	my_para.json_node=root;
	my_para.flag=flag;
	ret = _convert_frame_func(addr,root,struct_template,&json_2_part_struct_ops,		&my_para);
	if(ret<0)
		return ret;
	return 0;
}

struct json_marked_para
{
	void * father_node;
	int flag;
};

int _json_marked_start(void * addr, void * data,void *elem,void * para)
{
	struct json_marked_para * my_para=para;
	// json node init
	if(json_get_type(data) != JSON_ELEM_MAP)
		return -EINVAL;
	if(data==NULL)
		return -EINVAL;	
	my_para->father_node=data;
	return 1;	
}

int _json_marked_test(void * addr,void * data,void * elem,void *para)
{
	struct json_marked_para * my_para=para;
	struct elem_template * curr_elem=elem;
	void * temp_node=json_find_elem(curr_elem->elem_desc->name,my_para->father_node);
	if(temp_node==NULL)
		return 0;		
	return my_para->flag;	
}


int _json_marked_enterstruct(void * addr,void * data, void * elem,void * para)
{
	struct json_marked_para * my_para=para;
	struct elem_template	* curr_elem=elem;
	int ret;
	void * temp_json_node;
	
	temp_json_node=json_find_elem(curr_elem->elem_desc->name,my_para->father_node);
	if(temp_json_node!=NULL)
	{
		my_para->father_node=temp_json_node;
	}
	return 1;
}
int _json_marked_exitstruct(void * addr,void * data, void * elem,void * para)
{
	struct json_marked_para * my_para=para;
	struct elem_template	* curr_elem=elem;
	int ret;
	
	if(my_para->father_node==NULL)
		return -EINVAL;
	my_para->father_node=json_get_father(my_para->father_node);
	return 1;
}

int _json_marked_procfunc(void * addr,void * data, void * elem,void * para)
{
	struct json_marked_para * my_para=para;
	struct elem_template	* curr_elem=elem;
	int ret;
	
	if(my_para->father_node==NULL)
		return -EINVAL;
	curr_elem->flag |= my_para->flag;
	if(_issubsetelem(curr_elem->elem_desc->type))
	{
		ret=struct_set_allflag(curr_elem->ref,my_para->flag);
	}
	if(curr_elem->father!=NULL)
	{
		struct elem_template * father_elem=curr_elem->father;
		while(father_elem!=NULL)
		{
			father_elem->flag |= my_para->flag;
			father_elem=father_elem->father;
		};
	}
	return 1;
}

int json_marked_struct(void * root, void * struct_template,int flag)
{
	int ret;
	struct struct_deal_ops json_marked_struct_ops =
	{
		.start=&_json_marked_start,
		.testelem=_json_marked_test,
		.enterstruct=&_json_marked_enterstruct,
		.exitstruct=&_json_marked_exitstruct,
		.proc_func=&_json_marked_procfunc,
	};	
	static struct json_marked_para my_para;
	my_para.father_node=struct_template;
	my_para.flag=flag;
	ret = _convert_frame_func(NULL,root,struct_template,&json_marked_struct_ops,&my_para);
	if(ret<0)
		return ret;
	return 0;
}
int struct_set_allflag(void * struct_template,int flag)
{
	STRUCT_NODE * root_node=struct_template;
	STRUCT_NODE * curr_node=root_node;
	STRUCT_NODE * temp_node;
	curr_node->temp_var=0;
	struct elem_template * curr_elem;

	do{
		if(curr_node->temp_var == curr_node->elem_no)
		{
			if(curr_node==root_node)
				break;
			temp_node=curr_node;
			curr_node=curr_node->parent;
			continue;
		}
		curr_elem=&curr_node->elem_list[curr_node->temp_var];
		if(_issubsetelem(curr_elem->elem_desc->type))
		{
			curr_node->temp_var++;
			curr_node=curr_elem->ref;
			curr_node->temp_var=0;
			curr_elem->flag |=flag;
			continue;
		}
		else
		{
			curr_elem->flag |=flag;
		}
		curr_node->temp_var++;
		
	}while(1);
	root_node->flag |=flag;
	return 0;
}

int struct_clear_allflag(void * struct_template,int flag)
{
	STRUCT_NODE * root_node=struct_template;
	STRUCT_NODE * curr_node=root_node;
	STRUCT_NODE * temp_node;
	curr_node->temp_var=0;
	struct elem_template * curr_elem;

	do{
		if(curr_node->temp_var == curr_node->elem_no)
		{
			if(curr_node==root_node)
				break;
			temp_node=curr_node;
			curr_node=curr_node->parent;
			continue;
		}
		curr_elem=&curr_node->elem_list[curr_node->temp_var];
		if(_issubsetelem(curr_elem->elem_desc->type))
		{
			curr_node->temp_var++;
			curr_node=curr_elem->ref;
			curr_node->temp_var=0;
			curr_node->flag &=~flag;
			continue;
		}
		else
		{
			curr_elem->flag &=~flag;
		}
		curr_node->temp_var++;
		
	}while(1);
	root_node->flag !=flag;
	return 0;
}

int struct_set_flag(void * struct_template,int flag, char * namelist)
{
	STRUCT_NODE * root_node=struct_template;
	STRUCT_NODE * father_node=NULL;
	struct elem_template * curr_elem;
	struct elem_template * father_elem;
	int ret;
	char name[DIGEST_SIZE+1];
	int offset=0;
	int i;
	int count=0;

	while((i=Getfiledfromstr(name,namelist+offset,',',DIGEST_SIZE+1))>0)
	{
		offset+=i;
		curr_elem = _get_elem_by_name(root_node,name);
		if(curr_elem==NULL)
			return -EINVAL;
		curr_elem->flag |= flag;
		if(_issubsetelem(curr_elem->elem_desc->type))
		{
			ret=struct_set_allflag(curr_elem->ref,flag);
		}
		count++;
		if(namelist[offset]==',')
			offset++;
		if(curr_elem->father==NULL)
			father_node=root_node;
		else
		{
			father_elem=curr_elem->father;
			father_node=father_elem->ref;
			while(father_elem!=NULL)
			{
				father_elem->flag |= flag;
				father_elem=father_elem->father;
			};
		}
	}
	return count;
}

int struct_clear_flag(void * struct_template,int flag, char * namelist)
{
	STRUCT_NODE * root_node=struct_template;
	STRUCT_NODE * father_node;
	STRUCT_NODE * temp_node;
	struct elem_template * curr_elem;
	struct elem_template * temp_elem;
	int ret;
	char name[DIGEST_SIZE+1];
	int offset=0;
	int i;
	int count=0;

	while((i=Getfiledfromstr(name,namelist+offset,',',DIGEST_SIZE+1))>0)
	{
		offset+=i;
		curr_elem = _get_elem_by_name(root_node,name);
		if(curr_elem==NULL)
			return -EINVAL;
		if(_issubsetelem(curr_elem->elem_desc->type))
		{
			ret=struct_set_allflag(curr_elem->ref,flag);
		}
		else
		{
			curr_elem->flag &= ~flag;
		}
		count++;
		if(namelist[offset]==',')
			offset++;
		if(curr_elem->father==NULL)
			father_node=root_node;
		else
			father_node=curr_elem->father->ref;
		do {
			for(i=0;i<father_node->elem_no;i++)
			{	
				temp_elem=&father_node->elem_list[i];
				if(_issubsetelem(temp_elem->elem_desc->type))
					
				{
					temp_node=temp_elem->ref;
					if(temp_node->flag &flag)
						break;
				}
				else
				{
					if(temp_elem->flag &flag)
						break;

				}
			}
			if(i<father_node->elem_no)
				break;
			father_node->flag &=~flag;
			if(father_node==root_node)
				break;
			father_node=father_node->parent;
		}while(1);
	}
	return count;
	
}

int struct_get_flag(void * struct_template,char * name)
{
	STRUCT_NODE * root_node=struct_template;
	struct elem_template * curr_elem;
	int ret;

	curr_elem = _get_elem_by_name(root_node,name);
	if(curr_elem==NULL)
		return -1;
	return curr_elem->flag;
}
