/*
 *
 *   Copyright (C) International Business Machines  Corp., 2006
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

/*
 * NAME
 *	Tspi_Key_CreateKey05.c
 *
 * DESCRIPTION
 *	This test will verify that Tspi_Key_CreateKey can be used with a
 *	no-auth parent.
 *
 * ALGORITHM
 *	Setup:
 *		Create Context
 *		Connect Context
 *		Create Object
 *		Load SRK By UUID
 *		Get Policy Object
 *		Set Secret
 *
 *	Test:	Call CreateKey. If it is not a success
 *		Make sure that it returns the proper return codes
 *
 *	Cleanup:
 *		Close hKey object
 *		Close context
 *		Print error/success message
 *
 * USAGE:	First parameter is --options
 *			-v or --version
 *		Second Parameter is the version of the test case to be run.
 *		This test case is currently only implemented for 1.1
 *
 * HISTORY
 *	Kent Yoder, kyoder@users.sf.net, 08/06
 *
 * RESTRICTIONS
 *	None.
 */

#include <stdio.h>
#include "../../include/data_type.h"
#include "../common.h"
#include "../../include/tesi.h"
#include "sha1.h"

void TSS_sha1(void *input, unsigned int len, unsigned char *output)
{
	SHA1_CTX sha;
	SHA1_Init(&sha);
	SHA1_Update(&sha,input,len);
	SHA1_Final(output,&sha);
}

int main(int argc, char **argv)
{
	char version;

	version = parseArgs( argc, argv );
	if (version)
		main_v1_1();
	else
		print_wrongVersion();
}

main_v1_1(void){

	char		*nameOfFunction = "Tspi_Key_CreateKey05";
	TSS_HCONTEXT	hContext;
	TSS_HTPM	hTPM;
	TSS_FLAG	initFlags;
	TSS_HKEY	hKey, hParentKey;
	TSS_HKEY	hSRK;
	TSS_RESULT	result;
	TSS_UUID	uuid;
	BYTE		*randomData;
	TSS_HPOLICY	srkUsagePolicy, keyUsagePolicy, keyMigPolicy;

	print_begin_test(nameOfFunction);

		//Create Context
	result = Tspi_Context_Create(&hContext);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_Create", result);
		exit(result);
	}
		//Connect Context
	result = Tspi_Context_Connect(hContext, get_server(GLOBALSERVER));
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_Connect", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

		//Load Key By UUID
	result = Tspi_Context_LoadKeyByUUID(hContext,
				TSS_PS_TYPE_SYSTEM, SRK_UUID, &hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_LoadKeyByUUID", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
#ifndef TESTSUITE_NOAUTH_SRK
		//Get Policy Object
	char *pwdk="sss";
	char kpass[20];
	TSS_sha1((unsigned char *)pwdk,strlen(pwdk),kpass);
	result = Tspi_GetPolicyObject(hSRK, TSS_POLICY_USAGE, &srkUsagePolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_GetPolicyObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Set Secret
	result = Tspi_Policy_SetSecret( srkUsagePolicy, TSS_SECRET_MODE_SHA1, 
					sizeof(kpass), kpass );
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
#endif
		//Create Parent Object
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
					   TSS_KEY_TYPE_STORAGE | TSS_KEY_SIZE_2048 |
					   TSS_KEY_VOLATILE | TSS_KEY_NO_AUTHORIZATION |
					   TSS_KEY_NOT_MIGRATABLE, &hParentKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

		//Create Object
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
					   TSS_KEY_TYPE_SIGNING | TSS_KEY_SIZE_512 |
					   TSS_KEY_VOLATILE | TSS_KEY_NO_AUTHORIZATION |
					   TSS_KEY_NOT_MIGRATABLE, &hKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

		//Create Parent Key
	result = Tspi_Key_CreateKey(hParentKey, hSRK, 0);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_CreateKey", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	result = Tspi_Key_LoadKey(hParentKey, hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_LoadKey", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

		//Create Key
	result = Tspi_Key_CreateKey(hKey, hParentKey, 0);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_CreateKey", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	result = Tspi_Key_LoadKey(hKey, hParentKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_LoadKey", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	result = sign_and_verify(hContext, hKey);
	if (result != TSS_SUCCESS){
		if(!checkNonAPI(result)){
			print_error(nameOfFunction, result);
			print_end_test(nameOfFunction);
			Tspi_Context_CloseObject(hContext, hKey);
			Tspi_Context_Close(hContext);
			exit(result);
		}
		else{
			print_error_nonapi(nameOfFunction, result);
			print_end_test(nameOfFunction);
			Tspi_Context_CloseObject(hContext, hKey);
			Tspi_Context_Close(hContext);
			exit(result);
		}
	}
	else{
		print_success(nameOfFunction, result);
		print_end_test(nameOfFunction);
		Tspi_Context_CloseObject(hContext, hKey);
		Tspi_Context_Close(hContext);
		exit(0);
	}
}
