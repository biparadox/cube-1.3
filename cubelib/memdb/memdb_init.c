
#include "../include/data_type.h"
#include"../include/errno.h"
#include "../include/list.h"
#include "../include/attrlist.h"
#include "../include/string.h"
#include "../include/alloc.h"
#include "../include/json.h"
#include "../include/struct_deal.h"
#include "../include/basefunc.h"
#include "../include/memdb.h"
#include "../struct_deal/struct_ops.h"
#include "../include/crypto_func.h"

#include "valuelist.h"
#include "type_desc.h"
#include "memdb_internal.h"


extern ELEM_OPS enumtype_convert_ops;
extern ELEM_OPS recordtype_convert_ops;
extern ELEM_OPS subtype_convert_ops;

struct memdb_desc ** static_db_list;
struct memdb_desc * dynamic_db_list;
struct struct_namelist *elemenumlist;
struct struct_namelist *typeenumlist;

static struct InitElemInfo_struct MemdbElemInfo[] =
{
	{CUBE_TYPE_ELEMTYPE,&enumtype_convert_ops,0,sizeof(int)},
	{CUBE_TYPE_RECORDTYPE,&recordtype_convert_ops,ELEM_ATTR_VALUE,sizeof(int)},
	{CUBE_TYPE_RECORDSUBTYPE,&subtype_convert_ops,ELEM_ATTR_DEFINE,sizeof(int)},
	{CUBE_TYPE_ENDDATA,NULL,ELEM_ATTR_EMPTY,0},

};

/*
struct struct_elem_attr struct_index_desc[] =
{
	{"head",CUBE_TYPE_SUBSTRUCT,1,&uuid_head_desc,NULL},
	{"flag",CUBE_TYPE_INT,sizeof(int),NULL,NULL},
	{"uuid",CUBE_TYPE_UUID,DIGEST_SIZE,NULL,NULL},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};

*/

void * _alloc_dynamic_db(int type,int subtype);
int _namelist_tail_func(void * memdb,void * record)
{
	DB_RECORD * db_record=record;
	NAME2VALUE * namevalue;
	struct struct_namelist * namelist=db_record->record;
	int i;
	int ret;
	ret=Galloc0(&namevalue,sizeof(NAME2VALUE)*(namelist->elem_no+1));
	Memcpy(namevalue,namelist->elemlist,sizeof(NAME2VALUE)*namelist->elem_no);	

	db_record->tail=namevalue;
	return 0;
}

static inline int _get_namelist_no(void * list)
{
	NAME2VALUE * namelist=list;
	int i=0;

	if(list==NULL)
		return -EINVAL;
	while(namelist[i].name!=NULL)
		i++;
	return i;
}

static inline int _get_value_namelist(char * name,void * list)
{
	struct struct_namelist * namelist=list;
	int i;
	for(i=0;i<namelist->elem_no;i++)
	{
		if(!Strcmp(namelist->elemlist[i].name,name))
		{
			return namelist->elemlist[i].value;
		}
	}
	return 0;
}

void * _clone_namelist(void * list1)
{
	struct struct_namelist * namelist1 = list1;
	struct struct_namelist * newnamelist;
	int ret;
	int i;

	int elem_no = namelist1->elem_no;

	ret=Galloc0(&newnamelist,sizeof(struct struct_namelist));
	if(ret<0)
		return NULL;
	ret = Galloc0(&newnamelist->elemlist,sizeof(NAME2VALUE)*elem_no);
	if(ret<0)
		return NULL;
	newnamelist->elem_no=elem_no;
	for(i=0;i<elem_no;i++)
	{
		newnamelist->elemlist[i].value=namelist1->elemlist[i].value;
		newnamelist->elemlist[i].name=namelist1->elemlist[i].name;
	}
	return newnamelist;
}

