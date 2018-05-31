#include <errno.h>

#include "../include/data_type.h"
#include "../include/list.h"
#include "../include/attrlist.h"
#include "../include/memfunc.h"
#include "../include/alloc.h"
#include "../include/json.h"
#include "../include/struct_deal.h"
#include "../include/basefunc.h"
#include "../include/memdb.h"
#include "../include/message.h"
#include "../include/dispatch.h"

#include "dispatch_struct.h"


static POLICY_LIST process_policy;
static POLICY_LIST aspect_policy;

static void * policy_head_template;
static void * match_rule_template;
static void * route_rule_template;

static int match_flag = 0x10000000;

static inline int _init_policy_list(void * list)
{
    	POLICY_LIST * policy_list=(POLICY_LIST *)list;
   	memset(policy_list,0,sizeof(POLICY_LIST));
   	INIT_LIST_HEAD(&(policy_list->head.list));
   	policy_list->head.record=NULL;
	policy_list->policy_num=0;
	return 0;

}

int dispatch_init( )
{
	int ret;
	ret=_init_policy_list(&process_policy);
	ret=_init_policy_list(&aspect_policy);
    	policy_head_template=memdb_get_template(DTYPE_DISPATCH,SUBTYPE_POLICY_HEAD);
    	match_rule_template=memdb_get_template(DTYPE_DISPATCH,SUBTYPE_MATCH_HEAD);
    	route_rule_template=memdb_get_template(DTYPE_DISPATCH,SUBTYPE_ROUTE_RULE);
	return 0;
}

const char * dispatch_policy_getname(void * policy)
{
    DISPATCH_POLICY * dispatch_policy=policy;
    if(policy==NULL)
	    return -EINVAL;
    return dispatch_policy->name;
}
int dispatch_policy_getstate(void * policy)
{
    DISPATCH_POLICY * dispatch_policy=policy;
    if(policy==NULL)
	    return -EINVAL;
    return dispatch_policy->state;
}
int dispatch_policy_gettype(void * policy)
{
    DISPATCH_POLICY * dispatch_policy=policy;
    if(policy==NULL)
	    return -EINVAL;
    return dispatch_policy->type;
}
void * dispatch_policy_create()
{
	int ret;
	DISPATCH_POLICY * policy;
	policy=Calloc0(sizeof(DISPATCH_POLICY));
	if(policy==NULL)
		return NULL;
	_init_policy_list(&policy->match_list);
	_init_policy_list(&policy->route_list);
	return policy;
	
}

int read_policy_from_buffer(char * buffer, int max_len)
{
	int offset=0;
	int ret;
	void * root;
	int solve_offset;
	int count=0;
	void * policy;

	while(offset<max_len)
	{
      	        solve_offset=json_solve_str(&root,buffer+offset);
        	if(solve_offset<=0)
			break;
		offset+=solve_offset;

        	policy=dispatch_read_policy(root);
		if(policy==NULL)
			break;
		count++;
	}
	return count;
}

void * dispatch_read_policy(void * policy_node)
{
    void * match_policy_node;
    void * route_policy_node;
    void * match_rule_node;
    void * route_rule_node;
    void * temp_node;
    char buffer[1024];
    char * temp_str;
    int ret;
    MATCH_RULE * temp_match_rule;
    ROUTE_RULE * temp_route_rule;

    DISPATCH_POLICY * policy=dispatch_policy_create();
    if(policy==NULL)
        return NULL;

    temp_node=json_find_elem("policy_head",policy_node);
    if(temp_node==NULL)
    {
	return NULL;
    }
    ret=json_2_struct(temp_node,policy,policy_head_template);
    if(ret<0)
        return NULL;
    // get the match policy json node
    match_policy_node=json_find_elem("MATCH_RULES",policy_node);
    if(match_policy_node==NULL)
        return -EINVAL;

    // get the match policy json node
    route_policy_node=json_find_elem("ROUTE_RULES",policy_node);

    if(route_policy_node==NULL)
        return -EINVAL;

    // read the match rule

    match_rule_node=json_get_first_child(match_policy_node);
    while(match_rule_node!=NULL)
    {
	void * record_template;
        temp_match_rule=Dalloc0(sizeof(MATCH_RULE),policy);
	if(temp_match_rule==NULL)
		return NULL;
        ret=json_2_struct(match_rule_node,temp_match_rule,match_rule_template);
        if(ret<0)
            return NULL;
	temp_node=json_find_elem("value",match_rule_node);
	if(temp_node!=NULL)
	{
		void * record_desc;
		void * value_struct;
		record_template=memdb_get_template(temp_match_rule->type,temp_match_rule->subtype);
		if(record_template==NULL)
			return NULL;
		record_template=clone_struct_template(record_template);
		if(record_template==NULL)
			return NULL;
		temp_match_rule->match_template=record_template;
		ret=json_marked_struct(temp_node,record_template,match_flag);
		if(ret<0)
			return NULL;
		value_struct=Dalloc0(struct_size(record_template),policy);
		if(value_struct==NULL)
		{
			free_struct_template(record_template);
			return NULL;
		}
		ret=json_2_part_struct(temp_node,value_struct,temp_match_rule->match_template,match_flag);
		if(ret<0)
			return NULL;	
		temp_match_rule->value=value_struct;
	}
	ret=dispatch_policy_addmatchrule(policy,temp_match_rule);
	if(ret<0)
		return NULL;	
	
//        __route_policy_add(policy->match_list,temp_match_rule);
         match_rule_node=json_get_next_child(match_policy_node);
    }

    // read the route policy
    // first,read the main route policy

    temp_node=json_find_elem("main_policy",route_policy_node);
    if(temp_node==NULL)
	return -EINVAL;	

    route_rule_node=json_get_first_child(temp_node);
    while(route_rule_node!=NULL)
    {
    	temp_route_rule=Dalloc0(sizeof(ROUTE_RULE),policy);
    	if(temp_route_rule==NULL)
		return NULL;
   	 temp_route_rule=malloc(sizeof(ROUTE_RULE));
    	ret=json_2_struct(route_rule_node,temp_route_rule,route_rule_template);
    	if(ret<0)
        	return NULL;
	ret=dispatch_policy_addrouterule(policy,temp_route_rule);
	if(ret<0)
		return NULL;	
        route_rule_node=json_get_next_child(temp_node);
    } 

    return policy;
}

int _dispatch_policy_getfirst(void * policy_list,void ** policy)
{
	Record_List * recordhead;
	Record_List * newrecord;
        POLICY_LIST * dispatch_policy=(POLICY_LIST *)policy_list;

        recordhead = &(dispatch_policy->head);
	if(recordhead==NULL)
		return -EINVAL;

	newrecord = (Record_List *)(recordhead->list.next);
	*policy=newrecord->record;
        dispatch_policy->curr=newrecord;
	return 0;
}

