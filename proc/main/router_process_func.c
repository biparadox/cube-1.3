#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "data_type.h"
#include "alloc.h"
#include "string.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "message.h"
#include "memdb.h"
#include "dispatch.h"
#include "ex_module.h"

//#include "router_value.h"
#include "router_process_func.h"
#include "main_proc_func.h"

int proc_router_hit_target(void * message,char * local_uuid,char * proc_name)
{
	int ret;
	char conn_uuid[DIGEST_SIZE*2];
	char * receiver = message_get_receiver(message);
	if(receiver ==NULL)
		return -EINVAL;
	if(receiver[0]==':')
		return 1;
	if(receiver[0]=='@')
		return !(strncmp(receiver+1,proc_name,DIGEST_SIZE*2-1));

	ret=comp_proc_uuid(local_uuid,proc_name,conn_uuid);
	return !(strncmp(receiver,conn_uuid,DIGEST_SIZE*2));

}

int 	proc_audit_init()
{
	char audit_text[4096];
        const char * audit_filename= "./message.log";
    	int fd ;
	char *beginstr="\n**************Begin new trust node proc****************************\n";
    	fd=open(audit_filename,O_WRONLY|O_CREAT|O_APPEND);
    	if(fd<0)
		return -ENOENT;
	write(fd,beginstr,strlen(beginstr));
	close(fd);
	return 0;
}

int     proc_audit_log (void * message)
{
	int ret;
	char *isostr="\n************************************************************\n";
	char audit_text[4096];
        const char * audit_filename= "./message.log";
    	int fd ;
	ret=message_2_json(message,audit_text);	

	audit_text[ret]='\n';			
    	fd=open(audit_filename,O_WRONLY|O_APPEND);
    	if(fd<0)
		return -ENOENT;
	write(fd,audit_text,ret+1);
	write(fd,isostr,strlen(isostr));
	close(fd);
}

			
int proc_router_recv_msg(void * message,char * local_uuid,char * proc_name)
{
	void * sec_sub;
	int ret;
	MSG_HEAD * msg_head;

	if(message_get_state(message) & MSG_FLAG_LOCAL)
		return 0;

	if(message_get_state(message) & MSG_FLOW_DELIVER)
	{
		return 0;
	}

	if(message_get_state(message) & MSG_FLOW_RESPONSE)
	{
		
		ret=router_check_sitestack(message,"FTRE");
		if(ret<0)
			return ret;
		// if response stack finished, set the state to FINISH 
		if(ret==0)
			message_set_state(message, MSG_FLOW_FINISH);
	}
	if(message_get_flow(message) & MSG_FLOW_ASPECT)
	{
		
		ret=router_check_sitestack(message,"APRE");
		if(ret<0)
			return ret;
		if(ret==0)
		{
			// if aspect stack finished, remove the aspect flag from state 
			int flow=message_get_flow(message) &(~MSG_FLOW_ASPECT);
			message_set_flow(message, flow);
		}
	}
	return 0;
}

int proc_router_send_msg(void * message,char * local_uuid,char * proc_name)
{
	void * sec_sub;
	int ret;
	MSG_HEAD * msg_head;
	BYTE conn_uuid[DIGEST_SIZE*2];
	switch(message_get_state(message))
	{
		case MSG_FLAG_LOCAL:
		
			msg_head=get_message_head(message);
			if(msg_head==NULL)
			{
				return  -EINVAL;
			}
			ret=find_ex_module(msg_head->receiver_uuid,&sec_sub);	
			if(sec_sub!=NULL)
			{
//				if(sec_subject_getprocstate(sec_sub)<SEC_PROC_START)
//				{	
//					printf("start process %s!\n",sec_subject_getname(sec_sub));
  //  					ret=sec_subject_start(sec_sub,NULL);
//				}
				send_ex_module_msg(sec_sub,message);
			}
			break;

		case MSG_FLOW_DELIVER:
		case MSG_FLOW_QUERY:
		case MSG_FLOW_RESPONSE:
		case MSG_FLOW_ASPECT:
			ret=find_ex_module("connector_proc",&sec_sub);	
			if(sec_sub==NULL)
			{
				printf("can't find conn process!\n");
				return -EINVAL;
			}
			send_ex_module_msg(sec_sub,message);
			printf("send message to conn process!\n");
				
			break;
		default:
			return -EINVAL;

	}
	return 0;
}

int proc_router_init(void * sub_proc,void * para)
{
    int ret;
    // main proc: read router config	
    char * config_filename= "./router_policy.cfg";
   if(para!=NULL)
        config_filename= para;

    router_policy_init();
    ret=router_read_cfg(config_filename);	
    if(ret<=0)
    {
	    printf("read router policy error %d!\n",ret);
//	    return ret;
    }

    void * context;
    ret=sec_subject_getcontext(sub_proc,&context);
    if(ret<0)
	return ret;
    proc_audit_init();
    return 0;
}