void * _merge_namelist(void * list1, void * list2)
{
	struct struct_namelist * namelist1 = list1;
	struct struct_namelist * namelist2 = list2;
	struct struct_namelist * newnamelist;
	int ret;
	int elem_no;
	int i,j,k;
	NAME2VALUE  * buf;

	elem_no = namelist1->elem_no+namelist2->elem_no;

	ret = Galloc0(&buf,sizeof(NAME2VALUE)*elem_no);
	if(ret<0)
		return NULL;
	j=0;
	k=0;
	for(i=0;i<elem_no;i++)
	{
		if(j==namelist1->elem_no)
		{
			buf[i].value=namelist2->elemlist[k].value;
			buf[i].name=namelist2->elemlist[k++].name;
			continue;
		}
		if(k==namelist2->elem_no)
		{
			buf[i].value=namelist1->elemlist[j].value;
			buf[i].name=namelist1->elemlist[j++].name;
			continue;
		}

		if(namelist1->elemlist[j].value<namelist2->elemlist[k].value)
		{
			buf[i].value=namelist1->elemlist[j].value;
			buf[i].name=namelist1->elemlist[j++].name;
		}
		else if(namelist1->elemlist[j].value>namelist2->elemlist[k].value)
		{
			buf[i].value=namelist2->elemlist[k].value;
			buf[i].name=namelist2->elemlist[k++].name;
		}
		else
		{
			buf[i].value=namelist1->elemlist[j].value;
			buf[i].name=namelist1->elemlist[j++].name;
			elem_no--;
			k++;
		}
	}

	
	ret=Galloc0(&newnamelist,sizeof(struct struct_namelist));
	if(ret<0)
		return NULL;
	ret = Galloc0(&newnamelist->elemlist,sizeof(NAME2VALUE)*elem_no);
	if(ret<0)
		return NULL;
	newnamelist->elem_no=elem_no;
	for(i=0;i<elem_no;i++)
	{
		newnamelist->elemlist[i].value=buf[i].value;
		newnamelist->elemlist[i].name=buf[i].name;
	}

	Free0(buf);
	return newnamelist;
}

int _memdb_record_find_name(void * record,char * name)
{
	int ret;
	int namelist_no;
	int i;
	DB_RECORD * record_db=(DB_RECORD *)record;
	if(Strncmp(record_db->head.name,name,DIGEST_SIZE)==0)
		return 1;
	if(record_db->names==NULL)
		return 0;
	namelist_no=record_db->name_no;
	if(record_db->head.name[0]!=0)
		namelist_no--;
	if(namelist_no<1)
		return 0;
	for(i=0;i<namelist_no;i++)
	{
		if(Strncmp(record_db->names[i],name,DIGEST_SIZE)==0)
			return i+2;
	}
	return 0;	
}

int _memdb_record_add_name(void * record,char * name)
{
	int ret;
	int len;
	int namelist_no;
	char * new_name;
	char ** new_namearray;
	DB_RECORD * record_db=(DB_RECORD *)record;

	// judge if this name is in the list;
	if(_memdb_record_find_name(record,name)>0)
		return 0;

	len=Strnlen(name,DIGEST_SIZE);
	if(len<DIGEST_SIZE)
		len++;

	ret=Galloc0(&new_name,len);
	if(ret<0)
		return -ENOMEM;
	Strncpy(new_name,name,DIGEST_SIZE);
	
	if(record_db->names==NULL)
	{
		ret=Galloc0(&(record_db->names),sizeof(char *));
		if(ret<0)
			return -ENOMEM;
		record_db->names[0]=new_name;
		record_db->name_no++;
		return 2;
	}

	record_db->name_no++;
	namelist_no=record_db->name_no;
	if(record_db->head.name[0]!=0)
		namelist_no--;
	namelist_no++;
		
	ret=Galloc0(&(new_namearray),sizeof(char *)*namelist_no);
	if(ret<0)
		return -EINVAL;
	Memcpy(new_namearray,record_db->names,sizeof(char *)*(namelist_no-1));
	new_namearray[namelist_no-1]=new_name;

	Free0(record_db->names);
	record_db->names=new_namearray;	

	return namelist_no+2;	
}

int _memdb_record_remove_name(void * record,char * name)
{
	int ret;
	int len;
	int namelist_no;
	char * new_name;
	char ** new_namearray;
	int namesite_no;
	DB_RECORD * record_db=(DB_RECORD *)record;

	namesite_no=_memdb_record_find_name(record,name);
	if(namesite_no<0)
		return namesite_no;
	// no name in this record
	if(namesite_no==0)
	{
		return 0;
	}
	namelist_no=record_db->name_no;
	if(record_db->head.name[0]!=0)
		namelist_no--;
	// name is in the head
	if(namesite_no==1)
	{
		// there is no name in the names list, 
		// we should remove this record
		if(record_db->name_no==1)
		{
			memdb_remove(record,record_db->head.type,record_db->head.subtype);
			memdb_free_record(record);	
			return 1;
		}
		// or we should remove the first name in names list to head
		Strncpy(record_db->head.name,record_db->names[0],DIGEST_SIZE);
	}
	
	ret=Galloc0(&(new_namearray),sizeof(char *)*(namelist_no-1));
	if(ret<0)
		return -EINVAL;
	if(namelist_no<namesite_no)
		return -EINVAL;
	Free0(record_db->names[namesite_no-1]);
	Memcpy(new_namearray,record_db->names,sizeof(char *)*(namesite_no-1));
	Memcpy(new_namearray+namesite_no-1,record_db->names+namesite_no,
		sizeof(char *)*(namelist_no-namesite_no));
	Free0(record_db->names);
	record_db->names=new_namearray;	

	return namesite_no;		
}