int dispatch_policy_getfirst(void ** policy)
{
    return _dispatch_policy_getfirst(&process_policy,policy);
}
int dispatch_aspect_policy_getfirst(void ** policy)
{
    return _dispatch_policy_getfirst(&aspect_policy,policy);
}


int dispatch_policy_getfirstmatchrule(void * policy,void ** rule)
{
    DISPATCH_POLICY * dispatch_policy=(DISPATCH_POLICY *)policy;
    return _dispatch_policy_getfirst(&dispatch_policy->match_list,rule);
}

int dispatch_policy_getfirstrouterule(void * policy,void ** rule)
{
    DISPATCH_POLICY * dispatch_policy=(DISPATCH_POLICY *)policy;
    return _dispatch_policy_getfirst(&dispatch_policy->route_list,rule);
}

int _dispatch_policy_getnext(void * policy_list,void ** policy)
{
	Record_List * recordhead;
	Record_List * newrecord;
        POLICY_LIST * dispatch_policy=(POLICY_LIST *)policy_list;

        if(dispatch_policy->curr==NULL)
		return -EINVAL;
       if(dispatch_policy->curr==&(dispatch_policy->head.list))
      { 
              *policy=NULL;
	      return 0;
      }
	
       recordhead = dispatch_policy->curr;
	if(recordhead==NULL)
		return -EINVAL;

	newrecord = (Record_List *)(recordhead->list.next);
	*policy=newrecord->record;
        dispatch_policy->curr=newrecord;
	return 0;
}

int dispatch_policy_getnext(void ** policy)
{
    return _dispatch_policy_getnext(&process_policy,policy);
}
int dispatch_aspect_policy_getnext(void ** policy)
{
    return _dispatch_policy_getnext(&aspect_policy,policy);
}

int dispatch_policy_getnextmatchrule(void * policy,void ** rule)
{
    DISPATCH_POLICY * dispatch_policy=(DISPATCH_POLICY *)policy;
    return _dispatch_policy_getnext(&dispatch_policy->match_list,rule);
}

int dispatch_policy_getnextrouterule(void * policy,void ** rule)
{
    DISPATCH_POLICY * dispatch_policy=(DISPATCH_POLICY *)policy;
    return _dispatch_policy_getnext(&dispatch_policy->route_list,rule);
}

int _dispatch_rule_add(void * list,void * rule)
{
	int ret;
	Record_List * recordhead;
	Record_List * newrecord;
    	POLICY_LIST * rule_list=(POLICY_LIST *)list;

   	recordhead = &(rule_list->head);
	if(recordhead==NULL)
		return -ENOMEM;

	newrecord=Calloc0(sizeof(Record_List));
	if(newrecord==NULL)
		return -ENOMEM;
	INIT_LIST_HEAD(&(newrecord->list));
	newrecord->record=rule;
	List_add_tail(&(newrecord->list),recordhead);
	rule_list->policy_num++;
	return 1;
}

int dispatch_policy_addmatchrule(void * policy,void * rule)
{
    DISPATCH_POLICY * dispatch_policy=(DISPATCH_POLICY *)policy;
   	
    return _dispatch_rule_add(&dispatch_policy->match_list,rule);
}

int dispatch_policy_addrouterule(void * policy,void * rule)
{
    DISPATCH_POLICY * dispatch_policy=(DISPATCH_POLICY *)policy;
   	
    return _dispatch_rule_add(&dispatch_policy->route_list,rule);
}

int _dispatch_policy_add(void * list,void * policy)
{
	return _dispatch_rule_add(list,policy);	
}

int dispatch_policy_add(void * policy)
{
      void * general_policy_list;
      void * aspect_policy_list;
      DISPATCH_POLICY * dispatch_policy=(DISPATCH_POLICY *)policy;
      _dispatch_policy_add(&process_policy,policy);
      return 1;
}

int dispatch_aspect_policy_add(void * policy)
{
      void * general_policy_list;
      void * aspect_policy_list;
      DISPATCH_POLICY * dispatch_policy=(DISPATCH_POLICY *)policy;
      _dispatch_policy_add(&aspect_policy,policy);
      return 1;
}

int dispatch_match_sender(void * policy, char * sender)
{
	int ret;
	DISPATCH_POLICY * dispatch_policy=(DISPATCH_POLICY *)policy;
	if(policy==NULL)
		return 0;
	ret=Strcmp(dispatch_policy->sender,sender);
	if(ret!=0)
		return 0;
	return 1;
}

int dispatch_match_message_jump(void * policy, void * message)
{
	int ret;
	MSG_HEAD * msg_head;
	DISPATCH_POLICY * dispatch_policy=(DISPATCH_POLICY *)policy;
	if(policy==NULL)
		return 0;
	msg_head=message_get_head(message);
	if((dispatch_policy->rjump==0)||(dispatch_policy->rjump==msg_head->rjump))
		if((dispatch_policy->ljump==0)||(dispatch_policy->ljump==msg_head->ljump))
		{
			return 1;
		}
	return 0;
}



