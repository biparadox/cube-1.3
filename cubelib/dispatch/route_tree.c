#include <errno.h>

#include "../include/data_type.h"
#include "../include/list.h"
#include "../include/attrlist.h"
#include "../include/alloc.h"
#include "../include/attrlist.h"
#include "../include/memfunc.h"
#include "../include/json.h"
#include "../include/struct_deal.h"
#include "../include/basefunc.h"
#include "../include/memdb.h"
#include "../include/message.h"
#include "message_box.h"
#include "../include/dispatch.h"

#include "route_tree.h"

const int hash_order=8;
static NODE_LIST route_forest;
static NODE_LIST aspect_forest;
static NODE_LIST hash_forest[256];
static NODE_LIST waiting_message_list;

static void * policy_head_template;
static void * match_rule_template;
static void * route_rule_template;

static int match_flag = 0x10000000;

void * route_read_policy(void * policy_node);
int  route_set_path(ROUTE_PATH * path, void * policy);
int dispatch_policy_addmatchrule(void * path,MATCH_RULE * rule);
int dispatch_policy_addrouterule(void * path,ROUTE_RULE * rule);
int dispatch_policy_getnextmatchrule(void * path,MATCH_RULE ** rule);
int dispatch_policy_getnextrouterule(void * path,ROUTE_RULE ** rule);
int route_tree_addresponserule(void * path,ROUTE_RULE * rule);

static inline unsigned int _hash_index(char * uuid)
{
	int ret = *((unsigned int *)uuid);
	return ret&(0xffff>>(16-hash_order));
}
int tracenode_comp_uuid(void * List_head, void * uuid)
{
        struct List_head * head;
        TRACE_NODE * trace_node;
        head=(struct List_head *)List_head;
        if(head==NULL)
                return -EINVAL;
        Record_List * record;
        record = List_entry(head,Record_List,list);
        trace_node = (TRACE_NODE *) record->record;
        if(trace_node == NULL)
                return -EINVAL;
        return Memcmp(trace_node->msg_uuid,uuid,DIGEST_SIZE);
}

static inline int _init_node_list (void * list)
{
    	NODE_LIST * node_list=(NODE_LIST *)list;
   	Memset(node_list,0,sizeof(NODE_LIST));
   	INIT_LIST_HEAD(&(node_list->head.list));
   	node_list->head.record=NULL;
	node_list->policy_num=0;
	return 0;

}

Record_List * _node_list_add(void * list,void * record)
{
	Record_List * recordhead;
	Record_List * newrecord;
    	NODE_LIST * node_list=(NODE_LIST *)list;

   	recordhead = &(node_list->head);
	if(recordhead==NULL)
		return NULL;

	newrecord=Dalloc0(sizeof(Record_List),NULL);
	if(newrecord==NULL)
		return NULL;
	INIT_LIST_HEAD(&(newrecord->list));
	newrecord->record=record;
	List_add_tail(&(newrecord->list),&(recordhead->list));
	node_list->policy_num++;
	return newrecord;
}

void _node_list_del(void * record)
{
	Record_List * recordhead=record;
	List_del(&(recordhead->list));
	return;
}

Record_List * _node_list_getfirst(void * list)
{
	NODE_LIST * node_list=list;
	node_list->curr = &(node_list->head.list.next);

	Record_List * record; 

    record=List_entry(node_list->curr,Record_List,list);
	return record;	
}

Record_List * _node_list_getnext(void * list)
{
	Record_List * record; 
	NODE_LIST * node_list=list;
	struct List_head * curr = (struct List_head *)node_list->curr;

	if(curr->next == &(node_list->head.list.next))
	{
		node_list->curr=NULL;
		return NULL;
	}
	node_list->curr=curr->next;
    record=List_entry(node_list->curr,Record_List,list);
	return record;	
}

