#ifndef ROUTE_H
#define ROUTE_H

#define DIGEST_SIZE 32


enum dynamic_dispatch_typelist
{
	DTYPE_DISPATCH=0x210,
};

enum subtypelist_dispatch
{
	SUBTYPE_POLICY_HEAD=0x01,
	SUBTYPE_MATCH_HEAD,
	SUBTYPE_ROUTE_RULE
};


enum match_op_type
{
    DISPATCH_MATCH_AND=0x01,
    DISPATCH_MATCH_OR,
    DISPATCH_MATCH_NOT,	
    DISPATCH_MATCH_ORNOT,	
    DISPATCH_MATCH_ERROR,
};

enum message_area_type
{
    MATCH_AREA_NULL=0x01,	
    MATCH_AREA_HEAD=0x02,
    MATCH_AREA_RECORD=0x04,
    MATCH_AREA_EXPAND=0x08,
    MATCH_AREA_ERR=0xFF,
};

enum route_target_type
{
    ROUTE_TARGET_LOCAL=0x01,
    ROUTE_TARGET_NAME=0x02,
    ROUTE_TARGET_UUID=0x04,
    ROUTE_TARGET_CONN=0x08,
    ROUTE_TARGET_RECORD=0x10,
    ROUTE_TARGET_EXPAND=0x20,
    ROUTE_TARGET_CHANNEL=0x40,
    ROUTE_TARGET_PORT=0x80,
    ROUTE_TARGET_MIXUUID=0x100,
    ROUTE_TARGET_ERROR=0xFFFF,
};

enum dispatch_policy_state
{
	POLICY_OPEN=0x01,
	POLICY_CLOSE,
	POLICY_WAIT,	
	POLICY_TEST,	
	POLICY_IGNORE
};
int dispatch_init();
void * dispatch_policy_create();


void * dispatch_create_match_list(void);
void * dispatch_create_route_list(void);


void * dispatch_read_match_policy(void * policy_node);
int dispatch_add_match_policy(void * list,void * policy);

void * dispatch_read_route_policy(void * policy_node);
int dispatch_add_route_policy(void * list,void * policy);

void * dispatch_read_policy(void * policy_node);
int dispatch_policy_add(void * policy);
int dispatch_aspect_policy_add(void * policy);



int dispatch_policy_getfirst(void ** policy);
int dispatch_policy_getnext(void ** policy);
int dispatch_policy_getfirstmatchrule(void * policy,void ** rule);
int dispatch_policy_getnextmatchrule(void * policy,void ** rule);
int dispatch_policy_getfirstrouterule(void * policy,void ** rule);
int dispatch_policy_getnextrouterule(void * policy,void ** rule);
int dispatch_policy_getstate(void * policy);
const char * dispatch_policy_getname(void * policy);

int dispatch_match_sender(void * policy,char * sender);
int dispatch_match_message(void * policy,void * message);

int aspect_policy_getfirst(void ** policy);
int aspect_policy_getnext(void ** policy);

int route_find_local_policy(void * message,void **msg_policy,char * sender_proc);
int route_find_aspect_policy(void * message,void **msg_policy,char * sender_proc);
int router_find_policy_byname(void **msg_policy,char * name, int rjump,int ljump);

int route_push_site(void * message,BYTE * name);
int route_push_site_str(void * message,char * name);
int route_push_aspect_site(void * message,char * proc,char * point);
int route_check_sitestack(void * message);
int route_pop_site(void * message, char * type);
int route_pop_aspect_site(void * message, char * proc);
int router_set_local_route(void * message,void * policy);
int router_set_aspect_route(void * message,void * policy);
int router_set_dup_route(void * message,void * policy);
int route_dup_activemsg_info (void * message);


#endif // DISPATCH_H