int dispatch_match_message(void * policy,void * message)
{
	int ret;
	int result=0;
	int prev_result=0;
	int match_rule_num=0;
	DISPATCH_POLICY * dispatch_policy=(DISPATCH_POLICY *)policy;
	MATCH_RULE * match_rule;
	MSG_HEAD * msg_head;
	void * msg_record;
	void * record_template;
	
	msg_head=message_get_head(message);
	if(msg_head==NULL)
		return -EINVAL;

	if(dispatch_policy->name!=NULL)
	{
		if(msg_head->route[0]!=0)
			if(Strncmp(dispatch_policy->name,msg_head->route,DIGEST_SIZE)!=0)
				return 0;	
	}

	ret=dispatch_policy_getfirstmatchrule(policy,&match_rule);
	if(ret<0)
		return ret;
	while(match_rule!=NULL)
	{
		if((match_rule->area==0)||(match_rule->area==MATCH_AREA_HEAD))
		{
			if(match_rule->type==0)
				result=1;
			else if(match_rule->type ==msg_head->record_type)
			{
				if(match_rule->subtype==0)
					result= 1;
				else if(match_rule->subtype==msg_head->record_subtype)
					result = 1;
			}
			else
			{
				result = 0;
			}
		}
		else if(match_rule->area==MATCH_AREA_RECORD)
		{	
			if(match_rule->type==0)
				result = 1;
			else if((match_rule->type!=msg_head->record_type)||
				(match_rule->subtype!=msg_head->record_subtype))
				result = 0;
			else if(match_rule->match_template==NULL)
				result = 1;
			else
			{
				ret=message_get_record(message,&msg_record,0);
				if(ret<0)
					result =ret;
				else if(msg_record==NULL)
					result = 0;
	
				else if(!struct_part_compare(match_rule->value,msg_record,match_rule->match_template,match_flag))
					result = 1;
				else
					result = 0;
			}
		}
		else if(match_rule->area==MATCH_AREA_EXPAND)
		{
			MSG_EXPAND * msg_expand;			
			void * expand_template;
			ret=message_get_define_expand(message,&msg_expand,match_rule->type,match_rule->subtype);
			if(msg_expand==NULL)
				result=0;
			else if(match_rule->value==NULL)
				result=1;
			else
			{
			//	expand_template=memdb_get_template(match_rule->type,match_rule->subtype);
				if(match_rule->match_template==NULL)
					result=0;
				else
				{
					if(!struct_part_compare(match_rule->value,msg_expand->expand,
							match_rule->match_template,match_flag))
						result = 1;
					else
						result = 0;
				}
			}	
		}
		else
		{
			result = 0;
		}
		
		if(result<0)
			return result;
		switch(match_rule->op)
		{
			case DISPATCH_MATCH_AND:
				if(result==0)
					return result;	
				prev_result=1;
				break;				
			case DISPATCH_MATCH_OR:
				if(result>0)
					return result;	
				break;				

			case DISPATCH_MATCH_NOT:
				if(result>0)
					return 0;	
				prev_result=1;
				break;
			case DISPATCH_MATCH_ORNOT:
				if(result==0)
					return 1;
				break;	
			
			default:
				return -EINVAL;	
		}
		match_rule_num++;
		ret=dispatch_policy_getnextmatchrule(policy,&match_rule);
		if(ret<0)
			return ret;
	}	
	if(match_rule_num==0)
		return 0;
	return prev_result;
}

int rule_get_target(void * router_rule,void * message,void **result)
{
    BYTE * target;
    int ret;
    char buffer[DIGEST_SIZE*2];
    ROUTE_RULE * rule= (ROUTE_RULE *)router_rule;
    *result=NULL;
    switch(rule->target_type){
	    case ROUTE_TARGET_NAME:
	    case ROUTE_TARGET_PORT:
	    case ROUTE_TARGET_LOCAL:
		    target=Talloc0(DIGEST_SIZE);
		    Strncpy(target,rule->target_name,DIGEST_SIZE);
		    break;
	    case ROUTE_TARGET_UUID:
		    ret=Strnlen(rule->target_name,DIGEST_SIZE*2);
		    target=Talloc0(DIGEST_SIZE);
		    if(ret ==DIGEST_SIZE*2)
		    {
			if(target==NULL)
				return -ENOMEM;
			ret=uuid_to_digest(rule->target_name,target);
			if(ret<0)
				return ret;
		    }		
		    else if(ret<DIGEST_SIZE/4*3)
		    {
		    	Memcpy(target,rule->target_name,ret);
		    }
		    else
			return -EINVAL;
		    break;
	    case ROUTE_TARGET_CONN:
		    target=Talloc0(DIGEST_SIZE);
		    if(target==NULL)
			return -ENOMEM;	
		    ret=Strnlen(rule->target_name,DIGEST_SIZE*2);
		    if(ret>DIGEST_SIZE/4*3-1)
			return -EINVAL;
		    target[0]=':';
		    Memcpy(target+1,rule->target_name,ret);
		    break;
	    case ROUTE_TARGET_RECORD:
		   {
			ret=message_read_elem(message,rule->target_name,0,&target);
			if(ret<0)
				return -EINVAL;
			if(target==NULL)
				return -EINVAL;
			break;
		   }

	    case ROUTE_TARGET_EXPAND:
		   {
			int expand_type;
			int expand_subtype;
			MSG_EXPAND * expand;
			void * expand_template;
			void * expand_data;
			int len;
		
			len=Strlen(rule->target_name);
			if(len>DIGEST_SIZE*3)
				return -EINVAL;
			
			int i=0;
			int offset=0;
			offset=message_read_typestr(rule->target_name,&expand_type,&expand_subtype);
			if(offset<0)
				return offset;
			while(rule->target_name[offset++]!=':')
			{
				if(offset>=len)
					return -EINVAL;
			}

		        ret = message_get_define_expand(message,&expand,expand_type,expand_subtype);
			if(ret<0)
				return -EINVAL;
			if(expand==NULL)
				return -EINVAL;
			expand_template=memdb_get_template(expand_type,expand_subtype);
			if(expand_template==NULL)
				return -EINVAL;		
			target=Talloc(DIGEST_SIZE*2+1);
			ret=struct_read_elem(rule->target_name+offset,expand->expand,target,expand_template);		
			if(ret<0)
				return ret;
		   	if(target==NULL)
				return -EINVAL;
		   	break;
		   }

/*
	    case ROUTE_TARGET_SPLIT:
		    return -EINVAL;

	    case ROUTE_TARGET_MIXUUID:
		    {
			    void * json_root;
			    void * json_elem;
			    const int repeat_time=2;
			    char mixvalue[repeat_time][DIGEST_SIZE*2];
			    char tag[16];
			    char uuid[DIGEST_SIZE*2];
			    int i;

			    ret=json_solve_str(&json_root,rule->target_name);
			    if(ret<0)
				    return ret;
			    if(json_get_type(json_root)!=JSON_ELEM_MAP)
				    return -EINVAL;
			
			    memset(uuid,0,DIGEST_SIZE*2);

			    char * value;
			    char name[DIGEST_SIZE*2];

			    for(i=0;i<repeat_time;i++)
			    {
			   	 memset(mixvalue[i],0,DIGEST_SIZE*2);

				 if(i==0)
				 {
			      	 	 // first json_elem is the node uuid value
			       		json_elem=json_get_first_child(json_root);
				 }
				 else
				 {
					json_elem=json_get_next_child(json_root);

				 }
				 ret=json_node_getname(json_elem,name);
			    	 if(ret<0)
					return ret;
				 if(json_get_type(json_elem)!=JSON_ELEM_STRING)
					return -EINVAL;

				 ret=json_node_getvalue(json_elem,tag,15);
				// tag is "RECORD"  means this elem defines an elem in the message's record
				 if(strcmp(tag,"RECORD")==0)
				 {
					void * record;	
					ret=message_read_elem(message,name,0,&record);
					if(ret<0)
						return ret;
				        int len=strlen(record);
					if(len<DIGEST_SIZE*2)
					{
						Memcpy(mixvalue[i],record,len);
					}
					else
					{
						Memcpy(mixvalue[i],record,DIGEST_SIZE*2);
					}
				}
				// tag is "name" means this elem defines remote proc's value
				else if(strcmp(tag,"NAME")==0)
				{
				        int len=strlen(name);
					if(len<DIGEST_SIZE*2)
					{
						Memcpy(mixvalue[i],name,len);
					}
					else
					{
						Memcpy(mixvalue[i],name,DIGEST_SIZE*2);
					}
				}

			    	else if(strncmp(tag,"EXPAND",6)==0)
			    	{
					char * type=tag+7;
			       		if(strlen(type)!=4)
					{
						return -EINVAL;	
					}		
					void * expand;
				        ret = message_get_define_expand(message,&expand,type);
					if(ret<0)
						return -EINVAL;
					if(expand==NULL)
						return -EINVAL;
					void * struct_template=load_record_template(type);
					if(struct_template==NULL)
						return -EINVAL;
					ret=struct_read_elem(name,expand,mixvalue[i],struct_template);		
					free_struct_template(struct_template);
					if(ret<0)
						return ret;
			    	}

		    		else
			    	{
			    		return -EINVAL;
			    	}
			  	if(i==0)
					continue;
			  	if(i==1)
			   	{
					comp_proc_uuid(mixvalue[0],mixvalue[1],uuid);
				  	continue;
			    	}
			   	comp_proc_uuid(uuid,mixvalue[i],uuid);
		
			  }
		   	 target=malloc(DIGEST_SIZE*2);
		 	 Memcpy(target,uuid,DIGEST_SIZE*2);
		   	 break;
		   }
*/		    
	    case ROUTE_TARGET_ERROR:
	    default:
		    return -EINVAL;
    }
    *result=target;
    return rule->target_type;	

}

