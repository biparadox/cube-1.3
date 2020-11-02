
#include "../include/errno.h"
#include "../include/data_type.h"
#include "../include/list.h"
#include "../include/attrlist.h"
#include "../include/memfunc.h"
#include "../include/alloc.h"
#include "../include/json.h"
#include "../include/struct_deal.h"
#include "../include/basefunc.h"
#include "../include/memdb.h"

#include "memdb_internal.h"

void * head_template;

int read_namelist_json_desc(void * root,void * record)
{
	int ret;
	struct struct_namelist * namelist;
	DB_RECORD * db_record=record;
	void * namelist_template;
	
	void * temp_node;

	namelist =Dalloc0(sizeof(struct struct_namelist),record);
	if(namelist==NULL)
		return -ENOMEM;
	if(db_record->head.type!=DB_NAMELIST)
		return -EINVAL;

	temp_node=json_find_elem("elemlist",root);
	if(temp_node==NULL)
		return -EINVAL;
	namelist_template=memdb_get_template(DB_NAMELIST,0);
	if(namelist_template==NULL)
		return -EINVAL;

	ret=json_2_struct(root,namelist,namelist_template);
//	namelist->elem_no=json_get_elemno(temp_node);/
	db_record->record=namelist;

	ret=memdb_comp_uuid(db_record);
	if(ret<0)
		return ret;	
	ret=memdb_store_record(db_record);
	return ret;	
}

int read_typelist_json_desc(void * root,void * record)
{
	int ret;
	struct struct_namelist * namelist;
	struct struct_namelist * baselist;
	struct struct_typelist * typelist;
	struct struct_typelist * basetypelist;

	int * temp_node;
	void * namelist_template;
	void * typelist_template;
	DB_RECORD * db_record=record;
	DB_RECORD * namelist_record;


	typelist=Dalloc0(sizeof(struct struct_typelist),record);
	if(typelist==NULL)
		return -ENOMEM;
	if(db_record->head.type!=DB_TYPELIST)
		return -EINVAL;
	
//      store the namelist (if exists) and compute the uuid

	temp_node=json_find_elem("uuid",root);
	if(temp_node == NULL)
	{
		// this typelist use namelist describe, 
		// we should finish namelist store first	
		namelist_record=Calloc0(sizeof(DB_RECORD));
		if(namelist_record==NULL)
			return -ENOMEM;
		namelist_record->head.type=DB_NAMELIST;
		ret=read_namelist_json_desc(root,namelist_record);
		if(ret<0)
			return ret;
		Memcpy(typelist->uuid,namelist_record->head.uuid,DIGEST_SIZE);
		namelist=namelist_record->record;
		typelist->elem_no=namelist->elem_no;			
	}
	else
	{
		typelist_template=memdb_get_template(DB_TYPELIST,0);
		if(typelist_template==NULL)
			return -EINVAL;

		ret=json_2_struct(root,typelist,typelist_template);
		namelist_record=memdb_find(typelist->uuid,DB_NAMELIST,0);
		if(namelist_record==NULL)
			return -EINVAL;
		namelist=namelist_record->record;
		if(typelist->elem_no==0)
			typelist->elem_no=namelist->elem_no;
	}

	db_record->record=typelist;
	ret=memdb_store_record(db_record);

	// merge the typelist 

	namelist_record=memdb_find_byname("typeenumlist",DB_NAMELIST,0);
	
	if(namelist_record==NULL)
		return -EINVAL;

	baselist=_merge_namelist(namelist_record->record,db_record->tail);
	if(baselist==NULL)
		return -EINVAL;

	ret=memdb_remove_byname("typeenumlist",DB_NAMELIST,0);
	if(ret<0)
		return -EINVAL;
	
	memdb_store(baselist,DB_NAMELIST,0,"typeenumlist");
	typeenumlist=baselist;

	return ret;	
}
		
