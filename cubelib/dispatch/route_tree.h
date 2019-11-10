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
	int trace_flag; // 0 means query response, i means aspect
	BYTE source_uuid[DIGEST_SIZE];
	void * aspect_site;
	void * path;
}__attribute__((packed)) TRACE_NODE;

typedef struct tagaspect_node
{
	BYTE msg_uuid[DIGEST_SIZE];
	int trace_flag; // 0 means query response, i means aspect
	BYTE old_msguuid[DIGEST_SIZE];
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

int _waiting_message_add(void * msg);
void _waiting_message_del(void * record);
void * _waiting_message_getfirst();
void * _waiting_message_getnext();


#endif
