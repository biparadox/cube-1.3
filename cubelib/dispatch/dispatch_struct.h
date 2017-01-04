
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

/*
static NAME2VALUE match_op_type_list[] =
{
	{"AND",DISPATCH_MATCH_AND},
	{"OR",DISPATCH_MATCH_OR},
	{"NOT",DISPATCH_MATCH_NOT},
	{NULL,0},
};

static NAME2VALUE message_area_type_list[] =
{
	{"NULL",MATCH_AREA_NULL},
	{"HEAD",MATCH_AREA_HEAD},
	{"RECORD",MATCH_AREA_RECORD},
	{"EXPAND",MATCH_AREA_EXPAND},
	{NULL,0},
};

static NAME2VALUE route_target_type_list[] =
{
	{"LOCAL",ROUTE_TARGET_LOCAL},
	{"NAME",ROUTE_TARGET_NAME},
	{"UUID",ROUTE_TARGET_UUID},
	{"RECORD",ROUTE_TARGET_RECORD},
	{"EXPAND",ROUTE_TARGET_EXPAND},
	{"CHANNEL",ROUTE_TARGET_CHANNEL},
	{"PORT",ROUTE_TARGET_PORT},
	{"MIXUUID",ROUTE_TARGET_MIXUUID},
	{"ERROR",ROUTE_TARGET_ERROR},
	{NULL,0}
};

static struct struct_elem_attr match_rule_desc[] =
{
	{"op",CUBE_TYPE_ENUM,0,&match_op_type_list,NULL},
	{"area",CUBE_TYPE_ENUM,0,&message_area_type_list,NULL},
	{"type",CUBE_TYPE_RECORDTYPE,sizeof(int),NULL,NULL},
	{"subtype",CUBE_TYPE_RECORDSUBTYPE,sizeof(int),NULL,"type"},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};

static NAME2VALUE message_flow_type_valuelist[]=
{
	{"INIT",MSG_FLOW_INIT},
	{"LOCAL",MSG_FLOW_LOCAL},
	{"DELIVER",MSG_FLOW_DELIVER},
	{"QUERY",MSG_FLOW_QUERY},
	{"RESPONSE",MSG_FLOW_RESPONSE},
	{"ASPECT",MSG_FLOW_ASPECT},
	{"ASPECT_LOCAL",MSG_FLOW_ASPECT_LOCAL},
	{"ASPECT_RETURN",MSG_FLOW_ASPECT_RETURN},
	{"TRANS",MSG_FLOW_TRANS},
	{"DRECV",MSG_FLOW_DRECV},
	{"QRECV",MSG_FLOW_QRECV},
	{"FINISH",MSG_FLOW_FINISH},
	{"ERROR",MSG_FLOW_ERROR},
	{"NULL",0},
};
*/

typedef struct tagroute_rule
{
	int type;
	int target_type;
	char * target_name;
}__attribute__((packed)) ROUTE_RULE;

/*
struct struct_elem_attr route_rule_desc[] =
{
	{"type",CUBE_TYPE_ENUM,0,&message_flow_type_valuelist,NULL},
	{"target_type",CUBE_TYPE_ENUM,0,&route_target_type_list,NULL},
	{"target_name",CUBE_TYPE_ESTRING,DIGEST_SIZE*2,NULL,NULL},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};
*/
typedef struct tagdispatch_policy
{
	char sender[DIGEST_SIZE];
	POLICY_LIST match_list;
	POLICY_LIST route_list;
}  DISPATCH_POLICY;

#endif // DISPATCH_STRUCT_H
