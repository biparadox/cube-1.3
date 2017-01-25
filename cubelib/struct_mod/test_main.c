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
#include "../include/data_type.h"
#include "../include/alloc.h"
#include "../include/json.h"
#include "../include/struct_deal.h"

typedef struct uuid_head
{
	BYTE uuid[DIGEST_SIZE];
	char name[DIGEST_SIZE];
	int type;
	int subtype;
}__attribute__((packed)) UUID_HEAD;

struct connect_login
{
    char user[DIGEST_SIZE];
    char * passwd;
    char nonce[DIGEST_SIZE];
} __attribute__((packed));

static struct struct_elem_attr connect_login_desc[]=
{
    {"user",CUBE_TYPE_STRING,DIGEST_SIZE,NULL,NULL},
    {"passwd",CUBE_TYPE_ESTRING,sizeof(char *),NULL,NULL},
    {"nonce",CUBE_TYPE_STRING,DIGEST_SIZE,NULL,NULL},
    {NULL,CUBE_TYPE_ENDDATA,0,NULL}
};

struct struct_elem_attr uuid_head_desc[] =
{
	{"uuid",CUBE_TYPE_UUID,DIGEST_SIZE,NULL},
	{"name",CUBE_TYPE_STRING,DIGEST_SIZE,NULL},
	{"type",CUBE_TYPE_ENUM,sizeof(int),NULL},
	{"subtype",CUBE_TYPE_ENUM,sizeof(int),NULL},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL}
};

struct expand_flow_trace
{
    int  data_size;
    int  type;		      // this should be "MSG_EXPAND"
    int  subtype;	      // this should be "FLOW_TRACE"	
    int  record_num;
    char *  name;
    int  enum_num;
    NAME2VALUE * namelist;	
    BYTE *trace_record;
} __attribute__((packed));

struct struct_elem_attr expand_flow_trace_desc[] =
{
    	{"data_size",CUBE_TYPE_INT,0,NULL,NULL},
   	{"type",CUBE_TYPE_INT,0,NULL,NULL},
    	{"subtype",CUBE_TYPE_INT,0,NULL,NULL},
	{"record_num",CUBE_TYPE_INT,0,NULL,NULL},
	{"name",CUBE_TYPE_ESTRING,DIGEST_SIZE,NULL,NULL},
	{"enum_num",CUBE_TYPE_INT,0,NULL,NULL},
	{"namelist",CUBE_TYPE_DEFNAMELIST,0,NULL,"enum_num"},
	{"trace_record",CUBE_TYPE_DEFUUIDARRAY,0,NULL,"record_num"},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL}
};



int main() {

	struct expand_flow_trace test_trace={0,513,3,2,NULL,2,NULL,NULL};


	char buffer[4096];
	char buffer1[4096];
	char text[4096];
	char text1[4096];
	void * root;
	int ret;
	void * dup1;
	void * dup2;

	struct_deal_init();
	test_trace.name=dup_str("Hello",0);
	test_trace.trace_record=Talloc0(DIGEST_SIZE*test_trace.record_num);
	
	Strncpy(test_trace.trace_record,"ws_port",DIGEST_SIZE/2);
	Memset(test_trace.trace_record+DIGEST_SIZE,'A',DIGEST_SIZE);
	
	test_trace.namelist=Talloc0(sizeof(NAME2VALUE)*test_trace.enum_num);

	test_trace.namelist[0].value=1;
	test_trace.namelist[0].name=dup_str("INDEX",0);
	test_trace.namelist[1].value=2;
	test_trace.namelist[1].name=dup_str("KEY",0);

	
	void * struct_template= create_struct_template(&expand_flow_trace_desc);
	
	dup1=Talloc0(struct_size(struct_template));

	ret=struct_clone(&test_trace,dup1,struct_template);	

	ret=struct_2_json(&test_trace,text,struct_template);
	ret=struct_2_json(dup1,text1,struct_template);
	printf("%s \n",text);
	printf("%s \n",text1);

	ret=struct_2_blob(&test_trace,buffer,struct_template);
	printf ("blob size is %d \n",ret);
	dup2=Talloc0(struct_size(struct_template));
	ret=blob_2_struct(buffer,dup2,struct_template);
	ret=struct_2_json(dup2,text,struct_template);
	printf("%s \n",text);


    	free_struct_template(struct_template);
	return 0;
}
