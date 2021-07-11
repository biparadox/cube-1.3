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
#include "data_type.h"
#include "list.h"
#include "alloc.h"
#include "memfunc.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"


int main() {

  UUID_HEAD * test_uuid;
  UUID_HEAD * test_uuid1;

  BYTE digest[DIGEST_SIZE];
  int ret;
//  static unsigned char alloc_buffer[4096*(1+1+4+1+16+1+256)];	

  void * hash_head;

//  alloc_init(alloc_buffer);

  test_uuid=Calloc(sizeof(UUID_HEAD));
  Memset(test_uuid->uuid,'A',DIGEST_SIZE);  

  hash_head=init_hash_list(8,0,0);
	
  ret=hashlist_add_elem(hash_head,test_uuid);	
  
  Memset(digest,'A',DIGEST_SIZE);
  test_uuid1=hashlist_find_elem(hash_head,digest);

  test_uuid=hashlist_get_first(hash_head);
  test_uuid1=hashlist_get_next(hash_head);

  test_uuid=hashlist_remove_elem(hash_head,digest);	
  test_uuid1=hashlist_find_elem(hash_head,digest);

//  void * list_queue;
//  list_queue=init_list_queue();
 
//  list_queue_put(list_queue,test_uuid);	  	
//  list_queue_put(list_queue,test_uuid1);
 
//  void * temp;
	
//  ret=list_queue_get(list_queue,&temp);
//  ret=list_queue_get(list_queue,&temp);
//  ret=list_queue_get(list_queue,&temp);

//  free_list_queue(list_queue);

    Free(test_uuid);
	

    return 0;
}
