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
#include "../include/tesi.h"
#include "../include/struct_deal.h"

#define DIGEST_SIZE 32

int print_error(char * str, int result)
{
	printf("%s %s",str,tss_err_string(result));
}

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

struct ca_cert{
	struct aik_request_info reqinfo;
	char AIPubKey_uuid[DIGEST_SIZE*2];
}__attribute__((packed));

static struct struct_elem_attr ca_cert_desc[]=
{
	{"reqinfo",OS210_TYPE_ORGCHAIN,0,req_info_desc},
	{"AIPubKey_uuid",OS210_TYPE_STRING,DIGEST_SIZE*2,NULL},
	{NULL,OS210_TYPE_ENDDATA,0,NULL}
};

int main(void){

	char		*nameOfFunction = "TESI_AIK_Activate";
	TSS_RESULT	result;
	TSS_HKEY	hAIKey, hCAKey;
	
	TESI_SIGN_DATA signdata;

	int fd;
	int retval;
	int blobsize=0;


	result=TESI_Local_ReloadWithAuth("ooo","sss");

	if ( result != TSS_SUCCESS )
	{
		print_error("TESI_Local_Load Err!\n",result);
		return result;
	}


	result=TESI_Local_ReadKeyBlob(&hAIKey,"identkey");
	if ( result != TSS_SUCCESS )
	{
		print_error("TESI_Local_ReadKeyBlob Err!\n",result);
		return result;
	}

	result=TESI_Local_LoadKey(hAIKey,NULL,"kkk");
	if ( result != TSS_SUCCESS )
	{
		print_error("TESI_Local_LoadKey Err!\n",result);
		return result;
	}

	result=TESI_AIK_Activate(hAIKey,"active",&signdata);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		exit(result);
	}
	// Load the CA Key
	result=TESI_Local_GetPubKeyFromCA(&hCAKey,"CA");
	if (result != TSS_SUCCESS) {
		print_error("Get pubkey error!\n", result);
		exit(result);
	}
	
	// write the AIK and aipubkey

	result=TESI_Local_WriteKeyBlob(hAIKey,"AIK");
	if (result != TSS_SUCCESS) {
		print_error("store aik data error!\n", result);
		exit(result);
	}
	result=TESI_Local_WritePubKey(hAIKey,"AIK");
	if (result != TSS_SUCCESS) {
		print_error("store aik data error!\n", result);
		exit(result);
	}

	// verify the CA signed cert
	result=TESI_AIK_VerifySignData(&signdata,"CA");
	if (result != TSS_SUCCESS) {
		print_error("verify data error!\n", result);
		exit(result);
	}

	// get the content of the CA signed cert

	struct ca_cert usercert;

	// read the req info from aik request package
	void * struct_template=create_struct_template(ca_cert_desc);
	if(struct_template==NULL)
		return -EINVAL;
	blobsize=blob_2_struct(signdata.data,&usercert,struct_template);
	if(blobsize!=signdata.datalen)
		return -EINVAL;

	free_struct_template(struct_template);


	return 0;

}
