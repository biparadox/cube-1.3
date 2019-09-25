#ifndef ROUTE_TREE_H
#define ROUTE_TREE_H

#include "dispatch.h"

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

typedef struct tagroute_node
{
	
	ROUTE_RULE next_target;
	void * route_path; 
	NODE_LIST * aspect_node;
	
}__attribute__((packed)) ROUTE_NODE;

typedef struct tagroute_trace
{
	int  rjump;
	int  isstart;
	BYTE from_uuid[DIGEST_SIZE];
	NODE_LIST aspect_node;
}__attribute__((packed)) ROUTE_TRACE;

typedef struct route_path
{
	char name[32];
	char subname[32];
	int  layer;
	int  sequence;
	int  curr_state;
	int  next_state;
	struct route_tree_node * prev;   

	int match_rule_num;
	int route_rule_num;

	MATCH_RULE * match_list;
	NODE_LIST  route_path;
	ROUTE_TRACE * response_template;
	void *  response_hash;
	void *  aspect_hash;

        NODE_LIST next_route;
	NODE_LIST err_route;         
}ROUTE_PATH;

#endif
