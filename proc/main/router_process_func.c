#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "data_type.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "extern_struct.h"
#include "extern_defno.h"
#include "message.h"
#include "memdb.h"
#include "sec_entity.h"

#include "router_process_func.h"

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
	MESSAGE_HEAD * msg_head;

	if(message_get_state(message) & MSG_FLOW_LOCAL)
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
	MESSAGE_HEAD * msg_head;
	BYTE conn_uuid[DIGEST_SIZE*2];
	switch(message_get_state(message))
	{
		case MSG_FLOW_LOCAL:
		case MSG_FLOW_ASPECT_LOCAL:
		
			msg_head=get_message_head(message);
			if(msg_head==NULL)
			{
				return  -EINVAL;
			}
			ret=find_sec_subject(msg_head->receiver_uuid,&sec_sub);	
			if(sec_sub!=NULL)
			{
				if(sec_subject_getprocstate(sec_sub)<SEC_PROC_START)
				{	
					printf("start process %s!\n",sec_subject_getname(sec_sub));
    					ret=sec_subject_start(sec_sub,NULL);
				}
				send_sec_subject_msg(sec_sub,message);
				printf("send message to local process %s!\n",msg_head->receiver_uuid);
			}
			break;

		case MSG_FLOW_DELIVER:
		case MSG_FLOW_QUERY:
		case MSG_FLOW_RESPONSE:
		case MSG_FLOW_ASPECT:
		case MSG_FLOW_ASPECT_RETURN:
		
			ret=find_sec_subject("connector_proc",&sec_sub);	
			if(sec_sub==NULL)
			{
				printf("can't find conn process!\n");
				return -EINVAL;
			}
			send_sec_subject_msg(sec_sub,message);
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
	enum send_state_describe
	{	
		STATE_NONE,
		STATE_LOCAL,
		STATE_SEND,
		STATE_TRANS,
		STATE_HIT,
		STATE_RECV,
		STATE_ASPECT,
		STATE_ASPECT_LOCAL,
		STATE_ASPECT_RETURN,
		STATE_FINISH,
		STATE_ERROR,

	};
	int ret;
	int retval;
	void * message_box;
	MESSAGE_HEAD * message_head;
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


			// receiver the message
			ret=recv_sec_subject_msg(sub_proc,&message);
			if(ret<0)
			{
				get_next_sec_subject(&sub_proc);
				continue;
			}
			else if((message==NULL) ||IS_ERR(message))
			{
				get_next_sec_subject(&sub_proc);
				continue;	
			}

			origin_proc=sec_subject_getname(sub_proc);

			printf("router get proc %.64s's message!\n",origin_proc); 
			
			router_dup_activemsg_info(message);

			send_state=STATE_RECV;

			switch(message_get_state(message))
			{
				case MSG_FLOW_INIT:
					send_state=STATE_LOCAL;
					break;
				case MSG_FLOW_LOCAL:
					if(message_get_flow(message)&MSG_FLOW_RESPONSE)
					{	
						ret=router_check_sitestack(message,"FTRE");
						if(ret<0)
						{
							send_state=STATE_ERROR;
							break;
						}
						if(ret==0)
						{
							message_set_state(message,MSG_FLOW_INIT);
							message_set_flow(message,MSG_FLOW_INIT);
							send_state=STATE_LOCAL;
						}
						else
						{
							router_pop_site(message,"FTRE");
							send_state=STATE_ASPECT_LOCAL;
						}

					}
					else
					{
						send_state=STATE_LOCAL;
					}
					break;
				case MSG_FLOW_DELIVER:
					hit_target=proc_router_hit_target(message,local_uuid,proc_name);
					if(hit_target<0)
					{
						send_state=STATE_ERROR;
						break;
					}
					else if(hit_target==0)
					{
						send_state=STATE_ASPECT_LOCAL;
						break;
					}
					message_set_state(message,MSG_FLOW_LOCAL);
					flow=message_get_flow(message);
					message_set_flow(message,(flow&(~MSG_FLOW_DELIVER))|MSG_FLOW_DRECV);
					send_state=STATE_RECV;
					break;
				case MSG_FLOW_QUERY:
					hit_target=proc_router_hit_target(message,local_uuid,proc_name);
					if(hit_target<0)
					{
						send_state=STATE_ERROR;
						break;
					}
					else if(hit_target==0)
					{
						router_push_site(message,conn_uuid,"FTRE");
						send_state=STATE_ASPECT_LOCAL;
						break;
					}
					// query message reach the target, so we should cancel the query flow and add the response flow,
					// and set state to local to wait a local plugin to deal with the message  
					message_set_state(message,MSG_FLOW_LOCAL);
					flow=message_get_flow(message);
					message_set_flow(message,(flow&(~MSG_FLOW_QUERY))|MSG_FLOW_QRECV);
					send_state=STATE_RECV;
					break;
				case MSG_FLOW_RESPONSE:
					ret=router_check_sitestack(message,"FTRE");
					if(ret<0)
					{
						send_state=STATE_ERROR;
						break;
					}
					if(ret==0)
					{
						flow=message_get_flow(message);
						message_set_flow(message,flow&(~MSG_FLOW_RESPONSE)|MSG_FLOW_QRECV);
						message_set_state(message,MSG_FLOW_LOCAL);
					//	send_state=STATE_RECV;
						send_state=STATE_ASPECT_LOCAL;
					}
					else
					{
						router_pop_site(message,"FTRE");
						send_state=STATE_ASPECT_LOCAL;
					}
					break;
				case MSG_FLOW_ASPECT_LOCAL:
					flow=message_get_flow(message);
					if(flow & MSG_FLOW_ASPECT_RETURN)
					{
						message_set_flow(message,flow&(~MSG_FLOW_ASPECT_LOCAL));
						ret=router_pop_aspect_site(message,aspect_proc);
						send_state=STATE_ASPECT_RETURN;
						message_set_state(message,MSG_FLOW_ASPECT_RETURN);
						break;
					}
					ret=router_pop_aspect_site(message,aspect_proc);
					send_state=STATE_ASPECT;
					break;
				case MSG_FLOW_ASPECT:
					// first, check if the aspect router reach the target
					hit_target=proc_router_hit_target(message,local_uuid,proc_name);
					if(hit_target<0)
					{
						send_state=STATE_ERROR;
						break;
					}
					else if(hit_target==0)
					{
						// if aspect router do not reach the target, push the node's uuid and try to find the local aspect policy
						send_state=STATE_ASPECT_LOCAL;
						router_push_aspect_site(message,origin_proc,conn_uuid);
						break;
					}
					// if aspect router reach the target,set flow to ASPECT_RETURN, set state to ASPECT_LOCAL
					// pop an address in aspect stack and try to find the local aspect policy
					// if can't find an aspect local policy, this message will be discarded   
					message_set_state(message,MSG_FLOW_ASPECT_LOCAL);
					flow=message_get_flow(message);
					message_set_flow(message,(flow&(~MSG_FLOW_ASPECT))|(MSG_FLOW_ASPECT_RETURN));
					send_state=STATE_ASPECT_LOCAL;
					break;
				case MSG_FLOW_ASPECT_RETURN:
					flow = message_get_flow(message);
					ret=router_check_sitestack(message,"APRE");
					if(ret<0)
					{
						message_free(message);
						send_state=STATE_ERROR;
						break;
					}
					else if(ret==0)
					{
						// if aspect stack finished, remove the aspect flag from state 
//						flow = message_get_flow(message);
						send_state=STATE_FINISH;
						break;
					}
					else if(ret>1)
					{
//						flow = message_get_flow(message);
//						message_set_flow(message, flow&(~MSG_FLOW_ASPECT_RETURN));
						ret=router_pop_aspect_site(message,aspect_proc);
						send_state=STATE_TRANS;
						break;
					}

					flow &=(~MSG_FLOW_ASPECT_RETURN);
					message_set_flow(message, flow);
					ret=router_pop_aspect_site(message,aspect_proc);

					if(flow&MSG_FLOW_QRECV)
					{
						message_set_state(message,MSG_FLOW_FINISH);
						message_set_flow(message, flow&(~MSG_FLOW_QRECV)|MSG_FLOW_RESPONSE);
						send_state=STATE_FINISH;
					}
					if(flow&MSG_FLOW_DELIVER)
					{
						message_set_state(message,MSG_FLOW_DELIVER);
						send_state=STATE_TRANS;
					}
					else if (flow&MSG_FLOW_QUERY)
					{
						message_set_state(message,MSG_FLOW_QUERY);
						send_state=STATE_TRANS;
					}
					else if (flow&MSG_FLOW_RESPONSE)
					{
						message_set_state(message,MSG_FLOW_RESPONSE);
						send_state=STATE_TRANS;
					}
					else if (flow&MSG_FLOW_DRECV)
					{
						message_set_state(message,MSG_FLOW_DRECV);
						send_state=STATE_FINISH;
					}
					break;
				case MSG_FLOW_FINISH:
					send_state=STATE_FINISH;
					break;
				default:
					send_state=STATE_ERROR;
					break;
			}
			while( ( send_state != STATE_TRANS)
					&&(send_state!= STATE_ERROR))
			{
				switch(send_state)
				{
					case STATE_LOCAL:
						ret=router_find_local_policy(message,&msg_policy,origin_proc);
						if(ret<0)
						{
							send_state=STATE_ERROR;
							break;
						}
						if(msg_policy!=NULL)
						{
							ret=router_set_main_flow(message,msg_policy);
							if(ret<0)
							{
								send_state=STATE_ERROR;
								break;
							}
							send_state=STATE_TRANS;
							break;
						}
						send_state=STATE_SEND;
						break;
					case STATE_SEND:
						ret=router_find_match_policy(message,&msg_policy,origin_proc);
						if(ret<0)
						{
							send_state=STATE_ERROR;
							break;
						}
						if(msg_policy==NULL)
						{
							printf("message %s is discarded in SEND step!\n",message_get_recordtype(message));
							send_state=STATE_ERROR;
							break;
						}
						if(router_policy_gettype(msg_policy)&MSG_FLOW_QUERY)
							router_push_site(message,conn_uuid,"FTRE");
						ret=router_set_main_flow(message,msg_policy);
						if(ret<0)
						{
							send_state=STATE_ERROR;
							break;
						}
						send_state=STATE_ASPECT_LOCAL;
						break;
					case STATE_ASPECT_LOCAL:
						ret=router_find_aspect_local_policy(message,&aspect_policy,origin_proc);
						if(ret<0)
						{
							send_state=STATE_ERROR;
							break;
						}
						if(aspect_policy==NULL)
						{
							send_state=STATE_ASPECT;
							break;
						}
						flow=message_get_flow(message);
						message_set_flow(message,flow | MSG_FLOW_ASPECT_LOCAL);
						
						if(!(flow & MSG_FLOW_ASPECT_RETURN))
							router_push_aspect_site(message,sec_subject_getname(sub_proc),message_get_receiver(message));
						ret=router_set_aspect_local_flow(message,aspect_policy);
						if(ret<0)
						{
							send_state=STATE_ERROR;
							break;
						}
						message_set_state(message,MSG_FLOW_ASPECT_LOCAL);
						send_state=STATE_TRANS;
						break;
					case STATE_ASPECT:
						flow=message_get_flow(message);
						if(flow & MSG_FLOW_ASPECT_LOCAL)
						{
							message_set_flow(message,flow&(~MSG_FLOW_ASPECT_LOCAL));
							ret=router_find_aspect_policy(message,&aspect_policy,aspect_proc);
						}
						else
						{
							ret=router_find_aspect_policy(message,&aspect_policy,origin_proc);
						}

						if(ret<0)
						{
							send_state=STATE_ERROR;
							break;
						}
						if(aspect_policy==NULL)
						{
							if(flow & MSG_FLOW_ASPECT)
							{
								message_set_state(message,MSG_FLOW_ASPECT_RETURN);
								message_set_flow(message,flow&(~MSG_FLOW_ASPECT)|MSG_FLOW_ASPECT_RETURN);
								send_state=STATE_TRANS;
								break;
							}
							if(flow & MSG_FLOW_ASPECT_LOCAL)
							{
								ret=router_pop_aspect_site(message,aspect_proc);
								message_set_flow(message,flow&(~MSG_FLOW_ASPECT_LOCAL));
								origin_proc=aspect_proc;
							}
							if(flow & MSG_FLOW_QRECV)
							{
								message_set_state(message,MSG_FLOW_FINISH);
								send_state=STATE_FINISH;
								break;
							}
							if(flow & MSG_FLOW_DRECV)
							{
								message_set_state(message,MSG_FLOW_FINISH);
								send_state=STATE_FINISH;
								break;
							}
							if(flow & MSG_FLOW_RESPONSE)
							{
								message_set_state(message,MSG_FLOW_RESPONSE);
							}
							else if(flow & MSG_FLOW_QUERY)
							{
								message_set_state(message,MSG_FLOW_QUERY);
							}
							send_state=STATE_TRANS;
							break;

						}
						// find an aspect policy, we need set the aspect flow flag
						message_set_state(message,MSG_FLOW_ASPECT);
						// check if the aspect stack exist
						ret=router_check_sitestack(message,"APRE");
						if(ret<0)
						{
							send_state=STATE_ERROR;
							break;
						}
						if(ret==0)
						{
							// if the aspect stack not exist, we should push message's receiver id first
							if(flow &MSG_FLOW_ASPECT_LOCAL)
								router_push_aspect_site(message,aspect_proc,message_get_receiver(message));
							else
								router_push_aspect_site(message,origin_proc,message_get_receiver(message));
						}
						// push current trust node's uuid
						if(flow &MSG_FLOW_ASPECT_LOCAL)
							router_push_aspect_site(message,aspect_proc,conn_uuid);
						else
							router_push_aspect_site(message,origin_proc,conn_uuid);
						// set the aspect policy 
						ret=router_set_aspect_flow(message,aspect_policy);
						if(ret<0)
						{
							send_state=STATE_ERROR;
							break;
						}
						// prepare to trans
						send_state=STATE_TRANS;
						break;
					case STATE_ASPECT_RETURN:
						send_state=STATE_TRANS;
						break;
					case STATE_RECV:
						send_state=STATE_ASPECT_LOCAL;
						break;
					case STATE_TRANS:
						break;
					case STATE_FINISH:
						ret=router_find_local_policy(message,&msg_policy,origin_proc);
						if(ret<0)
						{
							send_state=STATE_ERROR;
							break;
						}
						if(msg_policy!=NULL)
						{
							ret=router_set_main_flow(message,msg_policy);
							if(ret<0)
							{
								send_state=STATE_ERROR;
								break;
							}
							flow=message_get_flow(message);
							if(flow&MSG_FLOW_QRECV)
							{
								ret=message_set_flow(message,flow&(~MSG_FLOW_QRECV)|MSG_FLOW_RESPONSE);
							}
							if(ret<0)
							{
								send_state=STATE_ERROR;
								break;
							}
							send_state=STATE_TRANS;
							break;
						}
						printf("message %s is discarded in FINISH state!\n",message_get_recordtype(message));
						send_state=STATE_ERROR;
						break;
					default:
						send_state=STATE_ERROR;
						break;
				}
			}
			if(send_state==STATE_ERROR)
			{
				proc_audit_log(message);
				message_free(message);
				continue;
			}
			if(msg_policy!=NULL)
			{
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

			proc_audit_log(message);

			ret=proc_router_send_msg(message,local_uuid,proc_name);
			if(ret<0)
			{
				printf("router send message to main flow failed!\n");
			}
		
			continue;
		}
	
	}
	return 0;
}
