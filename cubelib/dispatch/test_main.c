/**
 * Copyright [2015] Tianfu Ma (matianfu@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * File: main.c
 *
 * Created on: Jun 5, 2015
 * Author: Tianfu Ma (matianfu@gmail.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#include "../include/errno.h"
#include "../include/data_type.h"
#include "../include/alloc.h"
#include "../include/basefunc.h"
#include "../include/struct_deal.h"
#include "../include/memdb.h"
#include "../include/message.h"
#include "../include/dispatch.h"


int read_json_file(char * file_name)
{
	int ret;

	int fd;
	int readlen;
	int json_offset;

	int struct_no=0;
	void * root_node;
	void * findlist;
	void * memdb_template ;
	BYTE uuid[DIGEST_SIZE];
	char json_buffer[4096];

	fd=open(file_name,O_RDONLY);
	if(fd<0)
		return fd;

	readlen=read(fd,json_buffer,4096);
	if(readlen<0)
		return -EIO;
	json_buffer[readlen]=0;
	close(fd);

	json_offset=0;
	while(json_offset<readlen)
	{
		ret=json_solve_str(&root_node,json_buffer+json_offset);
		if(ret<0)
		{
			printf("solve json str error!\n");
			break;
		}
		json_offset+=ret;
		if(ret<32)
			continue;

		ret=memdb_read_desc(root_node,uuid);
		if(ret<0)
			break;
		struct_no++;
	}

	return struct_no;
}

int read_dispatch_file(char * file_name)
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
	char json_buffer[4096];
	void * policy;

	fd=open(file_name,O_RDONLY);
	if(fd<0)
		return fd;

	readlen=read(fd,json_buffer,1024);
	if(readlen<0)
		return -EIO;
	json_buffer[readlen]=0;
	printf("%s\n",json_buffer);
	close(fd);

	json_offset=0;
	while(json_offset<readlen)
	{
		ret=json_solve_str(&root_node,json_buffer+json_offset);
		if(ret<0)
		{
			printf("solve json str error!\n");
			break;
		}
		json_offset+=ret;
		if(ret<32)
			continue;

		policy=dispatch_read_policy(root_node);
		if(policy==NULL)
		{
			printf("read %d file error!\n",count);
			break;
		}
		dispatch_policy_add(policy);
		count++;
	}
	printf("read %d policy succeed!\n",count);
	return count;
}

int main() {

  	static unsigned char alloc_buffer[4096*(1+1+4+1+32+1+256)];	
	char json_buffer[4096];
	char print_buffer[4096];
	int ret;
	int readlen;
	int json_offset;
	void * root_node;
	void * findlist;
	void * memdb_template ;
	BYTE uuid[DIGEST_SIZE];
	int i;
	MSG_HEAD * msg_head;
	pthread_t  cube_thread;
	
	char * baseconfig[] =
	{
		"namelist.json",
		"dispatchnamelist.json",
		"typelist.json",
		"subtypelist.json",
		"msghead.json",
		"msgrecord.json",
		"expandrecord.json",
		"base_msg.json",
		"dispatchrecord.json",
		NULL
	};

//	alloc_init(alloc_buffer);
	struct_deal_init();
	memdb_init();

// test namelist reading start

	for(i=0;baseconfig[i]!=NULL;i++)
	{
		ret=read_json_file(baseconfig[i]);
		if(ret<0)
			return ret;
		printf("read %d elem from file %s!\n",ret,baseconfig[i]);
	}

	void * record;

// test struct desc reading start

	msgfunc_init();
	dispatch_init();
	
	void * message;
	void * policy;
	
	ret=read_dispatch_file("dispatch_policy.json");

	message=message_create(DTYPE_MESSAGE,SUBTYPE_BASE_MSG,NULL);	

	struct basic_message * base_msg;
	base_msg=Talloc0(sizeof(*base_msg));
	if(base_msg==NULL)
		return -ENOMEM;
	base_msg->message=dup_str("hello",0);

	message_add_record(message,base_msg);	
	ret=message_output_json(message,json_buffer);
	if(ret<0)
	{
		printf("message output json failed!\n");
		return ret;
	}

	printf("%s\n",json_buffer);

	ret=dispatch_policy_getfirst(&policy);
	while(policy!=NULL)
	{
		ret=dispatch_match_message(policy,message);
		if(ret==0)
		{
			printf("this message match the policy!\n");
			break;
		}
		ret=dispatch_policy_getnext(&policy);
	}

	if(policy!=NULL)
	{
		router_set_local_route(message,policy);	
		ret=message_output_json(message,json_buffer);
		if(ret<0)
		{
			printf("message output json failed!\n");
			return ret;
		}

		printf("%s\n",json_buffer);
	}

//	routine_start();
//	sleep(100000);
	
	return 0;

}
