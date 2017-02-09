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

/*
 * NAME
 *	Tspi_Key_CertifyKey01
 *
 * DESCRIPTION
 *	This test will verify Tspi_Key_CertifyKey
 *	The purpose of this test case is to get TSS_SUCCESS to be
 *		returned. This is easily accomplished by following 
 *		the algorithm described below.
 *
 *
 * ALGORITHM
 *	Setup:
 *		Create Context
 *		Connect
 *		Create hKey
 *		Load Key By UUID for hSRK
 *		Get Policy Object for the srk
 *		Set Secret for srk
 *		Create Ident Key Object
 *		Get Policy Object
 *		Set Secret
 *		Create Key
 *		Load Key
 *		Get Policy Object
 *		Set Secret
 *		Create Key
 *		Load Key
 *		Get Default Policy for the hPolicy
 *
 *	Test:	Call CertifyKey. If it is not a success
 *		Make sure that it returns the proper return codes
 *
 *	Cleanup:
 *		Free memory associated with the context
 *		Close the hCertifyingKey object
 *		Close the hKey object
 *		Close context
 *		Print error/success message
 *
 * USAGE:	First parameter is --options
 *			-v or --version
 *		Second Parameter is the version of the test case to be run.
 *		This test case is currently only implemented for 1.1
 *
 * HISTORY
 *	Author:	Kathy Robertson
 *	Date:	June 2004
 *	Email:	klrobert@us.ibm.com
 *	Kent Yoder <kyoder@users.sf.net>
 *	  - fixes 6/30/05
 *
 * RESTRICTIONS
 *	None.
 */

#include "../tesi/common.h"
#include "../include/tesi.h"
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
	char		version;

	version = parseArgs( argc, argv );
	if (version)
		main_v1_1();
	else
		print_wrongVersion();
}

main_v1_1(void){

	char		*nameOfFunction = "Tspi_Key_CertifyKey01";
	TSS_HKEY	hKey;
	TSS_HKEY	hSRK;
	TSS_HPOLICY	hPolicy;
	TSS_HKEY	hCertifyingKey;
	TSS_HCONTEXT	hContext;
	TSS_RESULT	result;
	TSS_FLAG	initFlags;
	TSS_HPOLICY	srkUsagePolicy, keyUsagePolicy;
	TSS_VALIDATION	pValidationData;
	BYTE		*data;
	TSS_HTPM	hTPM;

	print_begin_test(nameOfFunction);

		//Create Context
	result = Tspi_Context_Create(&hContext);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_Create ", result);
		print_error_exit(nameOfFunction, err_string(result));
		exit(result);
	}
		//Connect Context
	result = Tspi_Context_Connect(hContext, get_server(GLOBALSERVER));
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_Connect", result);
		print_error_exit(nameOfFunction, err_string(result));
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Get TPM Object
	result = Tspi_Context_GetTpmObject(hContext, &hTPM);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_GetTpmObject", result);
		print_error_exit(nameOfFunction, err_string(result));
		Tspi_Context_Close(hContext);
		exit(result);
	}

		//Load Key By UUID
	result = Tspi_Context_LoadKeyByUUID(hContext,
			TSS_PS_TYPE_SYSTEM,
			SRK_UUID, &hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_LoadKeyByUUID", result);
		print_error_exit(nameOfFunction, err_string(result));
		Tspi_Context_Close(hContext);
		Tspi_Context_CloseObject(hContext, hKey);
		exit(result);
	}
#ifndef TESTSUITE_NOAUTH_SRK
	char *pwdk="sss";
	char kpass[20];
	TSS_sha1((unsigned char *)pwdk,strlen(pwdk),kpass);

		//Get Policy Object
	result = Tspi_GetPolicyObject(hSRK, TSS_POLICY_USAGE, &srkUsagePolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_GetPolicyObject", result);
		print_error_exit(nameOfFunction, err_string(result));
		Tspi_Context_Close(hContext);
		Tspi_Context_CloseObject(hContext, hKey);
		exit(result);
	}
		//Set Secret
	result = Tspi_Policy_SetSecret(srkUsagePolicy, TSS_SECRET_MODE_SHA1,
				sizeof(kpass),kpass);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		print_error_exit(nameOfFunction, err_string(result));
		Tspi_Context_Close(hContext);
		Tspi_Context_CloseObject(hContext, hKey);
		exit(result);
	}
#endif
		//Create a storage key to certify
	result = Tspi_Context_CreateObject(hContext,
			TSS_OBJECT_TYPE_RSAKEY,
			TSS_KEY_TYPE_LEGACY | TSS_KEY_SIZE_512, &hKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		print_error_exit(nameOfFunction, err_string(result));
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Create Key
	result = Tspi_Key_CreateKey(hKey, hSRK, 0);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_CreateKey", result);
		print_error_exit(nameOfFunction, err_string(result));
		Tspi_Context_Close(hContext);
		Tspi_Context_CloseObject(hContext, hCertifyingKey);
		Tspi_Context_CloseObject(hContext, hKey);
		exit(result);
	}
	result = Tspi_Key_LoadKey(hKey, hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_LoadKey", result);
		print_error_exit(nameOfFunction, err_string(result));
		Tspi_Context_Close(hContext);
		Tspi_Context_CloseObject(hContext, hCertifyingKey);
		Tspi_Context_CloseObject(hContext, hKey);
		exit(result);
	}

		//Create signing Key object
	result = Tspi_Context_CreateObject(hContext, 
			TSS_OBJECT_TYPE_RSAKEY,
			TSS_KEY_SIZE_2048 |TSS_KEY_TYPE_SIGNING, &hCertifyingKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		print_error_exit(nameOfFunction, err_string(result));
		Tspi_Context_Close(hContext);
		exit(result);
	}

		//Create Key for Ident Key
	result = Tspi_Key_CreateKey(hCertifyingKey, hSRK, 0);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_CreateKey", result);
		print_error_exit(nameOfFunction, err_string(result));
		Tspi_Context_Close(hContext);
		exit(result);
	}
	result = Tspi_Key_LoadKey(hCertifyingKey, hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_LoadKey", result);
		print_error_exit(nameOfFunction, err_string(result));
		Tspi_Context_Close(hContext);
		exit(result);
	}

	result = Tspi_TPM_GetRandom(hTPM, 20, &data);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_TPM_GetRandom ", result);
		print_error_exit(nameOfFunction, err_string(result));
		Tspi_Context_Close(hContext);
		exit(result);
	}

	pValidationData.ulExternalDataLength = 20;
	pValidationData.rgbExternalData = data;

		//Call Key Certify Key
	result = Tspi_Key_CertifyKey(hKey, hCertifyingKey, &pValidationData);
	if (result != TSS_SUCCESS){
		if(!checkNonAPI(result)){
			print_error(nameOfFunction, result);
			print_end_test(nameOfFunction);
			Tspi_Context_Close(hContext);
			exit(result);
		}
		else{
			print_error_nonapi(nameOfFunction, result);
			print_end_test(nameOfFunction);
			Tspi_Context_Close(hContext);
			exit(result);
		}
	}
	else{
		print_success(nameOfFunction, result);
		print_end_test(nameOfFunction);
		Tspi_Context_Close(hContext);
		exit(0);
	}
}