static inline int _init_router_forest(void * list)
{
    	NODE_LIST * forest=(NODE_LIST *)list;
   	Memset(forest,0,sizeof(NODE_LIST));
   	INIT_LIST_HEAD(&(forest->head.list));
   	forest->head.record=NULL;
	forest->policy_num=0;
	return 0;
}
static inline int _init_hash_forest()
{
	int i;
	for(i=0;i<1<<hash_order;i++)
	{
    		NODE_LIST * forest=&hash_forest[i];
   		Memset(forest,0,sizeof(NODE_LIST));
   		INIT_LIST_HEAD(&(forest->head.list));
   		forest->head.record=NULL;
		forest->policy_num=0;
	}
	return 0;
}

int router_tree_init( )
{
	int ret;
	ret=_init_node_list(&route_forest);
	if(ret<0)
		return ret;
	ret=_init_node_list(&aspect_forest);
	if(ret<0)
		return ret;
	ret=_init_node_list(&waiting_message_list);
	if(ret<0)
		return ret;
	ret=_init_hash_forest();
	if(ret<0)
		return ret;
    	policy_head_template=memdb_get_template(TYPE_PAIR(DISPATCH,POLICY_HEAD));
    	match_rule_template=memdb_get_template(TYPE_PAIR(DISPATCH,MATCH_HEAD));
    	route_rule_template=memdb_get_template(TYPE_PAIR(DISPATCH,ROUTE_RULE));
	return 0;
}

char * route_path_getname(void * path)
{
    ROUTE_PATH * route_path=path;
    char * tempname;
    int namelen;
    if(path==NULL)
	    return NULL;
    namelen=Strnlen(route_path->name,DIGEST_SIZE);
	
    tempname=Talloc0(namelen+1);
    Strncpy(tempname,route_path->name,namelen);
    return tempname;
}

void * route_path_create()
{
	ROUTE_PATH * path;
	path=Dalloc0(sizeof(ROUTE_PATH),NULL);
	if(path==NULL)
		return NULL;
	_init_node_list(&path->match_list);
	_init_node_list(&path->route_path);
	_init_node_list(&path->response_path);
	return path;
}

void * route_node_create(ROUTE_RULE * rule)
{
	ROUTE_NODE * node;
	int ret;
	node=Dalloc0(sizeof(ROUTE_NODE),NULL);
	if(node==NULL)
		return NULL;
	if(rule!=NULL)
	{
		ret=struct_clone(rule,&node->this_target,route_rule_template);
		if(ret<0)
			return NULL;
	}

	_init_node_list(&node->aspect_branch);
	return node;
}

int read_policy_from_buffer(char * buffer, int max_len)
{
	int offset=0;
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

        policy=route_read_policy(root);
		if(policy==NULL)
			break;
		if(route_add_policy(policy)<0)
			return -EINVAL;
		count++;
	}
	return count;
}

int route_get_node_byjump(ROUTE_PATH * path, ROUTE_NODE ** node,int ljump, int rjump)
{
	NODE_LIST * node_path;
	struct List_head * curr;
	int i;
	if(rjump>=0)
	{
		node_path=&(path->route_path);
	}
	else
	{
		node_path=&(path->response_path);
	}
	if(ljump==0)
		ljump=1;
	
	if(node_path->policy_num<ljump)
	{
		node=NULL;
		return 0;
	}
	
	curr = &(node_path->head.list);

	for(i=0;i<ljump;i++)
	{
		curr=curr->next;
	}
	
	Record_List * trace_record; 

    trace_record=List_entry(curr,Record_List,list);
	*node=(ROUTE_NODE *)trace_record->record;
	if(*node!=NULL)
		return 1;
	return 0;	
}