int proc_router_start(void * sub_proc,void * para)
{
	int ret;
	int retval;
	void * message_box;
	MSG_HEAD * message_head;
	void * context;
	int i,j;
	char local_uuid[DIGEST_SIZE*2+1];
	char proc_name[DIGEST_SIZE*2+1];
	char receiver_uuid[DIGEST_SIZE*2+1];
	BYTE conn_uuid[DIGEST_SIZE*2];
	
	ret=proc_share_data_getvalue("uuid",local_uuid);
	ret=proc_share_data_getvalue("proc_name",proc_name);



//	struct timeval time_val={0,10*1000};
	struct timeval router_val;
	router_val.tv_usec=time_val.tv_usec;
	comp_proc_uuid(local_uuid,proc_name,conn_uuid);

	// message routing loop
	for(i=0;i<5000*1000;i++)
	{
		usleep(router_val.tv_usec);
		void * sub_proc;
		char * receiver_proc;
		char * sender_proc;
		void * msg_policy;
		void * aspect_policy;
		char aspect_proc[DIGEST_SIZE*2];
		char * origin_proc;

		// throughout all the sub_proc
		ret=get_first_sec_subject(&sub_proc);
		msg_policy=NULL;
		while(sub_proc!=NULL)
		{
			void * message;
			void * router_rule;
			int state;
			int flow;

			int send_state;
			int hit_target;

			enum proc_type curr_proc_type;

			// this pro is a port proc
			curr_proc_type=sec_subject_gettype(sub_proc);

				
			// receiver an outside message
			ret=recv_ex_module_msg(sub_proc,&message);
			if(ret<0)
			{
				get_next_ex_module(&sub_proc);
				continue;
			}
			else if((message==NULL) ||IS_ERR(message))
			{
				get_next_ex_module(&sub_proc);
				continue;	
			}
			origin_proc=ex_module_getname(sub_proc);

			printf("router get proc %.64s's message!\n",origin_proc); 
			
			//duplicate active message's info and init policy
			router_dup_activemsg_info(message);
			MSG_HEAD * msg_head;
			msg_head=message_get_head(message);
			msg_policy=NULL;
			aspect_policy=NULL;


			if(msg_head->flow==MSG_FLOW_ASPECT)
			{
				// enter the ASPECT route process
				if( msg_head->flag & MSG_FLAG_RESPONSE)	
				{
					msg_head->ljump=0;	
						// if this message's flow is query, we should push it into the stack

					if(msg_head->rjump==0)
					{
						router_recover_route(message);
						printf("recover from aspect\n");
					}	
					else
					{
						msg_head->rjump--;
						router_pop_aspect_site(message,origin_proc);
						printf("begin to response aspect message\n");
					}
				}
				else
				{
					switch(curr_proc_type){
						case PROC_TYPE_CONN:
						case PROC_TYPE_PORT:
						{
							// remote aspect receive
							ret=router_find_aspect_policy(message,&aspect_policy,sub_proc);
							if(ret<0)
							{
								printf("%s check aspect policy error!\n",sub_proc); 
								return ret;
							}
							ret=router_set_aspect_flow(message,aspect_policy);
							if((msg_head->state ==MSG_FLOW_DELIVER)
								&&!(msg_head->flag & MSG_FLAG_RESPONSE))	
							{
								msg_head->rjump++;
								router_push_aspect_site(message,origin_proc,conn_uuid);
							}
							msg_head->ljump=1;
							break;
						}
						case PROC_TYPE_MONITOR:
						case PROC_TYPE_DECIDE:
						case PROC_TYPE_CONTROL:
						{
							ret=router_set_next_jump(message);
							if(ret<0)
								return ret;
							if(ret==0)
							{
							// we met the aspect's end, we should pop the src from stack
								msg_head->state=MSG_FLOW_DELIVER;
								router_pop_aspect_site(message,aspect_proc);
								printf("begin to response aspect message");
								msg_head->flag|=MSG_FLAG_RESPONSE;
								msg_head->ljump--;
								break;
							}
							else if((msg_head->state ==MSG_FLOW_DELIVER)
								&&!(msg_head->flag & MSG_FLAG_RESPONSE))	
							{
								msg_head->rjump++;
								router_push_aspect_site(message,origin_proc,conn_uuid);
								break;
							}
						}
						default:
							return -EINVAL;
					}	
				}	
				proc_audit_log(message);
				printf("aspect message %s is send to %s!\n",message_get_recordtype(message),message_get_receiver(message));
				ret=proc_router_send_msg(message,local_uuid,proc_name);
				if(ret<0)
					return ret;
				
			}
			else
			{
				// message receive process
				switch(curr_proc_type)
				{
					case PROC_TYPE_PORT:

					// if this message is from a port, we find a policy match it and turned it to local message
						ret=router_find_route_policy(message,&msg_policy,sub_proc);	
						if(ret<0)
							return ret;
						if(msg_policy==NULL)
						{
							proc_audit_log(message);
							printf("message %s is discarded in FINISH state!\n",message_get_recordtype(message));
							break;
						}
						if(router_policy_gettype(msg_policy)==MSG_FLOW_QUERY) 
						{
							// if this message's flow is query, we should push it into the stack
							router_push_site(message,origin_proc,"FTRE");
						}
						 
						ret=router_set_local_route(message,msg_policy);
						if(ret<0)
							return ret;
						break;
					case PROC_TYPE_CONN:
						// if it is in response route
						if((msg_head->route[0]!=0)
							&&(msg_head->flow ==MSG_FLOW_QUERY)
							&&(msg_head->flag &MSG_FLAG_RESPONSE))
						{
							msg_head->ljump=0;	
							msg_head->rjump--;
								// if this message's flow is query, we should push it into the stack
							router_pop_site(message,"FTRE");
							printf("begin to response query message");
							break;
						}
						else
						{
							ret=router_find_route_policy(message,&msg_policy,sub_proc);	
							if(ret<0)
							{	
								printf("%s get error message!\n",sub_proc); 
								return ret;
							}
							if(msg_policy==NULL)
							{
								proc_audit_log(message);
								printf("message %s is discarded in FINISH state!\n",message_get_recordtype(message));
								msg_head->flow=MSG_FLOW_FINISH;
								break;
							}
							ret=router_set_local_route(message,msg_policy);
							if(ret<0)
								return ret;

							break;
						}
					case PROC_TYPE_MONITOR:
					case PROC_TYPE_CONTROL:
					case PROC_TYPE_DECIDE:
					{
						if(msg_head->route[0]!=0)
						{
							// this is a message in local dispatch
							msg_head->ljump++;	
							ret=router_set_next_jump(message);
							if(ret<0)
								break;
							else if(ret==0)
							{
							// we met the route's end, look if this message is a query message 
								if(msg_head->flow==MSG_FLOW_QUERY) 
								{
									// if this message's flow is query, we should push it into the stack
									router_pop_site(message,"FTRE");
									printf("begin to response query message");
									msg_head->flag|=MSG_FLAG_RESPONSE;
									msg_head->ljump--;
									break;
								}
							}
						}
						else
						{
							ret=router_find_route_policy(message,&msg_policy,sub_proc);	
							if(ret<0)
							{	
								printf("%s get error message!\n",sub_proc); 
								return ret;
							}
							if(msg_policy==NULL)
							{
								proc_audit_log(message);
								printf("message %s is discarded in FINISH state!\n",message_get_recordtype(message));
								msg_head->flow=MSG_FLOW_FINISH;
								break;
							}
							ret=router_set_local_route(message,msg_policy);
							if(ret<0)
								return ret;

							break;
							// this is a source message
						}
					}
						break;
					default:
						break;
				}


				if(msg_head->flow==MSG_FLOW_FINISH)
					continue;

				if(msg_policy!=NULL)
				{
					// duplicate message send process
					router_rule=router_get_first_duprule(msg_policy);
					while(router_rule!=NULL)
					{
						void * dup_msg;
						ret=router_set_dup_flow(message,router_rule,&dup_msg);
						if(ret<0)
							break;
						if(dup_msg!=NULL)
						{
							proc_audit_log(dup_msg);
							proc_router_send_msg(dup_msg,local_uuid,proc_name);
						}
						router_rule=router_get_next_duprule(msg_policy);
					}
				}
				if((msg_head->state==MSG_FLOW_DELIVER)
					||(curr_proc_type==PROC_TYPE_CONN))
				{
					// remote message send process
					// if this message is a query message, we should record its trace
					if((msg_head->state==MSG_FLOW_DELIVER)
						&&!(msg_head->flag &MSG_FLAG_RESPONSE))
					{
						msg_head->rjump++;
						if(msg_head->flow==MSG_FLOW_QUERY)
						{
							router_push_site(message,conn_uuid,"FTRE");
						}
					}
					ret=router_find_aspect_policy(message,&aspect_policy,sub_proc);
					if(ret<0)
					{
						printf("%s check aspect policy error!\n",sub_proc); 
						return ret;
					}
					if(aspect_policy!=NULL)
					{
						ret=router_store_route(message);
						if(ret<0)
						{
							printf("router store route for aspect error!\n");
						}
						ret=router_set_aspect_flow(message,aspect_policy);
						msg_head->rjump=0;
						router_push_aspect_site(message,origin_proc,conn_uuid);
					}	
				}
				proc_audit_log(message);
				printf("message %s is send to %s!\n",message_get_recordtype(message),message_get_receiver(message));
				ret=proc_router_send_msg(message,local_uuid,proc_name);
				if(ret<0)
					return ret;
			}

			continue;
		}
	
	}
	return 0;
}