void * _struct_octet_to_attr(void * octet_array,int elem_no)
{
	int ret,i;
	struct elem_attr_octet * struct_desc_octet=octet_array; 	
	struct elem_attr_octet * elem_desc_octet; 
	struct struct_elem_attr * struct_desc;
	struct struct_elem_attr * elem_desc;
	DB_RECORD * child_desc_record;
	DB_RECORD * ref_namelist;
	BYTE ref_comp[DIGEST_SIZE];

	Memset(ref_comp,0,DIGEST_SIZE);

	ret=Galloc0(&struct_desc,sizeof(struct struct_elem_attr)*(elem_no+1));
	if(ret<0)
		return NULL;
	for(i=0;i<elem_no;i++)
	{
		elem_desc_octet=struct_desc_octet+i;
		elem_desc=struct_desc+i;

		// duplicate all the value except ref		

		elem_desc->name=elem_desc_octet->name;
		elem_desc->type=elem_desc_octet->type;
		elem_desc->size=elem_desc_octet->size;
		if(elem_desc_octet->def[0]==0)
			elem_desc->def=NULL;
		else
			elem_desc->def=elem_desc_octet->def;
		
		// if their is no valid ref
		if(_issubsetelem(elem_desc_octet->type))
		{
			if(Memcmp(ref_comp,elem_desc_octet->ref,DIGEST_SIZE/2)==0)
			{
				// invalid ref uuid, we should find ref by ref_name
				child_desc_record=memdb_find_byname(elem_desc_octet->ref,DB_STRUCT_DESC,0);
			}
			else
			{
				child_desc_record=memdb_find(elem_desc_octet->ref,DB_STRUCT_DESC,0);
			}
			if(child_desc_record==NULL)
				return NULL;
			elem_desc->ref=child_desc_record->tail;	
			if(elem_desc->ref==NULL)
				return NULL;
		}

		else if(_isnamelistelem(elem_desc_octet->type))
		{

			
			if(Memcmp(ref_comp,elem_desc_octet->ref,DIGEST_SIZE/2)==0)
			{
				// invalid ref uuid, we should find ref by ref_name
				ref_namelist=memdb_find_byname(elem_desc_octet->ref_name,DB_NAMELIST,0);
			}
			else
			{
				ref_namelist=memdb_find(elem_desc_octet->ref,DB_NAMELIST,0);
			}
			if(ref_namelist==NULL)
				return NULL;
			elem_desc->ref=ref_namelist->tail;	
			if(elem_desc->ref==NULL)
				return NULL;
		}
	}
	return struct_desc; 
} 

int _struct_desc_tail_func(void * memdb,void * record)
{
	DB_RECORD * db_record=record;
	struct struct_desc_record * struct_desc_octet=db_record->record;

	db_record->tail=_struct_octet_to_attr(struct_desc_octet->elem_desc,struct_desc_octet->elem_no);
	if(db_record->tail==NULL)
		return -EINVAL;
	return 0;
}

int _typelist_tail_func(void * memdb,void * record)
{
	DB_RECORD * db_record=record;
	struct struct_typelist * typelist=db_record->record;
	DB_RECORD * temp_record;
	temp_record=memdb_find(typelist->uuid,DB_NAMELIST,0);
	if(temp_record==NULL)
		return NULL;

	db_record->tail=temp_record->record;
	if(db_record->tail==NULL)
		return -EINVAL;
	return 0;
}

int _subtypelist_tail_func(void * memdb,void * record)
{
	DB_RECORD * db_record=record;
	struct struct_subtypelist * subtypelist=db_record->record;
	DB_RECORD * temp_record;
	temp_record=memdb_find(subtypelist->uuid,DB_NAMELIST,0);
	if(temp_record==NULL)
		return NULL;

	db_record->tail=temp_record->record;
	if(db_record->tail==NULL)
		return -EINVAL;
	return 0;
}

