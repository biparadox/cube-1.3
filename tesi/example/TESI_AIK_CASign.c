/*
 *
 *   Copyright (C) International Business Machines  Corp., 2005
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

#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include "../include/tesi.h"
#include "../include/struct_deal.h"

#define DIGEST_SIZE 32
const char *fn = "TESI_AIK_CASign";

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


int  main(void){

	TSS_HKEY		hIdentKey, hCAKey;
	TSS_RESULT		result;
	RSA			*rsa = NULL;
	TCPA_IDENTITY_PROOF	identityProof;


	char buffer[1024];
	char digest[DIGEST_SIZE];
	int blobsize=0;
	
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	result=TESI_Local_Reload();

	if ( result != TSS_SUCCESS )
	{
		printf("TESI_Local_Load Err!\n");
		return result;
	}

		
	/* decrypt the TCPA_IDENTITY_REQ blobs */

	ReadPrivKey(&rsa,"CA","my ca center");

	if(rsa == NULL)
	{
		printf("read rsa private key failed!\n");
		return 0;
	}
		//Create CA Key Object
	result=TESI_Local_GetPubKeyFromCA(&hCAKey,"CA");

	TESI_AIK_VerifyReq(rsa,hCAKey,"aik",&hIdentKey,&identityProof);

	/* verify the EK's credentials in the identity proof structure */
	/* XXX Doesn't yet exist. In 1.2 they should be in NVDATA somewhere */

	struct ca_cert usercert;

	// read the req info from aik request package
	void * struct_template=create_struct_template(req_info_desc);
	if(struct_template==NULL)
		return -EINVAL;
	blobsize=blob_2_struct(identityProof.labelArea,&usercert.reqinfo,struct_template);
	if(blobsize!=identityProof.labelSize)
		return -EINVAL;

	free_struct_template(struct_template);

	// get the uuid of identity key and write it to user cert
	TESI_Local_WritePubKey(hIdentKey,"identkey");

	calculate_sm3("identkey.pem",digest);
	digest_to_uuid(digest,usercert.AIPubKey_uuid);

	struct_template=create_struct_template(ca_cert_desc);
	if(struct_template==NULL)
		return -EINVAL;

	// get the usercert's blob 
	blobsize=struct_2_blob(&usercert,buffer,struct_template);
	free_struct_template(struct_template);
	
	if (result = TESI_AIK_CreateAIKCert(hIdentKey,rsa,buffer,blobsize,"pubek","active")) {
		printf("ca_create_credential %s", tss_err_string(result));
		RSA_free(rsa);
		exit(result);
	}
	

/*
	if (result = ca_create_credential(hIdentKey,"pubek","active")) {
		print_error("ca_create_credential", result);
		RSA_free(rsa);
		exit(result);
	}
*/	

	RSA_free(rsa);

	return result;
}
