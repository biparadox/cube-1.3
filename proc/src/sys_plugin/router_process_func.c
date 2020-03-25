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
#include "list.h"
#include "attrlist.h"
#include "memfunc.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "basefunc.h"
#include "memdb.h"
#include "message.h"
#include "dispatch.h"
#include "ex_module.h"

#include "sys_func.h"
#include "message_box.h"
#include "route_tree.h"
#include "router_process_func.h"

int read_dispatch_file(char * file_name,int is_aspect)
{
	int ret;

	int fd;
	int readlen;
	int json_offset;

	int count=0;
	void * root_node;
	void * findlist;
	void * memdb_template ;
	BYTE uuid[DIGEST_SIZE];
	char json_buffer[4097];
	void * policy;
	int finishread=0;
	int leftlen=0;

	fd=open(file_name,O_RDONLY);
	if(fd<0)
		return fd;

	readlen=read(fd,json_buffer,4096);
	if(readlen<0)
		return -EIO;
	json_buffer[readlen]=0;
	if(readlen<4096)
		finishread=1;

	leftlen=readlen;

	json_offset=0;

	while(leftlen>DIGEST_SIZE)
	{
		readlen=json_solve_str(&root_node,json_buffer);
		if(readlen<0)
		{
			print_cubeerr("solve policy %s error!\n",file_name);
			break;
		}
		json_offset+=readlen;
		Memcpy(json_buffer,json_buffer+readlen,leftlen-readlen);
		leftlen-=readlen;
		json_buffer[leftlen]=0;

		if(finishread==0)
		{
			ret=read(fd,json_buffer+leftlen,4096-leftlen);
			if(ret<4096-leftlen)
				finishread=1;
			leftlen+=ret;	
		}	
	
		if(readlen<32)
			continue;

		policy=route_read_policy(root_node);
		if(policy==NULL)
		{
			print_cubeerr("read %d file error!",count);
			break;
		}
		if((route_path_getstate(policy)==POLICY_IGNORE)
			||(route_path_getstate(policy)==POLICY_CLOSE))
		{
			char * policyname =route_path_getname(policy);
			print_cubeaudit("policy %s is ignored!",policyname);
		}
		else
		{
			route_add_policy(policy);
			count++;
		}
	}
	
	//print_cubeaudit("read %d policy succeed!",count);
	close(fd);
	return count;
}

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
//	if(deep_debug==0)
//		return 0;

	int ret;
	char *isostr="\n************************************************************\n";
	char audit_text[4096];
        const char * audit_filename= "./message.log";
    	int fd ;
	ret=message_output_json(message,audit_text);	

	audit_text[ret]=0;			
   	fd=open(audit_filename,O_WRONLY|O_APPEND);
   	if(fd<0)
		return -ENOENT;
	print_pretty_text(audit_text,fd);
	//write(fd,audit_text,ret+1);
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
		
//		ret=route_check_sitestack(message);
		if(ret<0)
			return ret;
		// if response stack finished, set the state to FINISH 
		if(ret==0)
			message_set_state(message, MSG_FLOW_FINISH);
	}
	if(message_get_flow(message) & MSG_FLOW_ASPECT)
	{
		
//		ret=route_check_aspectstack(message);
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
	if(message_get_flag(message) & MSG_FLAG_LOCAL)
	{
		
			msg_head=message_get_head(message);
			if(msg_head==NULL)
			{
				return  -EINVAL;
			}
			ret=find_ex_module(msg_head->receiver_uuid,&sec_sub);	
			if(sec_sub!=NULL)
			{
//				if(sec_subject_getprocstate(sec_sub)<SEC_PROC_START)
//				{	
//					print_cubeaudit("start process %s!\n",sec_subject_getname(sec_sub));
  //  					ret=sec_subject_start(sec_sub,NULL);
//				}
				send_ex_module_msg(sec_sub,message);
			}
			else
			{
				print_cubeerr("proc_router_send_msg: can't find local message target %s",msg_head->receiver_uuid);
			}
	}
	else
	{
	
			ret=find_ex_module("connector_proc",&sec_sub);	
			if(sec_sub==NULL)
			{
				print_cubeerr("can't find conn process!\n");
				return -EINVAL;
			}
			send_ex_module_msg(sec_sub,message);
			print_cubeaudit("send message to conn process!");
				
	}
	return 0;
}