void * route_read_policy(void * policy_node)
{
    void * match_policy_node;
    void * route_policy_node;
    void * match_rule_node;
    void * route_rule_node;
    void * temp_node;
	Record_List * record;
//    char buffer[1024];
//    char * temp_str;
    int ret;
    MATCH_RULE * temp_match_rule;
    ROUTE_RULE * temp_route_rule;

	ROUTE_PATH * normal_path;
	ROUTE_NODE * insert_node;
	BRANCH_NODE * branch_node;

    RECORD(DISPATCH,POLICY_HEAD) * policy;	

    ROUTE_PATH  * path =route_path_create();
    if(path==NULL)
        return NULL;

    temp_node=json_find_elem("policy_head",policy_node);
    if(temp_node==NULL)
    {
	return NULL;
    }

    policy=Talloc0(sizeof(*policy));
    if(policy==NULL)
	return NULL;

    // read policy head

    ret=json_2_struct(temp_node,policy,policy_head_template);
    if(ret<0)
        return NULL;

    // set policy head to path	
	// for DUP and ASPECT policy, find old path and insert branch

    switch(path->flow)
	{
		case MSG_FLOW_DUP:
			
			ret=route_find_policy_byname(&normal_path,policy->name,policy->rjump,policy->ljump);
			if(ret<0)
				return ret;
			if(normal_path==NULL)
			{
				print_cubeerr("can't find DUP policy's dup path!\n");
				return 0;
			}
			ret = route_get_node_byjump(normal_path, &insert_node,policy->ljump,1);
			if(ret<0)
				return ret;
			if(insert_node==NULL)
			{
				print_cubeerr("can't find DUP policy's insert node!\n");
				return 0;
			}
			branch_node = Talloc0(sizeof(*branch_node));
			if(branch_node == NULL)
				return -ENOMEM;
			branch_node->route_path = normal_path;
			branch_node->branch_type =policy->type;  
			branch_node->branch_path=path;
			
     		if((record=_node_list_add(&insert_node->aspect_branch,(void *)branch_node))==NULL)
			{
				print_cubeerr("insert DUP policy failed!\n");
				return 0;
			}

    		Strncpy(path->name,policy->newname,DIGEST_SIZE);
			path->flow=MSG_FLOW_DELIVER;
			break;
		case MSG_FLOW_ASPECT:
    		Strncpy(path->name,policy->newname,DIGEST_SIZE);
			path->flow=MSG_FLOW_QUERY;
			break;
		default:	
    		Strncpy(path->name,policy->name,DIGEST_SIZE);
    		Strncpy(path->sender,policy->sender,DIGEST_SIZE);
    		path->flow = policy->type;
	}


    path->ljump=policy->ljump;
    path->rjump=policy->rjump; 
    path->state=policy->state; 

    // get the match policy json node
    match_policy_node=json_find_elem("MATCH_RULES",policy_node);
    if(match_policy_node==NULL)
        return NULL;
  
    // get the route policy json node
    route_policy_node=json_find_elem("ROUTE_RULES",policy_node);

    if(route_policy_node==NULL)
        return NULL;

    // read the match rule

    match_rule_node=json_get_first_child(match_policy_node);
    while(match_rule_node!=NULL)
    {
	void * record_template;
        temp_match_rule=Dalloc0(sizeof(MATCH_RULE),path);
	if(temp_match_rule==NULL)
		return NULL;
        ret=json_2_struct(match_rule_node,temp_match_rule,match_rule_template);
        if(ret<0)
            return NULL;
	temp_node=json_find_elem("value",match_rule_node);
	if(temp_node!=NULL)
	{
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
	ret=dispatch_policy_addmatchrule(path,temp_match_rule);
	if(ret<0)
		return NULL;	
	
        match_rule_node=json_get_next_child(match_policy_node);
    }

    // read the route policy
    // first,read the main route policy

    temp_node=json_find_elem("main_policy",route_policy_node);
    if(temp_node==NULL)
	return NULL;	

    route_rule_node=json_get_first_child(temp_node);
    while(route_rule_node!=NULL)
    {
    	temp_route_rule=Dalloc0(sizeof(ROUTE_RULE),path);
    	if(temp_route_rule==NULL)
		return NULL;
    	ret=json_2_struct(route_rule_node,temp_route_rule,route_rule_template);
    	if(ret<0)
        	return NULL;
	ret=dispatch_policy_addrouterule(path,temp_route_rule);
	if(ret<0)
		return NULL;	
        route_rule_node=json_get_next_child(temp_node);
    } 

    // next,read the response route policy

    temp_node=json_find_elem("response_policy",route_policy_node);
    if(temp_node!=NULL)
    {	

    	route_rule_node=json_get_first_child(temp_node);
    	while(route_rule_node!=NULL)
    	{
    		temp_route_rule=Dalloc0(sizeof(ROUTE_RULE),path);
    		if(temp_route_rule==NULL)
			return NULL;
    		ret=json_2_struct(route_rule_node,temp_route_rule,route_rule_template);
    		if(ret<0)
        		return NULL;
		ret = route_tree_addresponserule(path,temp_route_rule);
		if(ret<0)
			return NULL;	
        	route_rule_node=json_get_next_child(temp_node);
    	}
    }		 

    return path;
}

int _dispatch_policy_getfirst(void * policy_list,void ** policy)
{
	Record_List * recordhead;
	Record_List * newrecord;
        NODE_LIST * dispatch_policy=(NODE_LIST *)policy_list;

        recordhead = &(dispatch_policy->head);
	if(recordhead==NULL)
		return -EINVAL;

	newrecord = (Record_List *)(recordhead->list.next);
	*policy=newrecord->record;
        dispatch_policy->curr=newrecord;
	return 0;
}

int _route_match_aspect_branch(void * message,BRANCH_NODE * aspect_branch)
{
	int ret;
	struct message_box * msg_box = (struct message_box *)message;

	ret = _route_match_rule(message,aspect_branch->branch_path);
	return ret;
}

void * route_dup_message(void * message,BRANCH_NODE * branch)
{
	struct message_box * msg_box=(struct message_box *)message;
	if(branch->branch_type != MSG_FLOW_DUP)
		return -EINVAL;	
	ROUTE_PATH * branch_path = branch->branch_path;
	struct message_box * new_msg = message_clone(message);
	if(new_msg==NULL)
		return -EINVAL;
	new_msg->head.ljump=1;
	new_msg->head.rjump=1;
	Strncpy(new_msg->head.route,branch_path->name,DIGEST_SIZE);
	Strncpy(new_msg->head.sender_uuid,msg_box->head.sender_uuid,DIGEST_SIZE);
	message_route_setstart(new_msg,branch_path);
	return new_msg;
}

int dispatch_policy_getfirst(void ** policy)
{
    return _dispatch_policy_getfirst(&route_forest,policy);
}

int dispatch_policy_getfirstmatchrule(void * path,MATCH_RULE ** rule)
{
    ROUTE_PATH * route_path=(ROUTE_PATH *)path;
    return _dispatch_policy_getfirst(&route_path->match_list,(void **)rule);
}

int dispatch_policy_getfirstrouterule(void * path,ROUTE_RULE ** rule)
{
    ROUTE_PATH * route_path=(ROUTE_PATH *)path;
    return _dispatch_policy_getfirst(&route_path->route_path,(void **)rule);
}

int _dispatch_policy_getnext(void * policy_list,void ** policy)
{
	Record_List * recordhead;
	Record_List * newrecord;
        NODE_LIST * dispatch_policy=(NODE_LIST *)policy_list;

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
    return _dispatch_policy_getnext(&route_forest,policy);
}

int dispatch_policy_getnextmatchrule(void * path,MATCH_RULE ** rule)
{
    ROUTE_PATH  * route_path=(ROUTE_PATH *)path;
    return _dispatch_policy_getnext(&route_path->match_list,(void **)rule);
}

int dispatch_policy_getnextrouterule(void * path,ROUTE_RULE ** rule)
{
    ROUTE_PATH  * route_path=(ROUTE_PATH *)path;
    return _dispatch_policy_getnext(&route_path->route_path,(void **)rule);
}


int dispatch_policy_addmatchrule(void * path,MATCH_RULE * rule)
{
    ROUTE_PATH  * route_path=(ROUTE_PATH *)path;
    if(_node_list_add(&route_path->match_list,(void *)rule)==NULL)
	return 0;
    return 1;
}

int dispatch_policy_addrouterule(void * path,ROUTE_RULE * rule)
{
     ROUTE_PATH  * route_path=(ROUTE_PATH *)path;
     ROUTE_NODE  * route_node;
     Record_List * record;
     if(rule==NULL)
	  return 0; 	
     route_node =route_node_create(rule);
     if(route_node==NULL)
	return -EINVAL;

     route_node->route_path=path;
     if((record=_node_list_add(&route_path->route_path,(void *)route_node))==NULL)
		return 0;
     route_node->chain=record;
     return 1;
}

int route_tree_addresponserule(void * path,ROUTE_RULE * rule)
{
     ROUTE_PATH  * route_path=(ROUTE_PATH *)path;
     ROUTE_NODE  * route_node;
     Record_List * record;
     if(rule==NULL)
	  return 0; 	
     route_node =route_node_create(rule);
     if(route_node==NULL)
	return -EINVAL;

     route_node->route_path=path;
     if((record=_node_list_add(&route_path->response_path,(void *)route_node))==NULL)
	return 0;
     route_node->chain=record;
     return 1;
}

int _waiting_message_add(void * message)
{
    if(_node_list_add(&waiting_message_list,message)==NULL)
	return 0;
    return 1;
}
void _waiting_message_del(void * record)
{
	_node_list_del(record);
	waiting_message_list.policy_num--;
}
void * _waiting_message_getfirst()
{
	Record_List * msg_record =(Record_List *) waiting_message_list.head.list.next;
	waiting_message_list.curr = msg_record;
	if(msg_record==NULL)
		return NULL;
	return msg_record->record;
} 
void * _waiting_message_removehead()
{
	void * message;
	if(waiting_message_list.policy_num ==0)
		return NULL;
	Record_List * msg_record =(Record_List *) waiting_message_list.head.list.next;
	waiting_message_list.curr = msg_record;
	if(msg_record==NULL)
		return NULL;
	_waiting_message_del(msg_record);
	message=msg_record->record;
	Free(msg_record);
	return message;
} 

void * _waiting_message_getnext()
{
	Record_List * msg_record =(Record_List *) waiting_message_list.curr;
	if(msg_record==NULL)
		return NULL;
	waiting_message_list.curr=msg_record->list.next;
	// remove the curr waiting message
	_waiting_message_del(msg_record);

	msg_record =(Record_List *) waiting_message_list.curr;

	if(msg_record==NULL)
		return NULL;
	return msg_record->record;
}

int _route_add_policy(void * list,void * policy)
{
    if(_node_list_add(list,policy)==NULL)
	return 0;
    return 1;
}

int route_add_policy(void * policy)
{
      return _node_list_add(&route_forest,policy);
}
int route_path_getstate(void * path)
{
    ROUTE_PATH * route_path=path;
    if(path==NULL)
	    return -EINVAL;
    return route_path->state;
}


int route_match_sender(void * path, char * sender)
{
	int ret;
	ROUTE_PATH * route_path=(ROUTE_PATH *)path;
	if(path==NULL)
		return 0;
	ret=Strncmp(route_path->sender,sender,DIGEST_SIZE);
	if(ret!=0)
		return 0;
	return 1;
}

int route_match_message_jump(void * path, void * message)
{
	int ret;
	MSG_HEAD * msg_head;
	ROUTE_PATH * route_path=(ROUTE_PATH *)path;
	if(path==NULL)
		return 0;
	msg_head=message_get_head(message);
	if((route_path->rjump==0)||(route_path->rjump==msg_head->rjump))
		if((route_path->ljump==0)||(route_path->ljump==msg_head->ljump))
		{
			return 1;
		}
	return 0;
}

int _route_match_rules(void * path,void * message)
{
	int ret;
	int result=0;
	int prev_result=0;
	int match_rule_num=0;
	ROUTE_PATH * route_path=(ROUTE_PATH *)path;
	MATCH_RULE * match_rule;
	MSG_HEAD * msg_head;
	void * msg_record;
	void * record_template;
	msg_head=message_get_head(message);
	if(msg_head==NULL)
		return -EINVAL;
	ret=dispatch_policy_getfirstmatchrule(path,&match_rule);
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
		ret=dispatch_policy_getnextmatchrule(path,&match_rule);
		if(ret<0)
			return ret;
	}	
	if(match_rule_num==0)
		return 0;
	return prev_result;
}

