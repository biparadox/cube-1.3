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
#include "../include/data_type.h"
#include "../include/string.h"

int main() {


//	struct connect_login test_login={"HuJun","openstack"};
	char * src=" hello,world!";
	char * src1=" hello,World!";
	int i;
	int ret;
	char buffer[20];
	char buffer1[20];
	Memset(buffer,0,20);
	Memcpy(buffer,src,9);
	printf("\n%s\n",buffer);
	ret = Memcmp(src,src1,9);
	printf ("Memcmp result is %d!\n",ret);

	Strcpy(buffer1,src);
	printf("\n%s\n",buffer1);

	Strncpy(buffer1,src,20);
	printf("\n%s\n",buffer1);

	Memset(buffer1,'A',15);
	Strncpy(buffer1,src,9);
	printf("\n%s\n",buffer1);

	ret=Strcmp(src,src1);
	printf ("Strcmp result is %d!\n",ret);

	ret=Strncmp(src1,src,9);
	printf ("Strncmp result is %d!\n",ret);

	Memset(buffer,0,20);
	Memset(buffer1,0,20);

	Strncpy(buffer,src,9);
	Strncpy(buffer1,src1+3,9);

	Strncat(buffer,buffer1,20);
	printf("\n%s\n",buffer);

	ret=Strlen(buffer);
	printf ("Strlen result is %d!\n",ret);

	ret=Strnlen(buffer,10);
	printf ("Strnlen result is %d!\n",ret);



	i=Getfiledfromstr(buffer,src,',',0);
	printf("\n%s\n %c",buffer,src[i]);
	i=Getfiledfromstr(buffer,src+i+1,',',0);
	printf("\n%s\n",buffer);
	i=Itoa(-216342,buffer);
	printf("\n%s\n",buffer);
	
	i=1<<12;
	printf( "%d bit!\n",Getlowestbit(&i,4,1));
	Memset(buffer,0,20);
	buffer[8]=0x10;
	printf( "%d bit!\n",Getlowestbit(buffer,20,1));
	return 0;
}
