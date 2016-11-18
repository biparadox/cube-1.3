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

/*
struct verify_login
{
	char uuid[DIGEST_SIZE];
	struct connect_login login_info;
	char nonce_len[4];
   	char *nonce;
	int  listnum;
	char *uuidlist;
	void * namelist;
} __attribute__((packed));
*/
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

static struct struct_elem_attr login_db_desc[] =
{
    	{"head",CUBE_TYPE_SUBSTRUCT,0,&uuid_head_desc,NULL},
   	{"record_no",CUBE_TYPE_INT,sizeof(int),NULL,NULL},
    	{"login_list2",CUBE_TYPE_ARRAY,0,&connect_login_desc,"record_no"},
 //   	{"login_list1",CUBE_TYPE_SUBSTRUCT,3,&connect_login_desc,NULL},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL}
	
};

struct login_db
{
	UUID_HEAD head;
	int record_no;
	struct connect_login * login_list2;
//	struct connect_login login_list1[3];
}__attribute__((packed));


/*
static struct struct_elem_attr verify_login_desc[]=
{
    {"uuid",CUBE_TYPE_UUID,DIGEST_SIZE,NULL,NULL},
    {"login_info",CUBE_TYPE_SUBSTRUCT,0,&connect_login_desc,NULL},
    {"nonce_len",CUBE_TYPE_STRING,4,NULL},
    {"nonce",CUBE_TYPE_DEFINE,sizeof(char *),NULL,"nonce_len"},
    {"listnum",CUBE_TYPE_INT,DIGEST_SIZE,NULL,NULL},
    {"uuidlist",CUBE_TYPE_DEFUUIDARRAY,DIGEST_SIZE,NULL,"listnum"},
    {"namelist",CUBE_TYPE_DEFNAMELIST,DIGEST_SIZE,NULL,"listnum"},
    {NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};
*/