int route_match_message(void * path,void * message)
{
	int ret;
	int result=0;
	int prev_result=0;
	int match_rule_num=0;
	ROUTE_PATH * route_path=(ROUTE_PATH *)path;
	MATCH_RULE * match_rule;
	MSG_HEAD * msg_head;
	void * msg_record;
	void * record_template;
	
	msg_head=message_get_head(message);
	if(msg_head==NULL)
		return -EINVAL;

	if(route_path->name!=NULL)
	{
		if(msg_head->route[0]!=0)
			if(Strncmp(route_path->name,msg_head->route,DIGEST_SIZE)!=0)
				return 0;	
	}

	ret=dispatch_policy_getfirstmatchrule(path,&match_rule);
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
		ret=dispatch_policy_getnextmatchrule(path,&match_rule);
		if(ret<0)
			return ret;
	}	
	if(match_rule_num==0)
		return 0;
	return prev_result;
}

ROUTE_NODE * get_curr_pathsite(void * msg)
{
	struct message_box * msg_box = (struct message_box *)msg;
	Record_List * curr_pathrecord = (Record_List *)msg_box->path_site;
	if(curr_pathrecord==NULL)
		return NULL;
	return (ROUTE_NODE *) curr_pathrecord->record;

}

ROUTE_NODE * set_next_pathsite(void * msg)
{
	struct message_box * msg_box = (struct message_box *)msg;
	Record_List * curr_pathrecord = (Record_List *)msg_box->path_site;


	ROUTE_PATH * route_path = (ROUTE_PATH *)msg_box->policy;
	if(curr_pathrecord==NULL)
		return NULL;
	Record_List * next_pathrecord = (Record_List *)curr_pathrecord->list.next;
	if(next_pathrecord == &route_path->route_path.head.list)
	{
		msg_box->path_site=NULL;
		return NULL;
	}

	msg_box->path_site=next_pathrecord;
	return (ROUTE_NODE *) next_pathrecord->record;
}