int router_set_local_route(void * message,void * policy)
{
	int ret;
    	MSG_HEAD * msg_head;	
   	DISPATCH_POLICY * msg_policy=(DISPATCH_POLICY *)policy;
	ROUTE_RULE * rule;
	char * target;

    	if(policy==NULL)
        	return -EINVAL;
    	if(message==NULL)
        	return -EINVAL;
    	msg_head=message_get_head(message);
		
	Memset(msg_head->route,0,DIGEST_SIZE);
	Strncpy(msg_head->route,msg_policy->name,DIGEST_SIZE);
	msg_head->ljump=1;
	msg_head->flow=msg_policy->type;
	message_set_policy(message,policy);
	return 1;	
}

int router_set_query_end(void * message,void * policy)
{
	int ret;
    	MSG_HEAD * msg_head;	
   	DISPATCH_POLICY * msg_policy=(DISPATCH_POLICY *)policy;
	ROUTE_RULE * rule;
	char * target;
	int jumpcount=1;

    	if(policy==NULL)
        	return -EINVAL;
    	if(message==NULL)
        	return -EINVAL;
    	msg_head=message_get_head(message);
	message_set_policy(message,policy);
	ret=dispatch_policy_getfirstrouterule(policy,&rule);
	if(ret<0)
		return ret;
	while(rule!=NULL)
	{
		jumpcount++;
		if((rule->target_type==ROUTE_TARGET_CONN)||(rule->target_type==ROUTE_TARGET_NAME))	
			break;
		ret=dispatch_policy_getnextrouterule(policy,&rule);
	}

	if(rule==NULL)
		return -EINVAL;
	ret=dispatch_policy_getnextrouterule(policy,&rule);

	if(rule==NULL)
		msg_head->flow=MSG_FLOW_FINISH;
	else
	{
		// if next target is a remote target, change flow to DELIVER
		message_set_receiver(message,rule->target_name);
		if(rule->target_type!=ROUTE_TARGET_LOCAL)
		{
			msg_head->flow=MSG_FLOW_DELIVER;
			msg_head->flag &=~MSG_FLAG_RESPONSE;
		}
	}
	msg_head->ljump=jumpcount++;
	return 1;	
}

