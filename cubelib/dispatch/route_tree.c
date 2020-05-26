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
int route_policy_addmatchrule(void * path,MATCH_RULE * rule);
int route_policy_addrouterule(void * path,ROUTE_RULE * rule);
int route_policy_getnextmatchrule(void * path,MATCH_RULE ** rule);
int route_policy_getnextrouterule(void * path,ROUTE_RULE ** rule);
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
	node_list->curr = node_list->head.list.next;

	Record_List * record; 

    record=List_entry(node_list->curr,Record_List,list);
	return record;	
}

Record_List * _node_list_getnext(void * list)
{
	Record_List * record; 
	NODE_LIST * node_list=list;
	struct List_head * curr = (struct List_head *)node_list->curr;

	if(curr->next == &(node_list->head.list))
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

	// create an empty route path
    ROUTE_PATH  * path =route_path_create();
    if(path==NULL)
        return NULL;

	// get policy head
    temp_node=json_find_elem("policy_head",policy_node);
    if(temp_node==NULL)
    {
	return NULL;
    }

    policy=Talloc0(sizeof(*policy));
    if(policy==NULL)
	return NULL;

    ret=json_2_struct(temp_node,policy,policy_head_template);
    if(ret<0)
        return NULL;

    // set policy head to path	
	// for DUP and ASPECT policy, find old path and insert branch

    switch(policy->type)
	{
		case MSG_FLOW_DUP:
		case MSG_FLOW_ASPECT:
			
			// find the origin policy
			ret=router_find_policy_byname(&normal_path,policy->name,policy->rjump,policy->ljump);
			if(ret<0)
				return ret;
			if(normal_path==NULL)
			{
				print_cubeerr("can't find %s policy's aspect path!\n",policy->newname);
				return 0;
			}

			// find the insert site in the origin policy
			if(policy->ljump!=0)
			{
				// insert aspect policy to the old policy by policy->ljump ,
				// if policy->rjump is negative, it should be insert to the response path
				ret = route_get_node_byjump(normal_path, &insert_node,policy->ljump,policy->rjump);
			}
/*
			else if( Memcmp(policy->sender,EMPTY_UUID,DIGEST_SIZE) != 0)
			{
				// insert aspect policy to the old policy by finding message sender, 
				// if policy->rjump is negative, it should be insert to the response path
				ret = route_get_node_bysender(normal_path,&insert_node,policy->sender,policy->rjump);
			}
*/
			else
			{
				print_cubeerr("can't find %s policy's insert node!\n",policy->newname);
				return -EINVAL;
			}
			if(ret<0)
				return ret;
			if(insert_node==NULL)
			{
				print_cubeerr("can't find aspect policy %s 's insert node!\n",policy->newname);
				return -EINVAL;
			}

			// build the branch node
			branch_node = Talloc0(sizeof(*branch_node));
			if(branch_node == NULL)
				return -ENOMEM;
			branch_node->route_path = normal_path;
			branch_node->branch_type =policy->type;  
			branch_node->branch_path=path;

			// insert branch node to the aspect branch
			
     		if((record=_node_list_add(&insert_node->aspect_branch,(void *)branch_node))==NULL)
			{
				print_cubeerr("insert aspect policy %s failed!\n",policy->newname);
				return 0;
			}

			// set aspect policy's type
    		Strncpy(path->name,policy->newname,DIGEST_SIZE);
			if(policy->type == MSG_FLOW_DUP)
				path->flow=MSG_FLOW_DELIVER;
			else
				path->flow=MSG_FLOW_QUERY;
			break;
		default:
			// 	
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
		ret=route_policy_addmatchrule(path,temp_match_rule);
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
		ret=route_policy_addrouterule(path,temp_route_rule);
		if(ret<0)
			return NULL;	
        route_rule_node=json_get_next_child(temp_node);
    } 

	// build a finish route node and add it to the route rule
   	temp_route_rule=Dalloc0(sizeof(ROUTE_RULE),path);
	if(temp_route_rule==NULL)
		return NULL;
	temp_route_rule->target_type=ROUTE_TARGET_FINISH;
	ret=route_policy_addrouterule(path,temp_route_rule);
	if(ret<0)
		return NULL;	

    // next,read the response route policy

	if(path->flow == MSG_FLOW_QUERY)
	{

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

		// build a finish route node and add it to the response rule
   		temp_route_rule=Dalloc0(sizeof(ROUTE_RULE),path);
		if(temp_route_rule==NULL)
			return NULL;
		temp_route_rule->target_type=ROUTE_TARGET_FINISH;
		ret=route_tree_addresponserule(path,temp_route_rule);
		if(ret<0)
			return NULL;	
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

	ret = _route_match_rules(aspect_branch->branch_path,message);
	return ret;
}

void * route_dup_message(void * message,BRANCH_NODE * branch)
{
	struct message_box * msg_box=(struct message_box *)message;
	if(branch->branch_type != MSG_FLOW_DUP)
		return NULL;	
	ROUTE_PATH * branch_path = branch->branch_path;
	struct message_box * new_msg = message_clone(message);
	if(new_msg==NULL)
		return NULL;
	new_msg->head.ljump=1;
	new_msg->head.rjump=1;
	Strncpy(new_msg->head.route,branch_path->name,DIGEST_SIZE);
	Strncpy(new_msg->head.sender_uuid,msg_box->head.sender_uuid,DIGEST_SIZE);
	message_route_setstart(new_msg,branch_path);
	return new_msg;
}

ASPECT_NODE * _create_aspect_node(void * message,BRANCH_NODE * branch)
{
	struct message_box * msg_box=(struct message_box *)message;
	// build a query trace node and add it in hash nodelist 	
	ASPECT_NODE * aspect_node = Dalloc0(sizeof(*aspect_node),NULL);
	if(aspect_node ==NULL)
		return -ENOMEM;
	Memcpy(aspect_node->old_msguuid,msg_box->head.nonce,DIGEST_SIZE);
	aspect_node->trace_flag=MSG_FLOW_ASPECT;
	get_random_uuid(msg_box->head.nonce);
	Memcpy(aspect_node->msg_uuid,msg_box->head.nonce,DIGEST_SIZE);
	Memcpy(aspect_node->sender_uuid,msg_box->head.sender_uuid,DIGEST_SIZE);
	Memcpy(aspect_node->receiver_uuid,msg_box->head.receiver_uuid,DIGEST_SIZE);
	Memcpy(aspect_node->route,msg_box->head.route,DIGEST_SIZE);
	aspect_node->flow=msg_box->head.flow;
	aspect_node->state=msg_box->head.state;
	aspect_node->flag=msg_box->head.flag;
	aspect_node->ljump=msg_box->head.ljump;
	aspect_node->rjump=msg_box->head.rjump;
	
	aspect_node->path=msg_box->policy;
	aspect_node->aspect_site=msg_box->path_site;
		
	Strncpy(msg_box->head.route,msg_box->head.sender_uuid,DIGEST_SIZE);
	// find hash nodelist
	NODE_LIST * hash_list = &hash_forest[_hash_index(msg_box->head.nonce)];
	if(hash_list==NULL)
		return -EINVAL;
     // add trace_node to hash_nodelist 
     _node_list_add(hash_list,aspect_node);
	return aspect_node;

}

int _recover_aspect_message(void * message,ASPECT_NODE * aspect_node)
{
	struct message_box * msg_box = message;
	const int flag_mask = MSG_FLAG_CRYPT | MSG_FLAG_SIGN | MSG_FLAG_ZIP | MSG_FLAG_VERIFY;
	Memcpy(msg_box->head.nonce,aspect_node->old_msguuid,DIGEST_SIZE);
	Memcpy(msg_box->head.sender_uuid,aspect_node->sender_uuid,DIGEST_SIZE);
	Memcpy(msg_box->head.receiver_uuid,aspect_node->receiver_uuid,DIGEST_SIZE);
	Memcpy(msg_box->head.route,aspect_node->route,DIGEST_SIZE);
	msg_box->head.flow = aspect_node->flow;
	msg_box->head.state = aspect_node->state;

	msg_box->head.flag = (msg_box->head.flag & flag_mask) | (aspect_node->flag & (~flag_mask));
	msg_box->head.ljump = aspect_node->ljump;
	msg_box->head.rjump = aspect_node->rjump;
	
	msg_box->policy = aspect_node->path;
	msg_box->path_site = aspect_node->aspect_site;

	return 0;	

}


int route_aspect_message(void * message,BRANCH_NODE * branch)
{
	struct message_box * msg_box=(struct message_box *)message;
	if(branch->branch_type != MSG_FLOW_ASPECT)
		return -EINVAL;	
	ROUTE_PATH * branch_path = branch->branch_path;
	// wait to add aspect function
	ASPECT_NODE * aspect_node = _create_aspect_node(message,branch);
	if(aspect_node ==NULL)
		return -EINVAL;

	msg_box->head.ljump=1;
	msg_box->head.rjump=1;
		
	message_route_setstart(message,branch_path);
	return 1;
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

int route_policy_getnextmatchrule(void * path,MATCH_RULE ** rule)
{
    ROUTE_PATH  * route_path=(ROUTE_PATH *)path;
    return _dispatch_policy_getnext(&route_path->match_list,(void **)rule);
}

int route_policy_getnextrouterule(void * path,ROUTE_RULE ** rule)
{
    ROUTE_PATH  * route_path=(ROUTE_PATH *)path;
    return _dispatch_policy_getnext(&route_path->route_path,(void **)rule);
}


int route_policy_addmatchrule(void * path,MATCH_RULE * rule)
{
    ROUTE_PATH  * route_path=(ROUTE_PATH *)path;
    if(_node_list_add(&route_path->match_list,(void *)rule)==NULL)
	return 0;
    return 1;
}

int route_policy_addrouterule(void * path,ROUTE_RULE * rule)
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
		ret=route_policy_getnextmatchrule(path,&match_rule);
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
		ret=route_policy_getnextmatchrule(path,&match_rule);
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
			if(Isstrinuuid(target))
		    		message_set_receiver(message,target);
			else
			       message_set_receiver_uuid(message,target);
		    	flag=message_get_flag(message) &(~MSG_FLAG_LOCAL);
		    	message_set_flag(message,flag);
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
				
			if(Isstrinuuid(target))
		    		message_set_receiver(message,target);
			else
			       message_set_receiver_uuid(message,target);
		    	flag=message_get_flag(message) &(~MSG_FLAG_LOCAL);
		    	message_set_flag(message,flag);
		   	break;
		   }
	    case ROUTE_TARGET_FINISH:
			break;
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
	msg_box->head.state = MSG_STATE_MATCH;
	Strncpy(msg_box->head.route,route_path->name,DIGEST_SIZE);
//	msg_box->path_site=NULL;
//	return 0;

	msg_box->path_site = &route_path->route_path.head.list;
//	startnode = get_curr_pathsite(msg);
//	if(startnode==NULL)
//		return -EINVAL;
	
//	return rule_get_target(&startnode->this_target,msg,&receiver);
	return 1;

}