int _recordtype_tail_func(void * memdb,void * record)
{
	int ret;
	DB_RECORD * db_record=record;
	struct struct_recordtype * recordtype = db_record->record;
	DB_RECORD * temp_record;
	void * struct_template;
	int i;
	char * index_elems=NULL;
	temp_record=memdb_find(recordtype->uuid,DB_STRUCT_DESC,0);
	if(temp_record==NULL)
		return -EINVAL;

	db_record->tail=create_struct_template(temp_record->tail);
	if(db_record->tail==NULL)
		return -EINVAL;


	for(i=0;i<recordtype->flag_no;i++)
	{
		if(recordtype->index[i].flag==CUBE_ELEM_FLAG_INDEX)
		{
			index_elems=recordtype->index[i].elemlist;		
			break;
		}
	}
	if(index_elems==NULL)
	{
		ret=struct_set_allflag(db_record->tail,CUBE_ELEM_FLAG_INDEX);
		if(ret<0)
			return ret;
	}
	

	for(i=0;i<recordtype->flag_no;i++)
	{
		if(recordtype->index[i].flag!=CUBE_ELEM_FLAG_INDEX)
		{
			struct_set_flag(db_record->tail,recordtype->index[i].flag,
				recordtype->index[i].elemlist);		
		}
	}
	return 0;
}

int memdb_register_db(int type,int subtype,void * struct_desc,void * tail_func,char * index_elems)
{
	struct memdb_desc * memdb;
	int ret;
	if(type<0)
		return -EINVAL;

	memdb=memdb_get_dblist(type,subtype);
	if(memdb!=NULL)
		return -EINVAL;

	if(type<DB_DTYPE_START)
	{
		ret=Galloc0(&static_db_list[type],sizeof(struct memdb_desc));
		if(ret<0)
			return -EINVAL;
		memdb=static_db_list[type];
	}
	else if(type>=DB_DTYPE_START)
	{
		return -EINVAL;
	}
	memdb->record_db=init_hash_list(8,type,subtype);
	memdb->struct_template=create_struct_template(struct_desc);
	memdb->type=type;
	memdb->subtype=subtype;
	memdb->tail_func=tail_func;
	if(index_elems!=NULL)
	{
		ret=struct_set_flag(memdb->struct_template,CUBE_ELEM_FLAG_INDEX,index_elems);
		if(ret<0)
			return ret;
	}
	else
	{
		ret=struct_set_allflag(memdb->struct_template,CUBE_ELEM_FLAG_INDEX);
		if(ret<0)
			return ret;
	}
	return 0;
}


void * memdb_get_recordtype(int type, int subtype)
{
	DB_RECORD * db_record;
	struct struct_recordtype * recordtype;
	
	db_record=memdb_get_first(DB_RECORDTYPE,0);
	
	while(db_record!=NULL)
	{
		recordtype=db_record->record;
		if((recordtype->type==type) &&
			(recordtype->subtype==subtype))
		{
			return db_record;		
		}
		db_record=memdb_get_next(DB_RECORDTYPE,0);	
	}
	return NULL;
}

int memdb_register_dynamicdb(int type,int subtype)
{
	struct memdb_desc * memdb;
	int ret;
	DB_RECORD * db_record;
	struct struct_recordtype * record_define;
	
	if(type<=DB_DTYPE_START)
		return -EINVAL;

	memdb=memdb_get_dblist(type,subtype);
	if(memdb!=NULL)
		return -EINVAL;

	db_record=memdb_get_recordtype(type,subtype);
	if(db_record==NULL)
		return -EINVAL;

	record_define=db_record->record;

	memdb=_alloc_dynamic_db(type,subtype);
	if(memdb==NULL)
		return -EINVAL;
	Memcpy(memdb->uuid,record_define->uuid,DIGEST_SIZE);
	ret=hashlist_add_elem(dynamic_db_list,memdb);
	if(ret<0)
		return ret;
	memdb->struct_template=db_record->tail;
	memdb->tail_func=NULL;
	return 0;
}
		
int memdb_get_elem_type(void * elem)
{
	UUID_HEAD * head=elem;
	if(head==NULL)
		return -EINVAL;
	return head->type;
}

int memdb_get_elem_subtype(void * elem)
{
	UUID_HEAD * head=elem;
	if(head==NULL)
		return -EINVAL;
	return head->subtype;
}

