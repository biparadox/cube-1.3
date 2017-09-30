
#ifndef DISPATCH_STRUCT_H
#define DISPATCH_STRUCT_H

#define MATCH_FLAG 0x8000

typedef struct policy_list
{	
	Record_List head;
	int state;
	int policy_num;
	void * curr;
}POLICY_LIST;

typedef struct tagmatch_rule
{
	int op;
	int area;
	int type;
	int subtype;
	void * match_template;
	void * value;
}__attribute__((packed)) MATCH_RULE;

typedef struct tagroute_rule
{
	int target_type;
	char * target_name;
}__attribute__((packed)) ROUTE_RULE;


typedef struct tagdispatch_policy
{
	char * name;
	enum dispatch_policy_state state;	
	int type;
	char sender[DIGEST_SIZE];
	char * newname; 
	int ljump;
	int rjump;
	POLICY_LIST match_list;
	POLICY_LIST route_list;
}__attribute__((packed))  DISPATCH_POLICY;

static POLICY_LIST local_router_policy;
static POLICY_LIST main_router_policy;
static POLICY_LIST aspect_router_policy;


struct expand_route_record
{
	char sender_uuid[DIGEST_SIZE];
	char receiver_uuid[DIGEST_SIZE];
	char route[DIGEST_SIZE];
	int  flow;
	int  state;
	int  flag;
	int  ljump;
	int  rjump;	
} __attribute__((packed));

#endif // DISPATCH_STRUCT_H