int main() {

	char * name_list[] = {
		"Hujun",
		"Taozheng",
		"Wangyubo",
		NULL
	};
	char * passwd_list[] = {
		"openstack",
		"passwd",
		"baixibao",
		NULL,
	};
//	struct verify_login test_login={"",{"HuJun","openstack"},"0x20","",4,"",		""};
	char buffer[4096];
	char buffer1[4096];
	char text[4096];
	char text1[4096];
	void * root;
//	char * json_string = "{\"login_info\":{\"user\":\"HuJun\","
//		"\"passwd\":\"openstack\"},\"nonce_len\":\"0x20\","
//		"\"nonce\":\"AAAAAAAABBBBBBBBCCCCCCCCDDDDEEFG\"}";
	int ret;
	struct login_db init_struct;
	struct login_db * recover_struct;
	struct login_db * recover_struct1;
	
	//char * namelist= "login_info.user,login_info.passwd";
//	char * namelist= "login_info";
	int flag;
	int i;
  	static unsigned char alloc_buffer[4096*(1+1+4+1+32+1+256)];	

//  	alloc_init(alloc_buffer);
	struct_deal_init();


	Memset(init_struct.head.uuid,'B',0x20);
	Strcpy(init_struct.head.name,"login_list");
	init_struct.head.type=1025;
	init_struct.head.subtype=6;

	for(i=0;name_list[i]!=NULL;i++);
	init_struct.record_no=i;

	ret=Galloc0(&init_struct.login_list2,sizeof(struct connect_login)*init_struct.record_no);
//	for(i=0;i<init_struct.record_no;i++)
//	{
//		Strncpy(init_struct.login_list1[i].user,name_list[i],DIGEST_SIZE);
//		init_struct.login_list1[i].passwd=passwd_list[i];
//		Memset(init_struct.login_list1[i].nonce, 'A'+(char)i,DIGEST_SIZE);
		
//	}
	for(i=0;i<init_struct.record_no;i++)
	{
		Strncpy(init_struct.login_list2[i].user,name_list[i],DIGEST_SIZE);
		init_struct.login_list2[i].passwd=passwd_list[i];
		Memset(init_struct.login_list2[i].nonce, 'A'+(char)i,DIGEST_SIZE);
		
	}
	
/*
	nonce=Calloc(0x20);
	memset(test_login.nonce,'A',0x20);
	test_login.uuidlist=Calloc(DIGEST_SIZE*4);
	memset(test_login.uuidlist,'C',DIGEST_SIZE*4);
	test_login.namelist=Calloc(sizeof(NAME2VALUE)*test_login.listnum);
	NAME2VALUE * namelist=test_login.namelist;

	for(i=0;i<test_login.listnum;i++)
	{
		namelist[i].name=Calloc(10);
		sprintf(namelist[i].name,"name_%d",i);
	}
	namelist[0].value=1;
	namelist[1].value=2;
	namelist[2].value=4;
	namelist[3].value=5;
*/

//	recover_struct=Calloc(sizeof(struct verify_login));
    	void * struct_template=create_struct_template(login_db_desc);

	int size=struct_size(struct_template);
	printf("this struct's size is %d\n",size);

	ret=Galloc0(&recover_struct,size);
	if(ret<0)
		return ret;
	ret=Galloc0(&recover_struct1,size);
	if(ret<0)
		return ret;

	ret=struct_2_blob(&init_struct,buffer,struct_template);	
	printf("get %d size blob!\n",ret);

/*
	ret=struct_set_flag(struct_template,CUBE_ELEM_FLAG_TEMP,namelist);
	ret=struct_2_part_blob(&test_login,buffer1,struct_template,CUBE_ELEM_FLAG_TEMP);
	printf("get %d size blob!\n",ret);
*/
	ret=blob_2_struct(buffer,recover_struct,struct_template);
	printf("read %d size blob!\n",ret);
/*
	ret=blob_2_part_struct(buffer,recover_struct1,struct_template,CUBE_ELEM_FLAG_TEMP);
	printf("read %d size blob!\n",ret);
*/
	ret=struct_2_json(recover_struct,text,struct_template);
	printf("read %d size to json %s!\n",ret,text);
/*
	ret=struct_2_part_json(&test_login,text1,struct_template,CUBE_ELEM_FLAG_TEMP);
	printf("read %d size to json %s!\n",ret,text1);
*/
	ret=json_solve_str(&root,text);
	ret=json_2_struct(root,recover_struct1,struct_template);
	printf("read %d size to json!",ret);

	void * recover_struct2=clone_struct(recover_struct1,struct_template);
	if(recover_struct2==NULL)
	{
		printf("clone struct failed!\n");
		return -1;
	}

	ret=struct_compare(recover_struct1,recover_struct2,struct_template);
	if(ret!=0)
	{
		printf("compare struct different!\n");
		return -1;
	}
	else
		printf("compare struct is the same!\n");

	ret=struct_2_json(recover_struct2,text1,struct_template);
	printf("recover struct %d size to json %s!\n",ret,text1);

//	ret=struct_read_elem_text("login_info.passwd",&test_login,text,struct_template);
//	printf("read passwd %s from struct!\n",text);
/*	
	ret=struct_set_flag(struct_template,CUBE_ELEM_FLAG_TEMP,namelist);
	flag=struct_get_flag(struct_template,"login_info.passwd");
	
	memset(buffer,0,500);
	ret=struct_2_part_blob(&test_login,buffer,struct_template,CUBE_ELEM_FLAG_TEMP);
	ret=struct_2_part_json(&test_login,text,struct_template,CUBE_ELEM_FLAG_TEMP);
	printf("%s\n",text);

	ret=blob_2_struct(buffer,&test_login,struct_template);	
	ret=struct_2_json(&test_login,text,struct_template);
	printf("%s\n",text);
	ret=json_solve_str(&root,text);
	ret=json_2_struct(root,recover_struct,struct_template);
	ret=struct_2_json(recover_struct,text1,struct_template);
	printf("%s\n",text1);
//	ret=text_2_struct(text,&test_login,struct_template);
//	ret=blob_2_text(buffer,text,struct_template);	
//	ret=text_2_blob(buffer,text,struct_template);	
	ret=blob_2_struct(buffer,recover_struct,struct_template);
*/
	
	struct_free(recover_struct,struct_template);
    	free_struct_template(struct_template);
	return 0;
}