int read_subtypelist_json_desc(void * root,void * record)
{
	int ret;
	struct struct_namelist * namelist;
	struct struct_subtypelist * subtypelist;

	int * temp_node;
	void * namelist_template;
	void * subtypelist_template;
	DB_RECORD * db_record=record;
	DB_RECORD * namelist_record;


	subtypelist=Dalloc0(sizeof(struct struct_subtypelist),record);
	if(subtypelist==NULL)
		return -ENOMEM;
	if(db_record->head.type!=DB_SUBTYPELIST)
		return -EINVAL;
	
//      store the namelist (if exists) and compute the uuid

	temp_node=json_find_elem("uuid",root);
	if(temp_node == NULL)
	{
		// this typelist use namelist describe, 
		// we should finish namelist store first	
		namelist_record=Calloc0(sizeof(DB_RECORD));
		if(namelist_record==NULL)
			return -ENOMEM;
		namelist_record->head.type=DB_NAMELIST;
		ret=read_namelist_json_desc(root,namelist_record);
		if(ret<0)
		{
			Free0(namelist_record);
			return ret;
		}
		Memcpy(subtypelist->uuid,namelist_record->head.uuid,DIGEST_SIZE);
		namelist=namelist_record->record;
		subtypelist->elem_no=namelist->elem_no;			
		temp_node=json_find_elem("type",root);
		if(temp_node==NULL)
		{
			return -EINVAL;
		}
		subtypelist->type=memdb_get_typeno(json_get_valuestr(temp_node));
		if(subtypelist->type<0)
		{
			return -EINVAL;
		}
	
	}
	else
	{
		subtypelist_template=memdb_get_template(DB_SUBTYPELIST,0);
		if(subtypelist_template==NULL)
			return -EINVAL;

		ret=json_2_struct(root,subtypelist,subtypelist_template);
		namelist_record=memdb_find(subtypelist->uuid,DB_NAMELIST,0);
		if(namelist_record==NULL)
			return -EINVAL;
		namelist=namelist_record->record;
		if(subtypelist->elem_no==0)
			subtypelist->elem_no=namelist->elem_no;
	}

	db_record->record=subtypelist;
	ret=memdb_store_record(db_record);

	return ret;	
}
		
int read_default_json_desc(void * root,void * record)
{

	int ret;
	void * data;
	void * struct_template;

	DB_RECORD * db_record=record;
	
	void * temp_node;

	if(db_record->head.type <=0)
		return -EINVAL;

	struct_template=memdb_get_template(db_record->head.type,db_record->head.subtype);
	if(struct_template==NULL)
		return -EINVAL;

	data=Dalloc0(struct_size(struct_template),record);
	if(data==NULL)
		return -ENOMEM;

	ret=json_2_struct(root,data,struct_template);
//	namelist->elem_no=json_get_elemno(temp_node);
	db_record->record=data;

	ret=memdb_comp_uuid(db_record);
	if(ret<0)
		return ret;	
	ret=memdb_store_record(db_record);
	return ret;	
}

void * memdb_read_struct_template(void * node)
{
	DB_RECORD * struct_record;
	void * struct_template;
	BYTE uuid[DIGEST_SIZE];
	int ret;
	struct_record=Calloc0(sizeof(*struct_record));
	struct_record->head.type=DB_STRUCT_DESC;
	ret=read_default_json_desc(node,struct_record);
	if(ret<0)
		return NULL;
	struct_template=create_struct_template(struct_record->tail);
	
	return struct_template;		
}