int router_find_route_policy(void * message,void **msg_policy,char * sender_proc)
{
    DISPATCH_POLICY * policy;
    int ret;
    *msg_policy=NULL;

    ret=dispatch_policy_getfirst(&policy);
    if(ret<0)
        return ret;
    while(policy!=NULL)
    {
	if((policy->state==POLICY_CLOSE)||(policy->state==POLICY_IGNORE))
	{
    		ret=dispatch_policy_getnext(&policy);
    		if(ret<0)
             		return ret;
		continue;
	}

    	ret=dispatch_match_sender(policy,sender_proc);
	if(ret>0)
	{	
		ret=dispatch_match_message_jump(policy,message);
		if(ret>0)
		{
       			ret=dispatch_match_message(policy,message);
        		if(ret<0)
            			return ret;
   	     		if(ret>0)
        		{
            			*msg_policy=policy;
            			return ret;
        		}
		}	
	}
    	ret=dispatch_policy_getnext(&policy);
    	if(ret<0)
             return ret;
    }
    return ret;
}
int router_dup_activemsg_info (void * message)
{
	int ret;
	struct message_box * msg_box=message;
	MSG_HEAD * msg_head;
	MSG_HEAD * new_msg_head;
	if(message==NULL)
		return -EINVAL;

	void * active_msg=message_get_activemsg(message);
	if(active_msg==NULL)
		return 0;
	if(active_msg==message)
	{
		msg_head=message_get_head(message);
	//	msg_head->ljump++;
		return 0;
	}
	
	msg_head=message_get_head(active_msg);
	new_msg_head=message_get_head(message);
	message_set_flow(msg_box,msg_head->flow);
	message_set_state(msg_box,msg_head->state);
//	message_set_flag(msg_box,msg_head->flag);
	message_set_route(msg_box,msg_head->route);
	new_msg_head->ljump=msg_head->ljump;	
	new_msg_head->rjump=msg_head->rjump;	

	MSG_EXPAND * old_expand;
	MSG_EXPAND * new_expand;
	void * flow_expand;
	ret=message_get_define_expand(active_msg,&old_expand,DTYPE_MSG_EXPAND,SUBTYPE_FLOW_TRACE);
	if(old_expand!=NULL) 
	{
		new_expand=Calloc0(sizeof(MSG_EXPAND));
		if(new_expand==NULL)
			return -ENOMEM;
		Memcpy(new_expand,old_expand,sizeof(MSG_EXPAND_HEAD));
		void * struct_template =memdb_get_template(DTYPE_MSG_EXPAND,SUBTYPE_FLOW_TRACE);
		new_expand->expand = clone_struct(old_expand->expand,struct_template);
		message_add_expand(message,new_expand);
	}
	ret=message_get_define_expand(active_msg,&old_expand,DTYPE_MSG_EXPAND,SUBTYPE_ASPECT_POINT);
	if(old_expand!=NULL) 

	{
		new_expand=Calloc0(sizeof(MSG_EXPAND));
		if(new_expand==NULL)
			return -ENOMEM;
		Memcpy(new_expand,old_expand,sizeof(MSG_EXPAND_HEAD));
		void * struct_template =memdb_get_template(DTYPE_MSG_EXPAND,SUBTYPE_ASPECT_POINT);
		new_expand->expand = clone_struct(old_expand->expand,struct_template);
		message_add_expand(message,new_expand);
	}
	ret=message_get_define_expand(active_msg,&old_expand,DTYPE_MSG_EXPAND,SUBTYPE_ROUTE_RECORD);
	if(old_expand!=NULL) 
	{
		new_expand=Calloc0(sizeof(MSG_EXPAND));
		if(new_expand==NULL)
			return -ENOMEM;
		Memcpy(new_expand,old_expand,sizeof(MSG_EXPAND_HEAD));
		void * struct_template =memdb_get_template(DTYPE_MSG_EXPAND,SUBTYPE_ROUTE_RECORD);
		new_expand->expand = clone_struct(old_expand->expand,struct_template);
		message_add_expand(message,new_expand);
	}
	message_set_policy(message,message_get_policy(active_msg));

	return 1;
}
/*
int __message_policy_init(void * policy)
{
    MESSAGE_POLICY * message_policy=(MESSAGE_POLICY *)policy;
    if(message_policy==NULL)
        return -EINVAL;
    message_policy->sender_proc=NULL;
    message_policy->match_policy_list=
            (ROUTE_POLICY_LIST *)malloc(sizeof(ROUTE_POLICY_LIST));
    if(message_policy->match_policy_list==NULL)
        return -EINVAL;

    message_policy->dup_route_rule =
            (ROUTE_POLICY_LIST *)malloc(sizeof(ROUTE_POLICY_LIST));
    if(message_policy->dup_route_rule==NULL)
        return -EINVAL;
    message_policy->main_route_rule =
            (ROUTE_RULE *)malloc(sizeof(ROUTE_RULE));
    if(message_policy->main_route_rule==NULL)
        return -EINVAL;
    message_policy->sender_proc=NULL;
    __route_policy_init(message_policy->match_policy_list);

    memset(message_policy->main_route_rule,0,sizeof(ROUTE_RULE));
    __route_policy_init(message_policy->dup_route_rule);
    return 0;
}
*/
#define MAX_LINE_LEN 1024
//int read_cfg_buffer(FILE * stream, char * buf, int size)
    /*  Read text data from config file,
     *  ignore the ^# line and remove the \n character
     *  stream: the config file stream
     *  buf: the buffer to store the cfg data
     *  size: read data size
     *
     *  return value: read data size,
     *  negative value if it has special error
     *  */
/*
{
    long offset=0;
    long curr_offset;
    char buffer[MAX_LINE_LEN];
    char * retptr;
    int len;

    while(offset<size)
    {
        curr_offset=ftell(stream);
        retptr=fgets(buffer,MAX_LINE_LEN,stream);

        // end of the file
        if(retptr==NULL)
            break;
        len=strlen(buffer);
        if(len==0)
            break;
        // commet line
        if(buffer[0]=='#')
            continue;
        while((buffer[len-1]=='\r')||(buffer[len-1]=='\n'))
        {
            len--;
            if(len==0)
                continue;
            buffer[len]==0;
        }
        // this line is too long to read
        if(len>size)
            return -EINVAL;

        // out of the bound
        if(len+offset>size)
        {
            fseek(stream,curr_offset,SEEK_SET);
            break;
        }
        memcpy(buf+offset,buffer,len);
        offset+=len;
	buf[offset]=0;
    }
    return offset;
}

int read_one_policy(void ** message_policy,void * json_node)
{
    void * match_rule_template=create_struct_template(&match_rule_desc);
    void * route_rule_template=create_struct_template(&route_rule_desc);
    void * match_policy_node;
    void * route_policy_node;
    void * match_rule_node;
    void * route_rule_node;
    void * temp_node;
    char buffer[1024];
    int ret;
    MATCH_RULE * temp_match_rule;
    ROUTE_RULE * temp_route_rule;

    MESSAGE_POLICY * policy=malloc(sizeof(MESSAGE_POLICY));
    if(policy==NULL)
        return -EINVAL;
    __message_policy_init(policy);

    // get the match policy json node
    match_policy_node=json_find_elem("MATCH_POLICY",json_node);
    if(match_policy_node==NULL)
        return -EINVAL;

    // get the match policy json node
    route_policy_node=json_find_elem("ROUTE_POLICY",json_node);
    if(route_policy_node==NULL)
        return -EINVAL;

    // read the match policy
    // first,read the sender proc value

    temp_node=json_find_elem("sender",match_policy_node);
    if(temp_node==NULL)
        policy->sender_proc=NULL;
    else
    {
        ret=get_json_value_from_node(temp_node,buffer,1024);
        if((ret<0) ||(ret>=1024))
            return -EINVAL;
        policy->sender_proc=dup_str(buffer,ret+1);
    }

    // second,read the match rule

    temp_node = json_find_elem("rules",match_policy_node);
    if(temp_node==NULL)
        return -EINVAL;

    match_rule_node=json_get_first_child(temp_node);
    while(match_rule_node!=NULL)
    {
        temp_match_rule=malloc(sizeof(MATCH_RULE));
        ret=json_2_struct(match_rule_node,temp_match_rule,match_rule_template);
        if(ret<0)
            return -EINVAL;
        __route_policy_add(policy->match_policy_list,temp_match_rule);
        match_rule_node=json_get_next_child(temp_node);
    }

    // read the route policy
    // first,read the main route policy

    route_rule_node=json_find_elem("main_policy",route_policy_node);
    if(route_rule_node==NULL)
        return -EINVAL;
    temp_route_rule=malloc(sizeof(ROUTE_RULE));
    ret=json_2_struct(route_rule_node,temp_route_rule,route_rule_template);
    if(ret<0)
        return -EINVAL;
    policy->main_route_rule=temp_route_rule;

    // second,read the dup rule

    temp_node = json_find_elem("dup_policy",route_policy_node);
    if(temp_node==NULL)
    {
        *message_policy=policy;
        return 0;
    }
    route_rule_node=json_get_first_child(temp_node);
    while(route_rule_node!=NULL)
    {
        temp_route_rule=malloc(sizeof(ROUTE_RULE));
        ret=json_2_struct(route_rule_node,temp_route_rule,route_rule_template);
        if(ret<0)
            return -EINVAL;
        __route_policy_add(policy->dup_route_rule,temp_route_rule);
        route_rule_node=json_get_next_child(temp_node);
    }
    *message_policy=policy;
    return 0;
}

int route_read_cfg(char * filename)
{
    const int bufsize=1024;
    char buffer[bufsize];
    void * root;
    int read_offset;
    int solve_offset;
    int buffer_left=0;
    int policy_num=0;
    void * policy;
    int ret;

    FILE * fp = fopen(filename,"r");
    if(fp==NULL)
        return -EINVAL;

    do {

        // when the file reading is not finished, we should read new data to the buffer
        if(fp != NULL)
        {
            read_offset=read_cfg_buffer(fp,buffer+buffer_left,bufsize-buffer_left);
            if(read_offset<0)
                return -EINVAL;
            else if(read_offset==0)
            {
                fclose(fp);
                fp=NULL;
            }
        }
        printf("policy %d is %s\n",policy_num+1,buffer);

        solve_offset=json_solve_str(&root,buffer);
        if(solve_offset<=0)
		break;
        ret=read_one_policy(&policy,root);

        if(ret<0)
		break;
        route_policy_add(policy);
        policy_num++;
	read_offset+=buffer_left;
        buffer_left=read_offset-solve_offset;
        if(buffer_left>0)
	{
            Memcpy(buffer,buffer+solve_offset,buffer_left);
	    buffer[buffer_left]=0;
	}
        else
        {
            if(fp==NULL)
                break;
        }
    }while(1);
    return policy_num;
}

*/
/*
int match_message_record(void * match_rule,void * message)
{
    int ret;
    MSG_HEAD * msg_head=get_message_head(message);
    if(msg_head==NULL)
        return -EINVAL;
    void * record_template = load_record_template(msg_head->record_type);
    if(record_template==NULL)
        return -EINVAL;
    MATCH_RULE * rule=(MATCH_RULE *)match_rule;
    char buffer[1024];
    void * record;
    ret=message_comp_elem_text(message,rule->seg,0,rule->value);
	
    return ret;
}

*/
int router_set_aspect_flow(void * message,void * policy)
{
	int ret;
    	MSG_HEAD * msg_head;	
   	DISPATCH_POLICY * msg_policy=(DISPATCH_POLICY *)policy;
	ROUTE_RULE * rule;
	char * target;

    	if(policy==NULL)
        	return -EINVAL;
    	if(message==NULL)
        	return -EINVAL;
    	msg_head=message_get_head(message);
		
	if(!(msg_head->flow&MSG_FLOW_ASPECT))
	{
		Memset(msg_head->route,0,DIGEST_SIZE);
		Strncpy(msg_head->route,msg_policy->newname,DIGEST_SIZE);
		msg_head->ljump=1;
		msg_head->rjump=1;
		msg_head->flow=msg_policy->type;
	}
	Memset(msg_head->receiver_uuid,0,DIGEST_SIZE);
	message_set_policy(message,policy);

	return 1;	
}