int memdb_is_dynamic(int type)
{
	if(type>=DB_DTYPE_START)
		return 1;
	return 0;
}

void * memdb_get_first(int type,int subtype)
{
	int ret;
	struct memdb_desc * db_list;
	db_list=memdb_get_dblist(type,subtype);
	if(db_list==NULL)
		return NULL;
	return hashlist_get_first(db_list->record_db);
}

void * memdb_get_next(int type,int subtype)
{
	int ret;
	struct memdb_desc * db_list;
	db_list=memdb_get_dblist(type,subtype);
	if(db_list==NULL)
		return NULL;
	return hashlist_get_next(db_list->record_db);
}



void *  _get_dynamic_db_bytype(int type,int subtype)
{
	struct memdb_desc * memdb;
	memdb=hashlist_get_first(dynamic_db_list);
	while(memdb!=NULL)
	{
		if(memdb->type == type)
		{
			if(memdb->subtype == subtype)
				return memdb;
		}
		memdb=hashlist_get_next(dynamic_db_list);
	}	 
	return NULL;
}

void * memdb_get_dblist(int type, int subtype)
{
	void * db_list;
	if(type<0) 
		return NULL;
	if(type< DB_DTYPE_START)
		db_list=static_db_list[type];
	else if(type == DB_DTYPE_START)
		db_list=dynamic_db_list;
	else if(type >DB_DTYPE_START)
		db_list=_get_dynamic_db_bytype(type,subtype);
	else
		return NULL;
	return db_list;
}


