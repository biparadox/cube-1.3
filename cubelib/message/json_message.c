
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "../include/data_type.h"
#include "../include/list.h"
#include "../include/memfunc.h"
#include "../include/alloc.h"
#include "../include/json.h"
#include "../include/struct_deal.h"
//#include "../include/valuelist.h"
#include "../include/basefunc.h"
#include "../include/memdb.h"
#include "../include/message.h"

#include "message_box.h"

int json_2_message(char * json_str,void ** message)
{

    void * root_node;
    void * head_node;
    void * tag_node;
    void * record_node;
    void * curr_record;
    void * expand_node;
    void * curr_expand;

    void * record_value;
    void * expand_value;

    struct message_box * msg_box;
    MSG_HEAD * msg_head;
    MSG_EXPAND * msg_expand;
    int record_no;
    int expand_no;
    void * precord;
    void * pexpand;
    int i;
    int ret;
    char buffer[DIGEST_SIZE];

    int offset;
    int type;
    int subtype;

    offset=json_solve_str(&root_node,json_str);
    if(offset<0)
        return offset;

    // get json node's head
    head_node=json_find_elem("HEAD",root_node);
    if(head_node==NULL)
    	head_node=json_find_elem("head",root_node);
    if(head_node==NULL)
        return -EINVAL;
    tag_node=json_find_elem("tag",head_node);
    if(tag_node!=NULL)   // default tag value is "MESG"
    {
    	ret=json_node_getvalue(tag_node,buffer,10);
    	if(ret!=4)
        	return -EINVAL;
	if(Memcmp(buffer,"MESG",ret)!=0)
		return -EINVAL;
    }	
    msg_box=message_init();
    msg_head=message_get_head(msg_box);
    json_2_struct(head_node,msg_head,msg_box->head_template);


    // get json node's record
    // init message box
    ret=message_record_init(msg_box);
    if(ret<0)
        return ret;

    record_node=json_find_elem("RECORD",root_node);
    if(record_node==NULL)
    	record_node=json_find_elem("record",root_node);
    if(record_node==NULL)
        return -EINVAL;

    curr_record=json_get_first_child(record_node);
    if(curr_record==NULL)
         return -EINVAL;
    char node_name[DIGEST_SIZE*2];
    ret=json_node_getname(curr_record,node_name);
    if(!strcmp(node_name,"BIN_FORMAT"))
    {
	BYTE * radix64_string;
	radix64_string=malloc(4096);
	if(radix64_string==NULL)
		return -ENOMEM;
	ret=json_node_getvalue(curr_record,radix64_string,4096);
	if(ret<0)
		return -EINVAL;
	int radix64_len=strnlen(radix64_string,4096);
	msg_head->record_size=radix_to_bin_len(radix64_len);
	msg_box->blob=malloc(msg_head->record_size);
	if(msg_box->blob==NULL)
		return -ENOMEM;
	ret=radix64_to_bin(msg_box->blob,radix64_len,radix64_string);
   }
    else
   {
    	for(i=0;i<msg_head->record_num;i++)
    	{
        	if(curr_record==NULL)
            		return -EINVAL;
        	precord=Dalloc0(struct_size(msg_box->record_template),msg_box);
        	if(precord==NULL)
            		return -ENOMEM;
       		json_2_struct(curr_record,precord,msg_box->record_template);
        	message_add_record(msg_box,precord);
        	curr_record=json_get_next_child(record_node);
	}
    }

    // get json_node's expand
    expand_no=msg_head->expand_num;
    msg_head->expand_num=0;
    expand_node=json_find_elem("EXPAND",root_node); 
    if(expand_node==NULL)
    	expand_node=json_find_elem("expand",root_node);
    if(expand_node!=NULL)
    {
	char buf[20];
	void * curr_expand_template;
	curr_expand=json_get_first_child(expand_node);
   	for(i=0;i<expand_no;i++)
    	{
        	if(curr_expand==NULL)
            		return -EINVAL;
		msg_expand=Dalloc0(sizeof(*msg_expand),msg_box);
		if(msg_expand==NULL)
			return -ENOMEM;

		ret=json_2_struct(curr_expand,msg_expand,message_get_expand_template());
		if(ret<0)
			return ret;

		void * tempnode;
		if((tempnode=json_find_elem("expand",curr_expand))!=NULL)
		{
			curr_expand_template=memdb_get_template(msg_expand->type,msg_expand->subtype);
			if(curr_expand_template==NULL)
				return -EINVAL;
			msg_expand->data_size=0;
			msg_expand->expand=Dalloc0(struct_size(curr_expand_template),msg_box);
			if(msg_expand->expand==NULL)
				return -ENOMEM;
			ret=json_2_struct(tempnode,msg_expand->expand,curr_expand_template);
			if(ret<0)
				return ret;
		}
		else
		{
			ret=json_2_struct(curr_expand,msg_expand,message_get_expand_bin_template());
			if(ret<0)
				return ret;
			
		}
		
        	message_add_expand(msg_box,msg_expand);
        	curr_expand=json_get_next_child(expand_node);

	}
    }


    *message=msg_box;
    msg_box->box_state = MSG_BOX_RECOVER;
    return offset;
}