int router_set_dup_flow(void * message,void * policy)
{
	int ret;
    	MSG_HEAD * msg_head;	
   	DISPATCH_POLICY * msg_policy=(DISPATCH_POLICY *)policy;
	ROUTE_RULE * rule;
	char * target;

    	if(policy==NULL)
        	return -EINVAL;
    	if(message==NULL)
        	return -EINVAL;
    	msg_head=message_get_head(message);
		
	Memset(msg_head->route,0,DIGEST_SIZE);
	if(msg_policy->newname!=NULL)
		Strncpy(msg_head->route,msg_policy->newname,DIGEST_SIZE);
	msg_head->ljump=1;
	msg_head->rjump=0;
	msg_head->flow=MSG_FLOW_DELIVER;
//	msg_head->flag=msg_policy->flag;
	Memset(msg_head->receiver_uuid,0,DIGEST_SIZE);
	message_set_policy(message,policy);
	return 1;
}

int router_find_policy_byname(void **msg_policy,char * name,int rjump,int ljump)
{
    DISPATCH_POLICY * policy;
    int ret;
		
    *msg_policy=NULL;
    ret=dispatch_policy_getfirst(&policy);
    if(ret<0)
        return ret;
    while(policy!=NULL)
    {
	ret=Strncmp(name,policy->name,DIGEST_SIZE);
	if(ret==0)
	{
		if((policy->rjump==0) || (policy->rjump==rjump))
		{
			if((policy->ljump==0) || (policy->ljump==ljump))
			{
				*msg_policy=policy;		
				break;
			}
		}
	}
    	ret=dispatch_policy_getnext(&policy);
    }
    
    // find policy in router policy list;
    if(policy!=NULL)
	return ret;

    // if can't find policy in router policy list, look up it in aspect policy list	
    ret=dispatch_aspect_policy_getfirst(&policy);
    if(ret<0)
        return ret;
    while(policy!=NULL)
    {
	ret=Strncmp(name,policy->newname,DIGEST_SIZE);
	if(ret==0)
	{
		if((policy->rjump==0) || (policy->rjump==rjump))
		{
			if((policy->ljump==0) || (policy->ljump==ljump))
			{
				*msg_policy=policy;		
				break;
			}
		}
	}
    	ret=dispatch_aspect_policy_getnext(&policy);
    }

    return ret;
}


int router_find_aspect_policy(void * message,void **msg_policy,char * sender_proc)
{
    void * policy;
    int ret;
    *msg_policy=NULL;
    ret=dispatch_aspect_policy_getfirst(&policy);
    if(ret<0)
        return ret;
    while(policy!=NULL)
    {
	ret=dispatch_match_sender(policy,sender_proc);
	if(ret>0)
	{
		ret=dispatch_match_message_jump(policy,message);
		if(ret>0)
		{
        		ret=dispatch_match_message(policy,message);
       			if(ret<0)
            			return ret;
        		if(ret>0)
        		{
            			*msg_policy=policy;
            			return ret;
        		}
		}
	}
    	ret=dispatch_aspect_policy_getnext(&policy);
    	if(ret<0)
        	return ret;
    }
    return ret;
}
int router_find_aspect_policy_byname(void **msg_policy,char * name)
{
    DISPATCH_POLICY * policy;
    int ret;
		
    *msg_policy=NULL;
    ret=dispatch_policy_getfirst(&policy);
    if(ret<0)
        return ret;
    while(policy!=NULL)
    {
		
	if(policy->type==MSG_FLOW_ASPECT)
	{
		ret=Strncmp(name,policy->name,DIGEST_SIZE);
		if(ret==0)
		{
			*msg_policy=policy;		
			break;
		}
    		ret=dispatch_policy_getnext(&policy);
	}	
    }
    return ret;
}