void * _alloc_dynamic_db(int type,int subtype)
{
	int ret;
	struct memdb_desc * memdb;
	void * db_list=init_hash_list(8,type,subtype);
	if(db_list==NULL)
		return NULL;
	ret=Galloc0(&memdb,sizeof(DB_DESC));
	if(ret<0)
		return NULL;
	memdb->type=type;
	memdb->subtype=subtype;
	memdb->record_db=db_list;
	return memdb;


}
/*
{
	int ret;
	DB_DESC * memdb;
	struct struct_recordtype * record_type=record_def; 
	void * db_list=init_hash_list(8,type,subtype);
	if(db_list==NULL)
		return NULL;

	ret=Galloc0(&memdb,sizeof(DB_DESC));
	if(ret<0)
		return NULL;
	Memcpy(memdb->head.uuid,record_type->head.uuid,DIGEST_SIZE);
	Strncpy(memdb->head.name,record_type->head.name,DIGEST_SIZE);
	memdb->head.type=record_type->type;
	memdb->head.subtype=record_type->subtype;
	hashlist_set_desc(db_list,memdb);
	hashlist_add_elem(dynamic_db_list,db_list);
	return db_list;

}


int memdb_set_template(int type, int subtype,void * struct_template)
{
	DB_DESC * db_desc = Calloc0(sizeof(DB_DESC));
	if(db_desc == NULL)
		return -ENOMEM;
	void * db_list = memdb_get_dblist(type,subtype);
	if(db_list == NULL)
		return NULL;
	db_desc->head.type=type;
	db_desc->head.subtype=subtype;
	db_desc->struct_template = struct_template;
	return hashlist_set_desc(db_list,db_desc);
}
*/
void * memdb_get_template(int type, int subtype)
{
	struct memdb_desc * db_list;
	DB_RECORD * db_record;
	if(type<=0)
		return -EINVAL;
	if(type<DB_BASEEND)
	{
		db_list=memdb_get_dblist(type,subtype);
		return db_list->struct_template;
	}	
	
	db_record = memdb_get_recordtype(type,subtype);
	if(db_record == NULL)
		return NULL;
	return db_record->tail;
	
}
/*
int memdb_set_index(int type,int subtype,int flag,char * elem_list)
{
	void * base_struct_template;
	int ret;

	// init the index db
	base_struct_template=memdb_get_template(type,subtype);
	if(base_struct_template==NULL)
		return -EINVAL;
	if(flag==0)
		return -EINVAL;
	ret=struct_set_flag(base_struct_template,flag,elem_list);
	return ret;
}
int memdb_store_index(void * record, char * name,int flag)
{
	int ret;
	BYTE buffer[4096];
	UUID_HEAD * head = record;
	if(head==NULL)
		return -EINVAL;
	void * db_list = memdb_get_dblist(DB_INDEX,0);
	if(db_list == NULL)
		return NULL;
	
	INDEX_ELEM * index;
	INDEX_ELEM * findindex;

	ret=Galloc(&index,sizeof(INDEX_ELEM));
	if(ret<0)
		return -ENOMEM;
	
	Memcpy(index->uuid,head->uuid,DIGEST_SIZE);
	index->flag=flag;
	index->head.type=head->type;
	index->head.subtype=head->subtype;
	if(name==NULL)
		Memcpy(index->head.name,head->name,DIGEST_SIZE);
	else
	{
		memset(index->head.name,0,DIGEST_SIZE);
		strncpy(index->head.name,name,DIGEST_SIZE);
	}

	if(flag==0)
	{
		Memcpy(index->head.uuid,head->uuid,DIGEST_SIZE);
	}
	else
	{
		void * db_template;
		db_template=memdb_get_template(head->type,head->subtype);
		if(db_template==NULL)
		{
			Free0(index);
			return -EINVAL;
		}
		ret=struct_2_part_blob(record,buffer,db_template,flag);
		if(ret<0)
			return -EINVAL;

		ret=calculate_context_sm3(buffer,ret,index->head.uuid);
		if(ret<0)
			return ret;	
	}
	findindex=hashlist_find_elem(db_list,index);
	if(findindex!=NULL)
	{
		if(!memcmp(findindex->head.uuid,index->head.uuid,DIGEST_SIZE))
			return -EINVAL;
		Free0(findindex);	
	}
	return hashlist_add_elem(db_list,index);
}

INDEX_ELEM * memdb_find_index_byuuid(BYTE * uuid)
{
	INDEX_ELEM * index;
	index=memdb_get_first(DB_INDEX,0);
	while(index!=NULL)
	{
		if(!memcmp(uuid,index->uuid,DIGEST_SIZE))
		{
			return index;	
		}
		index=memdb_get_next(DB_INDEX,0);
	}
	return NULL;	
	
}

*/
int memdb_init()
{
	int ret;
	int i;
	void * struct_template; 
	void * base_struct_template;
	void * namelist_template;
	void * typelist_template;
	void * subtypelist_template;
	void * recordtype_template;
	void * index_template;
	struct struct_namelist  * baselist;
	struct struct_typelist  * basetypelist;
	struct struct_namelist * templist, *templist1;
	DB_RECORD *record;
	struct memdb_desc * curr_memdb;


	// alloc memspace for static database 
	ret=Galloc0(&static_db_list,sizeof(void *)*DB_DTYPE_START);
	if(ret<0)
		return ret;

	// generate two init namelist

	
	templist=Talloc(sizeof(struct struct_namelist));
	templist->elem_no=_get_namelist_no(&elem_type_valuelist_array);
	templist->elemlist=&elem_type_valuelist_array;

	
	templist1=Talloc(sizeof(struct struct_namelist));
	templist1->elem_no=_get_namelist_no(&memdb_elem_type_array);
	templist1->elemlist=&memdb_elem_type_array;

	ret=Galloc0(&elemenumlist,sizeof(struct struct_namelist));
	if(ret<0)
		return ret;

	elemenumlist=_merge_namelist(templist,templist1);


	ret=Galloc0(&typeenumlist,sizeof(struct struct_namelist));
	if(ret<0)
		return ret;

	templist->elem_no=_get_namelist_no(&struct_type_baselist);
	templist->elemlist=&struct_type_baselist;
	
	typeenumlist=_clone_namelist(templist);
	

	// register the new elem ops
	for(i=0;MemdbElemInfo[i].type!=CUBE_TYPE_ENDDATA;i++)
	{
		ret=struct_register_ops(MemdbElemInfo[i].type,
			MemdbElemInfo[i].enum_ops,
			MemdbElemInfo[i].flag,
			MemdbElemInfo[i].offset);
		if(ret<0)
			return ret;
	}
	

	// init those head template and elem template
	head_template= create_struct_template(&uuid_head_desc);
	if(head_template==NULL)
		return -EINVAL;

	elem_template= create_struct_template(&elem_attr_octet_desc);
	if(elem_template==NULL)
		return -EINVAL;

	// init the DB_NAMELIST lib
	ret=memdb_register_db(DB_NAMELIST,0,&struct_namelist_desc,&_namelist_tail_func,NULL);
	if(ret<0)
		return ret;

	// store the two init record of DB_NAMELIST
	record = memdb_store(elemenumlist,DB_NAMELIST,0,"elemenumlist");
	if(record==NULL)
		return -EINVAL;

	record = memdb_store(typeenumlist,DB_NAMELIST,0,"typeenumlist");
	if(record==NULL)
		return -EINVAL;

	record = memdb_store(typeenumlist,DB_NAMELIST,0,"baseenumlist");
	if(record==NULL)
		return -EINVAL;

	// init the DB_STRUCT_DESC lib
	ret=memdb_register_db(DB_STRUCT_DESC,0,&struct_define_desc,&_struct_desc_tail_func,"elem_no,elem_desc.name,elem_desc.type,elem_desc.size,elem_desc.ref,elem_desc.def");
	if(ret<0)
		return ret;

	// init the DB_TYPELIST lib
	ret=memdb_register_db(DB_TYPELIST,0,&struct_typelist_desc,&_typelist_tail_func,NULL);
	if(ret<0)
		return ret;

	// init the DB_SUBTYPELIST lib
	ret=memdb_register_db(DB_SUBTYPELIST,0,&struct_subtypelist_desc,&_subtypelist_tail_func,NULL);
	if(ret<0)
		return ret;

	// init the DB_RECORDTYPE lib
	ret=memdb_register_db(DB_RECORDTYPE,0,&struct_recordtype_desc,&_recordtype_tail_func,NULL);
	if(ret<0)
		return ret;

	// store the init typelist

	dynamic_db_list=init_hash_list(8,DB_DTYPE_START,0);

	return 0;
}