int proc_router_init(void * sub_proc,void * para)
{

    int ret;
    // main proc: read router config	
    char * config_filename= "./dispatch_policy.json";
    char * aspect_filename=NULL;
   if(para!=NULL)
   {
	struct router_init_para * init_para=para;
        config_filename= init_para->router_file;
        aspect_filename= init_para->aspect_file;
    }	

 // router_policy_init();
    ret=read_dispatch_file(config_filename,0);	
    if(ret<=0)
    {
	    print_cubeerr("read router policy error %d!",ret);
//	    return ret;
    }
	else
		print_cubeaudit("read %d policy from router_policy file!",ret);
    ret=read_dispatch_file(aspect_filename,1);	
    if(ret<=0)
    {
	    print_cubewarn("read aspect policy failed %d!",ret);
//	    return ret;
    }
	else
		print_cubeaudit("read %d policy from aspect policy file!",ret);

    proc_audit_init();
    return 0;
}


int proc_router_start(void * sub_proc,void * para)
{
	int ret;
	int retval;
	struct message_box * message_box;
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
	router_val.tv_sec=time_val.tv_sec;
	router_val.tv_usec=time_val.tv_usec;
	comp_proc_uuid(local_uuid,proc_name,conn_uuid);

	// message routing loop
	

	while(1)
	{
	// throughout all the sub_proc
		


		usleep(router_val.tv_usec);

		void * sub_proc;
		char * receiver_proc;
		char * sender_proc;
		void * msg_policy;
		void * aspect_policy;
		char aspect_proc[DIGEST_SIZE*2];
		char  origin_proc[DIGEST_SIZE];
		struct message_box * message;
		MSG_HEAD * msg_head;

		if(proc_share_data_getstate()<PROC_LOCAL_START)
			continue;
		msg_policy=NULL;
		ret=get_first_ex_module(&sub_proc);
		while(sub_proc!=NULL)
		{
			void * router_rule;
			int state;
			int flow;

			int send_state;
			int hit_target;

			enum module_type curr_proc_type;

			// this pro is a port proc
			curr_proc_type=ex_module_gettype(sub_proc);

			// check if sub_proc has message 
			ret=recv_ex_module_msg(sub_proc,&message);
			if(ret<0)
			{
				// get next sub_proc
				get_next_ex_module(&sub_proc);
				continue;
			}
			else if((message==NULL) ||IS_ERR(message))
			{
				get_next_ex_module(&sub_proc);
				continue;	
			}

			Strncpy(origin_proc,ex_module_getname(sub_proc),DIGEST_SIZE);
			print_cubeaudit("router get proc %.64s's message ",origin_proc); 



			// message's value reset
			if(Strcmp(origin_proc,"connector_proc")!=0)
			{
				message_set_sender(message,origin_proc);
			}
		
			// if message is init message

			router_dup_activemsg_info(message);

			message_set_activemsg(message,message);
			msg_head=message_get_head(message);
			// set rjump's value
			switch(ex_module_gettype(sub_proc))
			{
				case MOD_TYPE_CONN:
					msg_head->ljump=1;	
					if(msg_head->state!=MSG_STATE_RESPONSE)
						msg_head->rjump++;
					else
						msg_head->rjump--;
					break;
				case MOD_TYPE_PORT:
					msg_head->ljump=1;	
					msg_head->rjump=1;
					break;
				default:
					msg_head->ljump++;	
					if(msg_head->rjump==0)
						msg_head->rjump=1;
					break;
			}
		
			// enter the message route match process
			

			if(message->policy!=NULL)
			// if message has path already
			{
				_waiting_message_add(message);
			}
			// if message is init message
			else if(msg_head->flow == 0)
			{
				// finding the match policy 
				ret=router_find_route_policy(message,&msg_policy);
				if(ret<0)
				{
					print_cubeerr("Fatal error in finding policy !");
					return ret;
				}
				// if can't find match policy
				if(msg_policy==NULL)
				{
					msg_head->flow=MSG_FLOW_FINISH;
					proc_audit_log(message);
					get_next_ex_module(&sub_proc);
					continue;
				}
				// show the debug info
				print_cubeaudit("new msg match path %.64s's policy",route_path_getname(msg_policy)); 
				message_route_setstart(message,msg_policy);
				_waiting_message_add(message);
			}	
			else if (~(msg_head->state & MSG_STATE_RESPONSE))
			// message is not return message 
			{
				ret=message_route_setremotestart(message);
				if(ret<0)
				{
					print_cubeerr("Fatal error in remotestart!");
					return ret;
				}
				if(message->policy==NULL)
				{
					msg_head->flow=MSG_FLOW_FINISH;
					proc_audit_log(message);
					get_next_ex_module(&sub_proc);
					continue;
				}
				print_cubeaudit("remote msg match path %.64s's policy",msg_head->route);
				_waiting_message_add(message);
			}
			// message is a return message
			else
			{
				// we should find message's response record and reload the path

			} 
			get_next_ex_module(&sub_proc);
		};
		
		message=_waiting_message_removehead();

		int issend;
		int isaspect;
		while(message!=NULL)
		{		
			issend=0;
			isaspect=0;

			msg_head=message_get_head(message);
			if(msg_head->flow==MSG_FLOW_DELIVER)
			{
				ret=message_route_setnext(message);
				if(ret!=0)
				{
					// there is a target in the next	
					proc_audit_log(message);
					issend=1;
				}	
				else
				{
					message->head.flow=MSG_FLOW_FINISH;
					proc_audit_log(message);
				}
			}
			else if(msg_head->flow == MSG_FLOW_QUERY)
			{
				ret=message_route_setnext(message);
				if(ret < 0)
				{
					print_cubeerr("route query error!\n");
				}	
				else if((ret==0) ||(ret == ROUTE_TARGET_FINISH))
				{
					if(msg_head->rjump>1)
					{
						ret=message_route_findtrace(message,MSG_FLOW_QUERY);
						if(ret<0)
						{
							proc_audit_log(message);
							print_cubeerr("find trace message (%d %d)failed!\n",message->head.record_type,message->head.record_subtype);
						}
						else
						{
							proc_audit_log(message);
							issend=1;
						}
					}
					else
					{
						ret=message_route_findtrace(message,MSG_FLOW_ASPECT);
						if(ret<0)
						{
							proc_audit_log(message);
							print_cubeerr("find trace message (%d %d)failed!\n",message->head.record_type,message->head.record_subtype);
						}
						else
						{
							proc_audit_log(message);
							issend=1;
							isaspect=1;
						}
					}
				}
				else
				{
					// this is not the end	
					proc_audit_log(message);
					issend=1;
				}
				
			}
			if(issend==1)
			{
				issend=0;
				
				ROUTE_NODE * curr_pathnode;
				BRANCH_NODE * curr_branch;
				Record_List * curr_record;

				curr_record=message->path_site;

				if((curr_record !=NULL) && (curr_record->record!=NULL))
				{
					curr_pathnode=curr_record->record;
					curr_record = _node_list_getfirst(&curr_pathnode->aspect_branch);
	
					while((curr_record!=NULL) &&(curr_record->record!=NULL))
					{
						curr_branch = curr_record->record;
						switch(curr_branch->branch_type)
						{
							case MSG_FLOW_DUP:
								ret=_route_match_aspect_branch(message,curr_branch);	
								if(ret>0)
								{
									struct message_box * new_msg = route_dup_message(message,curr_branch);	
									print_cubeaudit("dup a message to path %.64s's policy",new_msg->head.route);
									_waiting_message_add(new_msg);
								}
								break;
							case MSG_FLOW_ASPECT:
								if(isaspect)
									break;
								ret=_route_match_aspect_branch(message,curr_branch);	
								if(ret>0)
								{
									route_aspect_message(message,curr_branch);
									print_cubeaudit("aspect message to path %.64s's policy",message->head.route);
									_waiting_message_add(message);
									message=NULL;
								}
								break;
							default:
								break;
						}
						curr_record = _node_list_getnext(&curr_pathnode->aspect_branch);
					}	
				}	
				if(message!=NULL)
				{
					if(Memcmp(message->head.receiver_uuid,EMPTY_UUID,DIGEST_SIZE)!=0)
					{
						message->active_msg = message;
						ret=proc_router_send_msg(message,local_uuid,proc_name);
					}
				}
			}
			message=_waiting_message_removehead();
		}

			// test's end
/*



			router_dup_activemsg_info(message);
			
			message_set_activemsg(message,message);

			msg_policy=NULL;
			aspect_policy=NULL;


			if(msg_head->flow==MSG_FLOW_ASPECT)
			{
				// enter the ASPECT route process
				if( msg_head->flag & MSG_FLAG_RESPONSE)	
				{
						// if this message's flow is query, we should push it into the stack
					ret=router_pop_aspect_site(message,origin_proc);
					msg_head->flag&=~MSG_FLAG_LOCAL;
					if(ret==0)
					{

						route_recover_route(message);
						print_cubeaudit("recover from aspect");
					}	
				}
				else
				{
					switch(curr_proc_type){
						case MOD_TYPE_CONN:
						case MOD_TYPE_PORT:
						{
							// remote aspect receive
							ret=router_find_aspect_policy(message,&aspect_policy,origin_proc);
							if(ret<0)
							{
								print_cubeerr("%s check aspect policy error!\n",sub_proc); 

								return ret;
							}
							ret=router_set_aspect_flow(message,aspect_policy);
							//if((msg_head->state ==MSG_FLOW_DELIVER)
							//	&&!(msg_head->flag & MSG_FLAG_RESPONSE))	
							//{
						//		msg_head->rjump++;
						//		route_push_aspect_site(message,origin_proc,conn_uuid);
							//}
							msg_head->ljump=1;
							ret=router_set_next_jump(message);
							if(ret<0)
								continue;
							if((msg_head->state ==MSG_FLOW_DELIVER)
								&&!(msg_head->flag &MSG_FLAG_LOCAL)
								&&!(msg_head->flag & MSG_FLAG_RESPONSE))	
							{
								route_push_aspect_site(message,origin_proc,conn_uuid);
								break;
							}
							break;
						}
						case MOD_TYPE_MONITOR:
						case MOD_TYPE_DECIDE:
						case MOD_TYPE_CONTROL:
						case MOD_TYPE_START:
						{
							ret=router_set_next_jump(message);
							if(ret<0)
								continue;
							if(ret==0)
							{
							// we met the aspect's end, we should pop the src from stack
								msg_head->state=MSG_FLOW_DELIVER;
								router_pop_aspect_site(message,aspect_proc);
								print_cubeaudit("begin to response aspect message");
								msg_head->flag|=MSG_FLAG_RESPONSE;
								msg_head->flag&=~MSG_FLAG_LOCAL;
								break;
							}
							else if((msg_head->state ==MSG_FLOW_DELIVER)
								&&!(msg_head->flag &MSG_FLAG_LOCAL)
								&&!(msg_head->flag & MSG_FLAG_RESPONSE))	
							{
								route_push_aspect_site(message,origin_proc,conn_uuid);
								break;
							}
							break;
						}
						default:
							return -EINVAL;
					}	
				}	
				proc_audit_log(message);
				debug_message(message,"aspect message prepare to send");
				//print_cubeaudit("aspect message (%s) is send to %s!\n",message_get_typestr(message),message_get_receiver(message));
				ret=proc_router_send_msg(message,local_uuid,proc_name);
				if(ret<0)
					return ret;
			}
			else
			{
				// message receive process
				switch(curr_proc_type)
				{
					case MOD_TYPE_PORT:

					// if this message is from a port, we find a policy match it and turned it to local message
						ret=router_find_route_policy(message,&msg_policy,origin_proc);	
						if(ret<0)
							return ret;
						if(msg_policy==NULL)
						{
							msg_head->flow=MSG_FLOW_FINISH;
							break;
						}
						if(dispatch_policy_gettype(msg_policy)==MSG_FLOW_QUERY) 
						{
							// if this message's flow is query, we should push it into the stack
							route_push_site_str(message,origin_proc);
						}
						 
						ret=router_set_local_route(message,msg_policy);
						if(ret<0)
							return ret;
						ret=router_set_next_jump(message);
						if(ret<0)
							return ret;
						break;
					case MOD_TYPE_CONN:
						// if it is in response route
						if((msg_head->route[0]!=0)
							&&(msg_head->flow ==MSG_FLOW_QUERY)
							&&(msg_head->flag &MSG_FLAG_RESPONSE))
						{
								// if this message's flow is query, we should push it into the stack
							ret=router_pop_site(message);
							if(ret==0)
							{
								ret=router_find_policy_byname(&msg_policy,msg_head->route,msg_head->rjump,msg_head->ljump);
								if(ret<0)
								{
									print_cubeerr("can't find response message's route!\n");
									continue;
								}
								ret=router_set_query_end(message,msg_policy);
								if(ret<0)
								{
									print_cubeerr("can't set response message!\n");
									continue;
								}	
								ret=router_set_next_jump(message);
								if(ret<0)
									return ret;
								break;
							}	
							print_cubeaudit("begin to response query message");
							break;
						}
						else
						{
							ret=router_find_route_policy(message,&msg_policy,origin_proc);	
							if(ret<0)
							{	
								print_cubeerr("%s get error message!\n",sub_proc); 
								return ret;
							}
							if(msg_policy==NULL)
							{
								msg_head->flow=MSG_FLOW_FINISH;
								break;
							}
							ret=router_set_local_route(message,msg_policy);
							if(ret<0)
								return ret;
							ret=router_set_next_jump(message);
							if(ret<0)
								return ret;

							break;
						}
					case MOD_TYPE_MONITOR:
					case MOD_TYPE_CONTROL:
					case MOD_TYPE_DECIDE:
					case MOD_TYPE_START:
					{
						if(msg_head->route[0]!=0)
						{
							// this is a message in local dispatch
							ret=router_set_next_jump(message);
							if(ret<0)
								break;
							else if(ret==0)
							{
							// we met the route's end, look if this message is a query message 
								if(msg_head->flow==MSG_FLOW_QUERY) 
								{
									// if this message's flow is query, we should push it into the stack
									router_pop_site(message);
									print_cubeaudit("begin to response query message");
									msg_head->flag|=MSG_FLAG_RESPONSE;
									// if the receiver uuid is a str in response mode, it should be a port module
									// else, it should be a remote proc
									if(!Isstrinuuid(msg_head->receiver_uuid))
										msg_head->flag&=~MSG_FLAG_LOCAL;
									break;
								}
								else
								{
									msg_head->flow=MSG_FLOW_FINISH;
									break;
								}
							}
						}
						else
						// 为本地新消息查找匹配策略
						{
							ret=router_find_route_policy(message,&msg_policy,origin_proc);	
							if(ret<0)
							{	
								print_cubeerr("%s get error message!\n",sub_proc); 
								return ret;
							}
							if(msg_policy==NULL)
							{
								msg_head->flow=MSG_FLOW_FINISH;
								break;
							}
							ret=router_set_local_route(message,msg_policy);
							if(ret<0)
								return ret;
							ret=router_set_next_jump(message);
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
				{
					proc_audit_log(message);
					debug_message(message,"discard message:");
					//print_cubeaudit("message (%s) is discarded in FINISH state!\n",message_get_typestr(message));
					continue;
				}

				if((msg_head->state==MSG_FLOW_DELIVER)
					||(curr_proc_type==MOD_TYPE_CONN))
				{
					// remote message send process
					// if this message is a query message, we should record its trace
					if((msg_head->state==MSG_FLOW_DELIVER)
						&&!(msg_head->flag &MSG_FLAG_RESPONSE))
					{
//						msg_head->rjump++;
						if(msg_head->flow==MSG_FLOW_QUERY)
						{
							if(!(msg_head->flag &MSG_FLAG_LOCAL))
								route_push_site(message,conn_uuid);
						}
					}
				}

				ret=router_find_aspect_policy(message,&aspect_policy,origin_proc);
				if(ret<0)
				{
					print_cubeerr("%s check aspect policy error!\n",sub_proc); 
					return ret;
				}
				if(aspect_policy!=NULL)
				{
					if(dispatch_policy_gettype(aspect_policy)&MSG_FLOW_ASPECT)
					{
						ret=router_store_route(message);
						if(ret<0)
						{
							print_cubeerr("router store route for aspect error!\n");
						}
						ret=router_set_aspect_flow(message,aspect_policy);
						if(ret>=0)
						{
							ret=router_set_next_jump(message);
							if(msg_head->state==MSG_FLOW_DELIVER)
							{
								route_push_aspect_site(message,origin_proc,conn_uuid);
							}
							if(ret>=0)
							{
								proc_audit_log(message);
								debug_message(message," prepare to send:");
								//print_cubeaudit("message (%s) is send to %s!\n",message_get_typestr(message),message_get_receiver(message));
								ret=proc_router_send_msg(message,local_uuid,proc_name);
								if(ret<0)
								{
									print_cubeerr("send aspect message failed!\n");
								}
								continue;

							}	
						}
					}
					else if(dispatch_policy_gettype(aspect_policy)&MSG_FLOW_DUP)
					{
						void * new_msg=message_clone(message);
						ret=router_set_dup_flow(new_msg,aspect_policy);
						if(ret>=0)
						{
							ret=router_set_next_jump(new_msg);
							if(ret>=0)
							{
								proc_audit_log(new_msg);
								debug_message(new_msg,"duplicate message prepare to send:");
								//print_cubeaudit("message (%s) is send to %s!\n",message_get_typestr(new_msg),message_get_receiver(new_msg));
								ret=proc_router_send_msg(new_msg,local_uuid,proc_name);
								if(ret<0)
								{
									print_cubeerr("send dup message failed!\n");
								}
							}		
						}
						
					}
				}	
				proc_audit_log(message);
				debug_message(message,"normal message prepare to send:");
				//print_cubeaudit("message (%s) is send to %s!\n",message_get_typestr(message),message_get_receiver(message));
				ret=proc_router_send_msg(message,local_uuid,proc_name);
				if(ret<0)
				{
					print_cubeerr("send message failed!\n");
					continue;
				}
			}

			continue;
		}
*/	
	}
	return 0;
}