ROUTE_NODE * get_next_pathsite(void * msg)
{
	struct message_box * msg_box = (struct message_box *)msg;
	Record_List * curr_pathrecord = (Record_List *)msg_box->path_site;
	ROUTE_PATH * route_path = (ROUTE_PATH *)msg_box->policy;
	if(curr_pathrecord==NULL)
		return NULL;
	Record_List * next_pathrecord = (Record_List *)curr_pathrecord->list.next;
	if(next_pathrecord == &route_path->route_path.head.list)
		return NULL;
	return (ROUTE_NODE *) next_pathrecord->record;
}

int rule_get_target(void * router_rule,void * message,void **result)
{
    BYTE * target;
    int ret;
    int flag;
    char buffer[DIGEST_SIZE*2];
    ROUTE_RULE * rule= (ROUTE_RULE *)router_rule;
    struct message_box * msg_box = (struct message_box *)message;
    *result=NULL;
    switch(rule->target_type){
	    case ROUTE_TARGET_NAME:
	    case ROUTE_TARGET_PORT:
	    case ROUTE_TARGET_LOCAL:
		    target=Talloc0(DIGEST_SIZE);
		    Strncpy(target,rule->target_name,DIGEST_SIZE);
		    message_set_receiver(message,target);
		    flag=message_get_flag(message) | MSG_FLAG_LOCAL;
		    message_set_flag(message,flag);
		    
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
		    message_set_receiver(message,target);
		    flag=message_get_flag(message) &(~MSG_FLAG_LOCAL);
		    message_set_flag(message,flag);
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
	    case ROUTE_TARGET_ERROR:
	    default:
		    return -EINVAL;
    }
    *result=target;
    return rule->target_type;	
}