void *  memdb_store(void * data,int type,int subtype,char * name)
{
	int ret;
	struct memdb_desc * db_list;
	UUID_HEAD * head;
	DB_RECORD * record;
	DB_RECORD * oldrecord;
	db_list=memdb_get_dblist(type,subtype);
	if(db_list==NULL)
		return NULL;
	ret=Galloc0(&record,sizeof(DB_RECORD));
	if(ret<0)
		return -EINVAL;

	// build a faked record and comp uuid

	record->head.type=type;
	record->head.subtype=subtype;
	record->record=data;
	ret=memdb_comp_uuid(record);
	if(ret<0)
		return NULL;

	// look for the old record with same uuid
	oldrecord=hashlist_find_elem(db_list->record_db,record);
	if(oldrecord==NULL)
	{
		// if there is no old record, build a new one;
		if(name!=NULL)
		{
			Strncpy(record->head.name,name,DIGEST_SIZE);
			record->name_no=1;
		}
		if(db_list->tail_func!=NULL)
		{
			ret=db_list->tail_func(db_list,record);	
			if(ret<0)
				return NULL;
		}
		void * struct_template=memdb_get_template(type,subtype);
		if(struct_template==NULL)
		{
			Free0(record);
			return NULL;
		}
		record->record=clone_struct(data,struct_template);
		ret=hashlist_add_elem(db_list->record_db,record);
		if(ret<0)
			return NULL;
	}
	else
	{
		
		Free0(record);
		ret=_memdb_record_add_name(oldrecord,name);
		if(ret<0)
			return NULL;
		return oldrecord;				
	}
	return record;
}

int memdb_store_record(void * record)
{
	int ret;
	struct memdb_desc * db_list;
	UUID_HEAD * head;
	DB_RECORD * db_record=record;
	DB_RECORD * oldrecord;
	if(db_record==NULL)
		return -EINVAL; 
	if(db_record->record==NULL)
		return -EINVAL;	
	db_list=memdb_get_dblist(db_record->head.type,db_record->head.subtype);
	if(db_list==NULL)
		return -EINVAL;

	if(db_record->tail==NULL)
	{
		if(db_list->tail_func!=NULL)
		{
			ret=db_list->tail_func(db_list,db_record);	
			if(ret<0)
				return ret;
		}
	}
	ret=memdb_comp_uuid(db_record);
	if(ret<0)
		return ret;
	oldrecord=hashlist_find_elem(db_list->record_db,record);
	if(oldrecord==NULL)
	{
		void * struct_template=memdb_get_template(db_record->head.type,
			db_record->head.subtype);
		if(struct_template==NULL)
		{
			Free0(record);
			return -EINVAL;
		}
		ret=hashlist_add_elem(db_list->record_db,record);
		if(ret<0)
			return -EINVAL;
	}
	else
	{
		
		ret=_memdb_record_add_name(oldrecord,db_record->head.name);
		if(ret<0)
			return -EINVAL;
		return ret;				
	}
	return 1;
}