int read_recordtype_json_desc(void * root,void * record)
{

	int ret;
	void * data;
	void * struct_template;

	DB_RECORD * db_record=record;
	DB_RECORD * struct_record;
	struct struct_desc_record * struct_desc;
	struct struct_recordtype * recordtype;
	
	void * temp_node;

	if(db_record->head.type <=0)
		return -EINVAL;

	struct_template=memdb_get_template(db_record->head.type,db_record->head.subtype);
	if(struct_template==NULL)
		return -EINVAL;

	recordtype=Dalloc0(sizeof(struct struct_recordtype),record);
	if(recordtype==NULL)
		return -ENOMEM;

	temp_node=json_find_elem("uuid",root);
	if(temp_node==NULL)
	{
		temp_node=json_find_elem("struct_desc",root);
		if(temp_node==NULL)
			return -EINVAL;
		struct_record=Calloc0(sizeof(*struct_record));
		if(struct_record==NULL)
			return -ENOMEM;
		struct_record->head.type=DB_STRUCT_DESC;
		Memcpy(struct_record->head.name,db_record->head.name,DIGEST_SIZE);
		ret=read_default_json_desc(temp_node,struct_record);
		if(ret<0)
			return ret;	
		Memcpy(recordtype->uuid,struct_record->head.uuid,DIGEST_SIZE);
	}
	else
	{
		char * uuid_str=json_get_valuestr(temp_node);
	
		if(!Isvaliduuid(uuid_str))
		{
			struct_record=memdb_find_byname(uuid_str,DB_STRUCT_DESC,0);
			if(struct_record==NULL)
				return -EINVAL;		
			Memcpy(recordtype->uuid,struct_record->head.uuid,DIGEST_SIZE);
			ret=json_remove_node(temp_node);
		}
	}
//	struct_template=create_struct_template(struct_record->tail);
//	db_record->tail=struct_template;

	ret=json_2_struct(root,recordtype,struct_template);
//	namelist->elem_no=json_get_elemno(temp_node);
	db_record->record=recordtype;

	ret=memdb_comp_uuid(db_record);
	if(ret<0)
		return ret;	
	ret=memdb_store_record(db_record);
	if(ret<0)
		return ret;
	ret=memdb_register_dynamicdb(recordtype->type,recordtype->subtype);

	return ret;	
}

int memdb_read_desc(void * root, BYTE * uuid)
{
	void * temp_node;
	void * head_node;
	void * record_node;
	int i;
	int ret;
	char * typestr;
	DB_RECORD * db_record;
	
	head_node=json_find_elem("head",root);
	if(head_node==NULL)
	{
		return -EINVAL;
	}
	record_node=json_find_elem("record",root);
	if(record_node==NULL)
	{
		return -EINVAL;
	}
	
	
	db_record=Calloc0(sizeof(DB_RECORD));
	if(db_record==NULL)
		return -ENOMEM;
	
	ret=json_2_struct(head_node,&(db_record->head),head_template);
	if(ret<0)
		return -EINVAL;	
	
	if(db_record->head.type<DB_DTYPE_START)
	{
		switch(db_record->head.type)
		{
			case DB_NAMELIST:
				ret=read_namelist_json_desc(record_node,db_record);
				break;
			case DB_STRUCT_DESC:
				ret=read_default_json_desc(record_node,db_record);
				break;
			case DB_TYPELIST:
				ret=read_typelist_json_desc(record_node,db_record);
				break;
			case DB_SUBTYPELIST:
				ret=read_subtypelist_json_desc(record_node,db_record);
				break;
			case DB_CONVERTLIST:
				break;
			case DB_RECORDTYPE:
				ret=read_recordtype_json_desc(record_node,db_record);
				break;
			default:
				return -EINVAL;
		}
	}
	else
	{
		ret=read_default_json_desc(record_node,db_record);

	}
	if(ret<0)
	{
		printf("read record %s failed!\n",db_record->head.name);
	}

	return ret;
	
}

int memdb_output_blob(void * record,BYTE * blob,int type,int subtype)
{
	int ret;
	void * record_template;
	record_template=memdb_get_template(type,subtype);
	if(record_template==NULL)
		return -EINVAL;
	ret=struct_2_blob(record,blob,record_template);
	return ret;
}

int memdb_read_blob(BYTE * blob,void * record,int type,int subtype)
{
	int ret;
	void * record_template;
	record_template=memdb_get_template(type,subtype);
	if(record_template==NULL)
		return -EINVAL;
	ret=blob_2_struct(blob,record,record_template);
	return ret;
}