int message_route_setstart( void * msg, void * path)
{
	int ret;
	struct message_box * msg_box = (struct message_box *)msg;
	ROUTE_PATH * route_path = (ROUTE_PATH *)path;
	ROUTE_NODE * startnode;
	char * receiver;
	
	msg_box->policy = path;
	msg_box->head.flow=route_path->flow;
	Strncpy(msg_box->head.route,route_path->name,DIGEST_SIZE);
//	msg_box->path_site=NULL;
//	return 0;

	msg_box->path_site = route_path->route_path.head.list.next;
	startnode = get_curr_pathsite(msg);
	if(startnode==NULL)
		return -EINVAL;
	
	return rule_get_target(&startnode->this_target,msg,&receiver);

}

int message_route_setremotestart( void * msg)
{
	int ret;
	struct message_box * msg_box = (struct message_box *)msg;
	ROUTE_PATH * route_path ;
	ROUTE_NODE * startnode;
	char * receiver;
	
	ret=router_find_policy_byname(&route_path,msg_box->head.route,msg_box->head.rjump,msg_box->head.ljump);
	if(ret<0)
		return ret;
	if(route_path==NULL)
		return 0;

	msg_box->policy = route_path;
	if(msg_box->head.state != MSG_STATE_RESPONSE)
	{
		msg_box->path_site = route_path->route_path.head.list.next;
		startnode = get_curr_pathsite(msg);
		if(startnode==NULL)
			return -EINVAL;
	}
	else
	{
		msg_box->path_site=route_path->response_path.head.list.next;
		startnode = get_curr_pathsite(msg);
		if(startnode==NULL)
			return 0;
	}
	
	
	if((message_get_flow(msg)==MSG_FLOW_QUERY) &&(msg_box->head.state!=MSG_STATE_RESPONSE))
	{
		// find hash nodelist
		NODE_LIST * hash_list = &hash_forest[_hash_index(msg_box->head.nonce)];
		if(hash_list==NULL)
			return -EINVAL;
		// build a query trace node and add it in hash nodelist 	
		TRACE_NODE * trace_node = Dalloc0(sizeof(*trace_node),NULL);
		if(trace_node ==NULL)
			return -ENOMEM;
		Memcpy(trace_node->msg_uuid,msg_box->head.nonce,DIGEST_SIZE);
		Memcpy(trace_node->source_uuid,msg_box->head.sender_uuid,DIGEST_SIZE);
		trace_node->path=msg_box->policy;
		
		// add trace_node to hash_nodelist 
      		 _node_list_add(hash_list,trace_node);
			
	} 

	return rule_get_target(&startnode->this_target,msg,&receiver);
}

