#ifndef ROUTE_TREE_H
#define ROUTE_TREE_H

enum route_type
{
	ROUTE_DELIVER=0x01,
	ROUTE_QUERY,
	ROUTE_DUP,
	ROUTE_ASPECT,
	ROUTE_MERGE,
	ROUTE_FINISH
};

typedef struct NODE_LIST
{	
	Record_List head;
	int state;
	int policy_num;
	void * curr;
}NODE_LIST;

typedef struct tagmatch_rule
{
	int op;
	int area;
	int type;
	int subtype;
	int ljump;
	int rjump;
	int state;
	int flag;
	void * match_template;
	void * value;
}__attribute__((packed)) MATCH_RULE;

typedef struct tagroute_rule
{
	int target_type;
	char * target_name;
}__attribute__((packed)) ROUTE_RULE;

typedef struct tagtrace_node
{
	BYTE msg_uuid[DIGEST_SIZE];
	enum message_flow_type trace_flag; // 0 means query response, i means aspect
	BYTE source_uuid[DIGEST_SIZE];
	void * aspect_site;
	void * path;
}__attribute__((packed)) TRACE_NODE;

typedef struct tagaspect_node
{
	BYTE msg_uuid[DIGEST_SIZE];
	int trace_flag; // 0 means query response, i means aspect
	BYTE old_msguuid[DIGEST_SIZE];
   char sender_uuid[DIGEST_SIZE];     // sender's uuid, or '@' followed with a name, or ':' followed with a connector's name
   char receiver_uuid[DIGEST_SIZE];   // receiver's uuid, or '@" followed with a name, or ':' followed with a connector's name
   char route[DIGEST_SIZE];
   int  flow;
   int  state;
   int  flag;
   int  ljump;
   int  rjump;
	void * aspect_site;
	void * path;
}__attribute__((packed)) ASPECT_NODE;

typedef struct tagroute_node
{
	void * route_path; 
	int layer;	
	ROUTE_RULE this_target;
	Record_List * chain;
	NODE_LIST aspect_branch;
	
}__attribute__((packed)) ROUTE_NODE;

typedef struct tagbranch_node
{
	void * route_path; 
	int layer;	
	enum message_flow_type branch_type;
	void * branch_path;
}__attribute__((packed)) BRANCH_NODE;
/*
typedef struct tagroute_trace
{
	int  rjump;
	int  isstart;
	BYTE from_uuid[DIGEST_SIZE];
	NODE_LIST aspect_node;
}__attribute__((packed)) ROUTE_TRACE;
*/

typedef struct route_path
{
	char name[32];
	char sender[32];
	int  ljump;
	int  rjump;
	enum dispatch_policy_state state;
	enum message_flow_type flow;

	int match_rule_num;
	int route_rule_num;

	NODE_LIST  match_list;
	NODE_LIST  route_path;
	NODE_LIST  response_path;
//	void *  response_hash;
//	void *  aspect_hash;

//      NODE_LIST next_route;
//	NODE_LIST err_route;         
}ROUTE_PATH;

int router_find_policy_byname(void **msg_policy,char * name, int rjump,int ljump);
int route_get_node_byjump(ROUTE_PATH * path, ROUTE_NODE ** node,int ljump,int reverse);
int _route_match_rule(void * message,void * path);			
void * route_dup_message(void * message,BRANCH_NODE * branch);
int route_aspect_message(void * message,BRANCH_NODE * branch);

int _waiting_message_add(void * msg);
void _waiting_message_del(void * record);
void * _waiting_message_getfirst();
void * _waiting_message_removehead();
void * _waiting_message_getnext();
void * route_read_policy(void * policy_node);

Record_List * _node_list_getfirst(void * list);
Record_List * _node_list_getnext(void * list);
#endif
