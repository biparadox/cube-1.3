/*
 *
 *   Copyright (C) International Business Machines  Corp., 2004, 2005
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include "../include/data_type.h"
#include "../include/struct_deal.h"
#include "../include/tesi.h"
#include "../include/tesi_aik_struct.h"

#define DIGEST_SIZE	32

int print_error(char * str, int result)
{
	printf("%s %s",str,tss_err_string(result));
}
/*
struct aik_request_info{
	char user_uuid[DIGEST_SIZE*2];
	char * user_name;
	char signpubkey_uuid[DIGEST_SIZE*2];
}__attribute__((packed));

static struct struct_elem_attr req_info_desc[]=
{
	{"user_uuid",OS210_TYPE_STRING,DIGEST_SIZE*2,NULL},
	{"user_name",OS210_TYPE_ESTRING,DIGEST_SIZE*2,NULL},
	{"signpubkey_uuid",OS210_TYPE_STRING,DIGEST_SIZE*2,NULL},
	{NULL,OS210_TYPE_ENDDATA,0,NULL}
};
*/
int get_local_uuid(char * uuid)
{
	memset(uuid,'A',DIGEST_SIZE*2);
}

int main(int argc, char ** argv){

	char		*nameOfFunction = "TESI_AIK_GenerateReq";
	TSS_RESULT	result;
	TSS_HKEY 	hSignKey;
	TSS_HKEY	hAIKey, hCAKey;

	BYTE		*labelString = "UserA";
	UINT32		labelLen = strlen(labelString) + 1;

	struct aik_cert_info reqinfo;
	char buffer[1024];
	char digest[DIGEST_SIZE];
	int blobsize=0;

	int fd;

	result=TESI_Local_ReloadWithAuth("ooo","sss");

	if ( result != TSS_SUCCESS )
	{
		print_error("TESI_Local_Load Err!\n",result);
		return result;
	}

	// create a signkey and write its key in localsignkey.key, write its pubkey in localsignkey.pem
	result=TESI_Local_CreateSignKey(&hSignKey,(TSS_HKEY)NULL,"sss","kkk");
	if(result == TSS_SUCCESS)
		printf("Create SignKey SUCCEED!\n");

	TESI_Local_WriteKeyBlob(hSignKey,"localsignkey");
	TESI_Local_WritePubKey(hSignKey,"localsignkey");
	
	// fill the reqinfo struct
	calculate_sm3("localsignkey.pem",digest);
	digest_to_uuid(digest,reqinfo.pubkey_uuid);
	reqinfo.user_info.user_name=labelString;
	get_local_uuid(reqinfo.machine_uuid);
	
	// create info blob
	void * struct_template=create_struct_template(aik_cert_info_desc);
	if(struct_template==NULL)
		return -EINVAL;
	blobsize=struct_2_blob(&reqinfo,buffer,struct_template);


	// Load the CA Key
	result=TESI_Local_GetPubKeyFromCA(&hCAKey,"CA");
	if (result != TSS_SUCCESS) {
		print_error("Get pubkey error!\n", result);
		exit(result);
	}
	
	TESI_AIK_CreateIdentKey(&hAIKey,NULL,"sss","kkk"); 
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		exit(result);
	}

	labelLen=strlen(labelString);

	result = TESI_AIK_GenerateReq(hCAKey,blobsize,buffer,hAIKey,"aik");
	if (result != TSS_SUCCESS){
		print_error(nameOfFunction, result);
		exit(result);
	}
	TESI_Local_WriteKeyBlob(hAIKey,"identkey");
}
