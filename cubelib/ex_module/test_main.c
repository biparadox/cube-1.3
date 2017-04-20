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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

//#include "../include/kernel/errno.h"
#include "../include/data_type.h"
#include "../include/alloc.h"
#include "../include/basefunc.h"
#include "../include/struct_deal.h"
/*
#include "../include/memdb.h"
#include "../include/message.h"
#include "../include/routine.h"
*/

#include "../include/ex_module.h"


int test_module_init(void * ex_module,void * para)
{
	printf("test ex_module init !\n");
	return 0;
}

int test_module_start(void * ex_module,void * para)
{
	printf("test ex_module start !\n");
	return 1;
}

int main() {

	void * ex_module;
	int ret;
	struct_deal_init();
	memdb_init();
	ex_module_list_init();
	ex_module_create("test",0,NULL,&ex_module);

	ex_module_setinitfunc(ex_module,&test_module_init);
	ex_module_setstartfunc(ex_module,&test_module_start);
	

	ex_module_init(ex_module,NULL);

	void * slot_list;
	void * slot_port;
	void * slot_pin;
	void * slot_sock;

	void * slot_port1;

	BYTE uuid[32];
	void * record;
	void * record1;

	slot_port=slot_port_init("test_port",0);
	slot_port_addpin(slot_port,1,7,0);
	slot_port_addpin(slot_port,2,8,0);
	ex_module_addslot(ex_module,slot_port);	
	
	slot_port1=ex_module_findport(ex_module,"test_port");

	Memset(uuid,'a',DIGEST_SIZE);

	slot_sock=slot_create_sock(slot_port1,uuid);

	record=NULL;

	ret=slot_sock_addrecorddata(slot_sock,7,0,record);
/*
	record1=slot_sock_removerecord(slot_sock,8,0);
*/	
	ex_module_start(ex_module,NULL);
	ex_module_join(ex_module,&ret);
	
	return ret;

}
