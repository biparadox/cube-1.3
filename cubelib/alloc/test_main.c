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
#include "../include/data_type.h"
#include "../include/memfunc.h"
#include "../include/alloc.h"

void test001() {
  void * p1, * p2, * p3;
  int freecount;
  int ret;
  p1 = Calloc0(250);
  p2 = Calloc0(98);
  p3 = Calloc0(77);

//  freecount=Ggetfreecount();
  
  printf("free count %d!\n",freecount);

  Free(p2);
  Free(p3);
  Free(p1);
}

void test002() {
  void * p1, * p2, * p3;
  int freecount;
  int ret;
  p1 = Talloc0(278);
  p2 = Talloc0(198);
  p3 = Talloc0(707);

//  freecount=Ggetfreecount();
  
  printf("free count %d!\n",freecount);

  TmemReset();
}

int main() {

//  static unsigned char alloc_buffer[4096*(1+1+4+1+32+1+256)];	

  alloc_init();
  test001();
  test002();
//  Tclear();
  return 0;
}