int message_route_setnext( void * msg, void * path)
{
	int ret;
	struct message_box * msg_box = (struct message_box *)msg;
	ROUTE_NODE * nextnode;
	char * receiver;
	
	nextnode = set_next_pathsite(msg);
	if(nextnode==NULL)
	{
		Memset(msg_box->head.receiver_uuid,0,DIGEST_SIZE);
		//msg_box->head.flow = MSG_FLOW_FINISH;
		return 0;
	}
	
	return rule_get_target(&nextnode->this_target,msg,&receiver);

}

int message_route_findtrace(void * msg)
{
	struct message_box * message=msg;
	TRACE_NODE * trace_node;
	Record_List * trace_record;
	struct List_head * curr_head;
	if(msg==NULL)
		return 0;
	NODE_LIST * hash_list = &hash_forest[_hash_index(message->head.nonce)];
	if(hash_list==NULL)
		return -EINVAL;
	curr_head = find_elem_with_tag(hash_list,
                tracenode_comp_uuid,message->head.nonce);
        if(curr_head == NULL)
        {
		message->head.flow=MSG_FLOW_FINISH;
                return 0;
        }
        trace_record=List_entry(curr_head,Record_List,list);
	trace_node=trace_record->record;
	Memcpy(message->head.receiver_uuid,trace_node->source_uuid,DIGEST_SIZE);
	message->head.state = MSG_STATE_RESPONSE;
	message->head.flag &= ~MSG_FLAG_LOCAL;

	return 1;	
}