int router_set_next_jump(void * message)
{
	int ret;
    	MSG_HEAD * msg_head;	
   	DISPATCH_POLICY * msg_policy;
	ROUTE_RULE * rule;
	char * target;
	int i;

    	if(message==NULL)
        	return -EINVAL;
    	msg_head=message_get_head(message);
		
	msg_policy=message_get_policy(message);
	if(msg_policy==NULL)
		return 0;	

	ret=dispatch_policy_getfirstrouterule(msg_policy,&rule);
	if(rule==NULL)
		return 0;
	for(i=1;i<msg_head->ljump;i++)
	{
		ret=dispatch_policy_getnextrouterule(msg_policy,&rule);		
		if(rule==NULL)
			return 0;
	}

	Memset(msg_head->receiver_uuid,0,DIGEST_SIZE);

	switch(rule->target_type)
	{
		case ROUTE_TARGET_LOCAL:
		case ROUTE_TARGET_PORT:
			ret=rule_get_target(rule,message,&target);
			if(ret<0)
				return ret;		
			Strncpy(msg_head->receiver_uuid,target,DIGEST_SIZE);
			msg_head->flag |=MSG_FLAG_LOCAL;
			Free(target);
			//message_set_state(message,MSG_FLOW_LOCAL);
			break;
		case ROUTE_TARGET_NAME:
		case ROUTE_TARGET_RECORD:
		case ROUTE_TARGET_EXPAND:
			ret=rule_get_target(rule,message,&target);
			if(ret<0)
				return ret;		
			if(Isstrinuuid(target))
			{
//				msg_head->receiver_uuid[0]='@';
				Strncpy(msg_head->receiver_uuid,target,DIGEST_SIZE/4*3);
			}
			else
			{
				Memcpy(msg_head->receiver_uuid,target,DIGEST_SIZE);
			}	
			Free(target);
			message_set_state(message,MSG_FLOW_DELIVER);
			msg_head->flag&=~MSG_FLAG_LOCAL;
//			msg_head->rjump++;
			break;
		case ROUTE_TARGET_CONN:
			ret=rule_get_target(rule,message,&target);
			if(ret<0)
				return ret;		
			Strncpy(msg_head->receiver_uuid,target,DIGEST_SIZE-1);
			Free(target);
			message_set_state(message,MSG_FLOW_DELIVER);
			msg_head->flag&=~MSG_FLAG_LOCAL;
//			msg_head->rjump++;
			break;
		default:
			return -EINVAL;
	}
	return 1;	
}

int route_push_site_str(void * message,char * name)
{
	int len;
	int ret;
	BYTE buffer[DIGEST_SIZE];
	Memset(buffer,0,DIGEST_SIZE);
	len=Strnlen(name,DIGEST_SIZE*2);
	if(len==DIGEST_SIZE*2)
	{
		ret=digest_to_uuid(name,buffer);
		if(ret<0)
			return ret;
	}
	else
	{
		if(len>DIGEST_SIZE/4*3)
			return -EINVAL;
		Strncpy(buffer,name,DIGEST_SIZE/4*3);
	}
	return route_push_site(message,buffer);	
}

int route_push_site(void * message,BYTE * name)
{
	MSG_EXPAND * expand;
	struct expand_flow_trace * flow_trace;
	int ret;
	int len;
	int index;

	index=message_get_define_expand(message,&expand,DTYPE_MSG_EXPAND,SUBTYPE_FLOW_TRACE);
	if(index<0)
		return -EINVAL;
	if(expand==NULL)
	{
		expand=Dalloc0(sizeof(MSG_EXPAND),message);
		if(expand==NULL)
			return -ENOMEM;	
		expand->type=DTYPE_MSG_EXPAND;
		expand->subtype=SUBTYPE_FLOW_TRACE;
		flow_trace=Dalloc0(sizeof(*flow_trace),message);
		if(flow_trace==NULL)
			return -ENOMEM;	
		expand->expand=flow_trace;
	}
	else
	{
		flow_trace=expand->expand;
	}
		
	if(flow_trace->record_num==0)
	{
		flow_trace->record_num++;
		flow_trace->trace_record=Dalloc0(DIGEST_SIZE,message);
		if(flow_trace->trace_record==NULL)
			return -ENOMEM;
		Memcpy(flow_trace->trace_record,name,DIGEST_SIZE);
		message_add_expand(message,expand);	
	}
	else
	{
		int curr_offset=flow_trace->record_num*DIGEST_SIZE;
		flow_trace->record_num++;
		char * buffer;
		buffer=Dalloc0(curr_offset+DIGEST_SIZE,message);
		if(buffer==NULL)
			return -ENOMEM;
		Memcpy(buffer,flow_trace->trace_record,curr_offset);
		Free(flow_trace->trace_record);
		Memcpy(buffer+curr_offset,name,DIGEST_SIZE);
		flow_trace->trace_record=buffer;
		message_replace_define_expand(message,expand);
	}
	return 0;
}

int route_push_aspect_site(void * message,char * proc,char * point)
{
	MSG_EXPAND * expand;
	struct expand_aspect_point * aspect_point;
	int ret;
	int len;
	int index;

	index=message_get_define_expand(message,&expand,DTYPE_MSG_EXPAND,SUBTYPE_ASPECT_POINT);
	if(index<0)
		return -EINVAL;
	if(expand==NULL)
	{
		expand=Calloc0(sizeof(MSG_EXPAND));
		if(expand==NULL)
			return -ENOMEM;	
		expand->type=DTYPE_MSG_EXPAND;
		expand->subtype=SUBTYPE_ASPECT_POINT;
		aspect_point=Dalloc0(sizeof(*aspect_point),message);
		if(aspect_point==NULL)
			return -ENOMEM;	
		expand->expand=aspect_point;
	}
	else
	{
		aspect_point=expand->expand;
	}
		
	if(aspect_point->record_num==0)
	{
		aspect_point->record_num++;
		aspect_point->aspect_proc=Dalloc0(DIGEST_SIZE,message);
		if(aspect_point->aspect_proc==NULL)
			return -ENOMEM;
		Memcpy(aspect_point->aspect_proc,proc,DIGEST_SIZE);
		aspect_point->aspect_point=Dalloc0(DIGEST_SIZE,message);
		if(aspect_point->aspect_point==NULL)
			return -ENOMEM;
		Memcpy(aspect_point->aspect_point,point,DIGEST_SIZE);
		message_add_expand(message,expand);	
	}
	else
	{
		int curr_offset=aspect_point->record_num*DIGEST_SIZE;
		aspect_point->record_num++;
		char * buffer;
		buffer=Dalloc0(curr_offset+DIGEST_SIZE,message);
		if(buffer==NULL)
			return -ENOMEM;
		Memcpy(buffer,aspect_point->aspect_proc,curr_offset);
		Free(aspect_point->aspect_proc);
		Memcpy(buffer+curr_offset,proc,DIGEST_SIZE);
		aspect_point->aspect_proc=buffer;

		buffer=Dalloc0(curr_offset+DIGEST_SIZE,message);
		if(buffer==NULL)
			return -ENOMEM;
		Memcpy(buffer,aspect_point->aspect_point,curr_offset);
		Free(aspect_point->aspect_point);
		Memcpy(buffer+curr_offset,point,DIGEST_SIZE);
		aspect_point->aspect_point=buffer;
		message_replace_define_expand(message,expand);
	}
	return 0;
}

