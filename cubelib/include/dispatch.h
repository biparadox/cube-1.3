#ifndef ROUTE_H
#define ROUTE_H

#define DIGEST_SIZE 32

enum match_op_type
{
    DISPATCH_MATCH_AND=0x01,
    DISPATCH_MATCH_OR,
    DISPATCH_MATCH_NOT,	
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
    ROUTE_TARGET_RECORD=0x08,
    ROUTE_TARGET_EXPAND=0x10,
    ROUTE_TARGET_CHANNEL=0x40,
    ROUTE_TARGET_PORT=0x80,
    ROUTE_TARGET_MIXUUID=0x100,
    ROUTE_TARGET_ERROR=0xFFFF,
};

int dispatch_init(void * object);
void * dispatch_policy_create();


void * dispatch_create_match_list(void);
void * dispatch_create_route_list(void);


void * dispatch_read_match_policy(void * policy_node);
int dispatch_add_match_policy(void * list,void * policy);

void * dispatch_read_route_policy(void * policy_node);
int dispatch_add_route_policy(void * list,void * policy);

void * dispatch_read_policy(void * policy_node);
int dispatch_policy_add(void * object,void * policy);



int dispatch_policy_getfirst(void ** policy);
int dispatch_policy_getnext(void ** policy);
int dispatch_policy_getfirstmatchrule(void * policy,void ** rule);
int dispatch_policy_getfirstrouterule(void * policy,void ** rule);

int dispatch_match_message(void * policy,void * message);

int  	aspect_policy_getfirst(void ** policy);
int aspect_policy_getnext(void ** policy);

int route_find_local_policy(void * message,void **msg_policy,char * sender_proc);
int route_find_aspect_policy(void * message,void **msg_policy,char * sender_proc);
int route_find_aspect_local_policy(void * message,void **msg_policy,char * sender_proc);

int route_push_site(void * message,char * name,char * type);
int route_push_aspect_site(void * message,char * proc,char * point);
int route_check_sitestack(void * message,char * type);
int route_pop_site(void * message, char * type);
int route_pop_aspect_site(void * message, char * proc);
int route_dup_activemsg_info (void * message);


#endif // DISPATCH_H