int router_dup_activemsg_info (void * message)
{
	int ret;
	struct message_box * msg_box=message;
	MSG_HEAD * msg_head;
	MSG_HEAD * new_msg_head;
	if(message==NULL)
		return -EINVAL;

	struct message_box * active_msg=message_get_activemsg(message);
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
	message_set_flag(msg_box,msg_head->flag);
	message_set_route(msg_box,msg_head->route);
	new_msg_head->ljump=msg_head->ljump;	
	new_msg_head->rjump=msg_head->rjump;	

	MSG_EXPAND * old_expand;
	MSG_EXPAND * new_expand;

	int i=0;
	if(!(new_msg_head->flag & MSG_FLAG_FOLLOW))
	{
		do{
			ret = message_get_expand(active_msg,&old_expand,i++);
			if(ret<0)
				return ret;
			if(old_expand==NULL)
				break;
			ret=message_add_expand(message,old_expand);
			if(ret<0)
				return ret;
		}while(1);	
	}
	msg_box->policy=active_msg->policy;
	msg_box->path_site=active_msg->path_site;

	return 1;
}
/*
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
*/
/*
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
*/

int router_find_route_policy(void * message,void **msg_policy)
{
    ROUTE_PATH * policy;
    int ret;
    *msg_policy=NULL;

    ret=dispatch_policy_getfirst(&policy);
    if(ret<0)
        return ret;
    MSG_HEAD * msg_head;
    msg_head=message_get_head(message);
    if(msg_head==NULL)
	return -EINVAL;
    	
    while(policy!=NULL)
    {
	if((policy->state==POLICY_CLOSE)||(policy->state==POLICY_IGNORE))
	{
    		ret=dispatch_policy_getnext(&policy);
    		if(ret<0)
             		return ret;
		continue;
	}

    	ret=route_match_sender(policy,msg_head->sender_uuid);
	if(ret>0)
	{	
		ret=route_match_message_jump(policy,message);
		if(ret>0)
		{
       			ret=route_match_message(policy,message);
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

int router_find_policy_byname(void **msg_policy,char * name,int rjump,int ljump)
{
    ROUTE_PATH * policy;
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
}


/*
int router_dup_activemsg_info (void * message)
{
	int ret;
	struct message_box * msg_box=message;
	MSG_HEAD * msg_head;
	MSG_HEAD * new_msg_head;
	if(message==NULL)
		return -EINVAL;

	struct message_box * active_msg=message_get_activemsg(message);
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

	int i=0;
	if(!(new_msg_head->flag & MSG_FLAG_FOLLOW))
	{
		do{
			ret = message_get_expand(active_msg,&old_expand,i++);
			if(ret<0)
				return ret;
			if(old_expand==NULL)
				break;
			ret=message_add_expand(message,old_expand);
			if(ret<0)
				return ret;
		}while(1);	
	}
	message_set_policy(message,message_get_policy(active_msg));

	return 1;
}
*/

/*
int __message_policy_init(void * policy)
{
    MESSAGE_POLICY * message_policy=(MESSAGE_POLICY *)policy;
    if(message_policy==NULL)
        return -EINVAL;
    message_policy->sender_proc=NULL;
    message_policy->match_policy_list=
            (ROUTE_NODE_LIST *)malloc(sizeof(ROUTE_NODE_LIST));
    if(message_policy->match_policy_list==NULL)
        return -EINVAL;

    message_policy->dup_route_rule =
            (ROUTE_NODE_LIST *)malloc(sizeof(ROUTE_NODE_LIST));
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

/*
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
		msg_head->flag &=~MSG_FLAG_RESPONSE;
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
//      clear dup info's active message
        message_set_activemsg(message,NULL);	
//      replace police to new 

	return 1;
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
*/