int route_check_sitestack(void * message)
{
	MSG_EXPAND * expand;
	struct expand_flow_trace * flow_trace;
	int ret;
	ret=message_get_define_expand(message,&expand,DTYPE_MSG_EXPAND,SUBTYPE_FLOW_TRACE);
	if(ret<0)
		return -EINVAL;
	if(expand==NULL)
		return 0;
	flow_trace=expand->expand;
	return flow_trace->record_num;
}
int route_check_aspectstack(void * message)
{
	MSG_EXPAND * expand;
	struct expand_aspect_point * aspect_point;
	int ret;
	ret=message_get_define_expand(message,&expand,DTYPE_MSG_EXPAND,SUBTYPE_ASPECT_POINT);
	if(ret<0)
		return -EINVAL;
	if(expand==NULL)
		return 0;
	aspect_point=expand->expand;
	if(aspect_point==NULL)
		return 0;
	return aspect_point->record_num;
}

int router_pop_site(void * message)
{
	MSG_EXPAND * expand;
	struct expand_flow_trace * flow_trace;
	int ret;
	int len;

	MSG_HEAD * msg_head =message_get_head(message);

	ret=message_get_define_expand(message,&expand,DTYPE_MSG_EXPAND,SUBTYPE_FLOW_TRACE);
	if(ret<0)
		return -EINVAL;
	if(expand==NULL)
		return 0;
	flow_trace=expand->expand;
	if(flow_trace->record_num<1)
		return 0;

	int curr_offset=(--(flow_trace->record_num))*DIGEST_SIZE;
	Memcpy(msg_head->receiver_uuid,flow_trace->trace_record+curr_offset,DIGEST_SIZE);

	if(flow_trace->record_num==0)
	{
		Free(flow_trace->trace_record);
		flow_trace->trace_record=NULL;
		message_remove_expand(message,DTYPE_MSG_EXPAND,SUBTYPE_FLOW_TRACE,&expand);
		Free(expand);
	}
		
//	{
//		msg_head->state=MSG_FLOW_DELIVER;
//		msg_head->flag |= MSG_FLAG_LOCAL;
//	}

	return 1;
}

int router_pop_aspect_site(void * message, char * proc)
{
	MSG_EXPAND * expand;
	struct expand_aspect_point * aspect_point;
	int ret;
	int len;

	MSG_HEAD * msg_head =message_get_head(message);

	ret=message_get_define_expand(message,&expand,DTYPE_MSG_EXPAND,SUBTYPE_ASPECT_POINT);
	if(ret<0)
		return -EINVAL;
	if(expand==NULL)
		return 0;
	aspect_point=expand->expand;
	if(aspect_point->record_num<1)
		return 0;

	int curr_offset=(--(aspect_point->record_num))*DIGEST_SIZE;
	Memcpy(msg_head->receiver_uuid,aspect_point->aspect_point+curr_offset,DIGEST_SIZE);
	Memcpy(proc,aspect_point->aspect_proc+curr_offset,DIGEST_SIZE);

	if(aspect_point->record_num==0)
	{
		message_remove_expand(message,DTYPE_MSG_EXPAND,SUBTYPE_ASPECT_POINT,&expand);
		Free(expand);
		Free(aspect_point->aspect_proc);
		Free(aspect_point->aspect_point);
	}
	return 1;
}

int router_store_route(void * message)
{
	int ret;
	MSG_HEAD * msg_head;
	struct expand_route_record * route_record;
	if(message==NULL)
		return  -EINVAL;
	msg_head=(MSG_HEAD *)message_get_head(message);	
	route_record=Dalloc0(sizeof(*route_record),message);
	if(route_record==NULL)
		return -ENOMEM;	

	Memcpy(route_record->sender_uuid,msg_head->sender_uuid,DIGEST_SIZE);
	Memcpy(route_record->receiver_uuid,msg_head->receiver_uuid,DIGEST_SIZE);
	Memcpy(route_record->route,msg_head->route,DIGEST_SIZE);
	route_record->flow=msg_head->flow;
	route_record->state=msg_head->state;
	route_record->flag=msg_head->flag;
	route_record->ljump=msg_head->ljump;
	route_record->rjump=msg_head->rjump;
		
	ret=message_add_expand_data(message,DTYPE_MSG_EXPAND,SUBTYPE_ROUTE_RECORD,route_record);
	return ret;
}
int route_recover_route(void * message)
{
	int ret;
	MSG_HEAD * msg_head;
	MSG_EXPAND * expand;
	struct expand_route_record * route_record=NULL;
	if(message==NULL)
		return  -EINVAL;
	ret=message_get_define_expand(message,&expand,DTYPE_MSG_EXPAND,SUBTYPE_ROUTE_RECORD);
	if(ret<0)
		return ret;
	if(expand==NULL)
		return -EINVAL;
	route_record=expand->expand;
	if(route_record==NULL)
		return -EINVAL;	

	int temp_flag=0;
	msg_head=(MSG_HEAD *)message_get_head(message);	

	Memcpy(msg_head->sender_uuid,route_record->sender_uuid,DIGEST_SIZE);
	Memcpy(msg_head->receiver_uuid,route_record->receiver_uuid,DIGEST_SIZE);
	Memcpy(msg_head->route,route_record->route,DIGEST_SIZE);
	msg_head->flow=route_record->flow;
	msg_head->state=route_record->state;
	msg_head->ljump=route_record->ljump;
	msg_head->rjump=route_record->rjump;

	int temp_mask=MSG_FLAG_CRYPT|MSG_FLAG_SIGN|MSG_FLAG_ZIP|MSG_FLAG_VERIFY;

	msg_head->flag = (msg_head->flag &temp_mask) | (route_record->flag & (~temp_mask));

	message_remove_expand(message,DTYPE_MSG_EXPAND,SUBTYPE_ROUTE_RECORD,&route_record);
	Free(route_record);
	return 1;
}