int message_route_setremotestart( void * msg)
{
	int ret;
	struct message_box * msg_box = (struct message_box *)msg;
	ROUTE_PATH * route_path ;
	ROUTE_NODE * startnode;
	char * receiver;
	
	//ret=router_find_policy_byname(&route_path,msg_box->head.route,msg_box->head.rjump,msg_box->head.ljump);
	ret=router_find_policy_byname(&route_path,msg_box->head.route,msg_box->head.rjump,0);
	if(ret<0)
		return ret;
	if(route_path==NULL)
		return 0;

	msg_box->policy = route_path;
	if(msg_box->head.state != MSG_STATE_RESPONSE)
	{
		msg_box->path_site = &route_path->route_path.head.list;
//		startnode = get_curr_pathsite(msg);
//		if(startnode==NULL)
//			return -EINVAL;
	}
	else
	{
		msg_box->path_site=&route_path->response_path.head.list;
//		startnode = get_curr_pathsite(msg);
//		if(startnode==NULL)
//			return 0;
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

//	return rule_get_target(&startnode->this_target,msg,&receiver);
	return 1;
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

void * _find_flowtype_node(NODE_LIST * hash_list,BYTE * comp_uuid,enum message_flow_type flow)
{
	Record_List * curr_record;
	struct List_head * curr_head = hash_list->head.list.next;
	TRACE_NODE * trace_node=NULL;

	while(curr_head != & hash_list->head.list)
	{
        curr_record=List_entry(curr_head,Record_List,list);
		trace_node=curr_record->record;
		if(trace_node==NULL)
			return NULL;
		if(Memcmp(trace_node->msg_uuid,comp_uuid,DIGEST_SIZE)==0)
		{
			if(trace_node->trace_flag == MSG_FLOW_ASPECT)
			{
				if(flow==MSG_FLOW_ASPECT)
					return curr_record;
			}
			else
			{
				return curr_record;
			}	
		}
		curr_head=curr_head->next;		
	}	
	return NULL;
}


int message_route_findtrace(void * msg,enum message_flow_type flow)
{
	struct message_box * message=msg;
	TRACE_NODE * trace_node;
	ASPECT_NODE * aspect_node;
	Record_List * trace_record;
	struct List_head * curr_head;
	if(msg==NULL)
		return 0;
	NODE_LIST * hash_list = &hash_forest[_hash_index(message->head.nonce)];
	if(hash_list==NULL)
		return -EINVAL;
	trace_record = _find_flowtype_node(hash_list,message->head.nonce,flow);
	if(trace_record ==NULL)
	{
		message->head.flow=MSG_FLOW_FINISH;
        return 0;
    }
	if(flow==MSG_FLOW_QUERY)
	{
		trace_node=trace_record->record;
		Memcpy(message->head.receiver_uuid,trace_node->source_uuid,DIGEST_SIZE);
		message->head.state = MSG_STATE_RESPONSE;
		message->head.flag &= ~MSG_FLAG_LOCAL;
	}
	else if(flow ==MSG_FLOW_ASPECT)
	{
		aspect_node=trace_record->record;
		_recover_aspect_message(message,aspect_node);
		//Free(aspect_node); 	
    	//List_del(&trace_record);
	}
    	//_node_list_del(trace_record);
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
				if(ljump==0)
				{
					*msg_policy=policy;		
					break;
				}
			
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