void * memdb_remove(void * uuid,int type,int subtype)
{
	int ret;
	struct memdb_desc * db_list;
	UUID_HEAD * head;
	DB_RECORD * record;
	db_list=memdb_get_dblist(type,subtype);
	if(db_list==NULL)
		return NULL;

	record=hashlist_remove_elem(db_list->record_db,uuid);

	return record;
}

int memdb_remove_byname(char * name,int type,int subtype)
{
	int ret;
	struct memdb_desc * db_list;
	UUID_HEAD * head;
	DB_RECORD * record;
	db_list=memdb_get_dblist(type,subtype);
	if(db_list==NULL)
		return -EINVAL;
	
	record=memdb_find_byname(name,type,subtype);
	if(record==NULL)
		return 0;
	ret=_memdb_record_remove_name(record,name);
	return ret;		
}

int memdb_free_record(void * record)
{
	int ret;
	void * struct_template;
	DB_RECORD * db_record=record;
	struct_template=memdb_get_template(db_record->head.type,db_record->head.subtype);
	if(struct_template==NULL)
		return -EINVAL;
	ret=struct_free(db_record->record,struct_template);
	if(ret<0)
		return -EINVAL;	
	Free0(db_record);
	return 0;
}

void * memdb_find(void * data,int type,int subtype)
{
	int ret;
	struct memdb_desc * db_list;
	db_list=memdb_get_dblist(type,subtype);
	if(db_list==NULL)
		return NULL;
	return hashlist_find_elem(db_list->record_db,data);
}

void * memdb_find_byname(char * name,int type,int subtype)
{
	int ret;
	struct memdb_desc * db_list;
	DB_RECORD * record;

	record=memdb_get_first(type,subtype);	
	
	while(record!=NULL)
	{
		ret=_memdb_record_find_name(record,name);
		if(ret<0)
			return NULL;
		if(ret>0)
			return record;
		record=memdb_get_next(type,subtype);
	}
	
	return NULL;
}

void * memdb_get_subtypelist(int type)
{
	DB_RECORD * record;
	struct struct_subtypelist * subtypelist;	
	record=memdb_get_first(DB_SUBTYPELIST,0);

	while(record!=NULL)
	{
		subtypelist=record->record;
		if(subtypelist->type==type)
			break;
		record=memdb_get_next(DB_SUBTYPELIST,0);
	}
	if(record==NULL)
		return NULL;
	return record->tail;
}

int memdb_print(void * data,char * json_str)
{
	DB_RECORD * record = data;
	void * struct_template ; 
	int ret;
	int offset;
	char * buf="\"record\":";

	struct_template=memdb_get_template(record->head.type,record->head.subtype);
	if(struct_template==NULL)
		return -EINVAL;	
	Strcpy(json_str,"{\"head\":");
	offset=Strlen(json_str);
	ret=struct_2_json(&record->head,json_str+offset,head_template);
	if(ret<0)
		return ret;
	offset+=ret;
	json_str[offset++]=',';
	ret=Strlen(buf);
	Memcpy(json_str+offset,buf,ret);
	offset+=ret;
	ret=struct_2_json(record->record,json_str+offset,struct_template);
	if(ret<0)
		return ret;
	offset+=ret;
	json_str[offset++]='}';
	json_str[offset++]=0;
	
	return offset;
}


int memdb_get_typeno(char * typestr)
{
	if(typeenumlist==NULL)
		return -EINVAL;
	return _get_value_namelist(typestr,typeenumlist);

}

int memdb_get_subtypeno(int typeno,char * typestr)
{
	struct struct_subtypelist  * subtypelist=memdb_get_subtypelist(typeno);
	if(subtypelist==NULL)
		return -EINVAL;
	return _get_value_namelist(typestr,subtypelist);

}

int memdb_comp_uuid(void * record)
{
	BYTE * buf;	
	int blob_size;
	DB_RECORD * db_record=record;
	void * struct_template=memdb_get_template(db_record->head.type,db_record->head.subtype);
	if(struct_template == NULL)
		return -EINVAL;
	if(record==NULL)
		return -EINVAL;
	buf=Talloc(4000);
	if(buf==NULL)
		return -ENOMEM;
	
	blob_size=struct_2_part_blob(db_record->record,buf,struct_template,CUBE_ELEM_FLAG_INDEX);
	if(blob_size>0)
	{
		calculate_context_sm3(buf,blob_size,&db_record->head.uuid);
	}
	Free0(buf);
	return blob_size;
}
