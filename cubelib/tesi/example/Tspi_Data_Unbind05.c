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
 *	Tspi_Data_Unbind05.c
 *
 * DESCRIPTION
 *	This test will verify that Tspi_Data_Unbind can encrypt and decrypt
 *	some data using the PKCS1.5 Encryption Scheme with a bind key.
 *
 * ALGORITHM
 *	Setup:
 *		Create Context
 *		Connect Context
 *		Create Key Object
 *		Create Enc Data
 *		Set Attrib Uint32
 *		Load Key By UUID
 *		Bind Data
 *
 *	Test:
 *		Call Data_Unbind then if it does not succeed
 *		Make sure that it returns the proper return codes
 *		Print results
 *
 *	Cleanup:
 *		Free memory related to hContext
 *		Close context
 *
 * USAGE
 *      First parameter is --options
 *                         -v or --version
 *      Second parameter is the version of the test case to be run
 *      This test case is currently only implemented for v1.1
 *
 * HISTORY
 *      Kent Yoder, shpedoikal@gmail.com, 04/14/05
 *
 * RESTRICTIONS
 *	None.
 */

#include <stdio.h>
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

int
main( int argc, char **argv )
{
	char version;

	version = parseArgs( argc, argv );
	if (version)
		main_v1_1();
	else
		print_wrongVersion();
}

/* This is the max data size for a BIND key with PKCS1.5 padding*/
#define DATA_SIZE	((2048/8) - 11 - 4 - 1)

int
main_v1_1( void )
{
	char		*function = "Tspi_Data_Unbind05";
	TSS_HCONTEXT	hContext;
	TSS_HKEY	hSRK;
	TSS_HKEY	hKey;
	TSS_HPOLICY	hSrkPolicy;
	BYTE		*prgbDataToUnBind;
	TSS_HENCDATA	hEncData;
	TSS_HENCDATA	hDecData;
	UINT32		pulDataLength;
	BYTE		rgbDataToBind[DATA_SIZE], *rgbEncryptedData = NULL;
	UINT32		ulDataLength = DATA_SIZE, ulEncryptedDataLength = 0;
	TSS_UUID	uuid;
	TSS_RESULT	result;
	UINT32		exitCode;
	TSS_FLAG	initFlags = TSS_KEY_TYPE_BIND | TSS_KEY_SIZE_2048  |
				TSS_KEY_VOLATILE | TSS_KEY_NO_AUTHORIZATION |
				TSS_KEY_NOT_MIGRATABLE;

	print_begin_test( function );

	memset (rgbDataToBind, 0x5a, DATA_SIZE);

		// Create Context
	result = Tspi_Context_Create( &hContext );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_Create", result );
		exit( result );
	}

		// Connect to Context
	result = Tspi_Context_Connect( hContext, get_server(GLOBALSERVER) );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_Connect", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

		// create hKey
	result = Tspi_Context_CreateObject( hContext,
						TSS_OBJECT_TYPE_RSAKEY,
						initFlags, &hKey );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_CreateObject (hKey)", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	result = Tspi_Context_CreateObject( hContext,
						TSS_OBJECT_TYPE_ENCDATA,
						TSS_ENCDATA_BIND, &hEncData );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_CreateObject (hEncData)", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

		//Load Key By UUID
	result = Tspi_Context_LoadKeyByUUID( hContext, TSS_PS_TYPE_SYSTEM,
						SRK_UUID, &hSRK );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_LoadKeyByUUID (hSRK)", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

#ifndef TESTSUITE_NOAUTH_SRK
	char *pwdk="sss";
	char kpass[20];
	TSS_sha1((unsigned char *)pwdk,strlen(pwdk),kpass);
	result = Tspi_GetPolicyObject( hSRK, TSS_POLICY_USAGE, &hSrkPolicy );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_GetPolicyObject", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	result = Tspi_Policy_SetSecret( hSrkPolicy, TSS_SECRET_MODE_SHA1, 
					sizeof(kpass), kpass );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Policy_SetSecret", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}
#endif

	result = Tspi_SetAttribUint32( hKey, TSS_TSPATTRIB_KEY_INFO,
					TSS_TSPATTRIB_KEYINFO_ENCSCHEME,
					TSS_ES_RSAESPKCSV15 );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_SetAtttribUint32", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	result = Tspi_Key_CreateKey( hKey, hSRK, 0 );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Key_CreateKey", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

		// Load the newly created key
	result = Tspi_Key_LoadKey( hKey, hSRK );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Key_LoadKey", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	printf("Data before binding:\n");
	print_hex(rgbDataToBind, ulDataLength);

	result = Tspi_Data_Bind( hEncData, hKey, ulDataLength, rgbDataToBind );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Data_Bind", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	result = Tspi_GetAttribData(hEncData, TSS_TSPATTRIB_ENCDATA_BLOB,
					TSS_TSPATTRIB_ENCDATABLOB_BLOB,
					&ulEncryptedDataLength, &rgbEncryptedData);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_GetAttribData", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	result = Tspi_Context_CreateObject( hContext,
						TSS_OBJECT_TYPE_ENCDATA,
						TSS_ENCDATA_BIND, &hDecData );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_CreateObject (hDecData)", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	result = Tspi_SetAttribData(hDecData, TSS_TSPATTRIB_ENCDATA_BLOB,
					TSS_TSPATTRIB_ENCDATABLOB_BLOB,
					ulEncryptedDataLength, rgbEncryptedData);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_GetAttribData", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	printf("Data after encrypting:\n");
	print_hex(rgbEncryptedData, ulEncryptedDataLength);

	result = Tspi_Data_Unbind( hDecData, hKey, &pulDataLength,
					&prgbDataToUnBind );
	if ( result != TSS_SUCCESS )
	{
		if( !(checkNonAPI(result)) )
		{
			print_error( function, result );
			exitCode = 1;
		}
		else
		{
			print_error_nonapi( function, result );
			exitCode = 1;
		}
	}
	else
	{
		printf("Data after unbinding:\n");
		print_hex(prgbDataToUnBind, pulDataLength);

		if (pulDataLength != ulDataLength) {
			printf("ERROR: Size of decrypted data does not match! "
					"(%u != %u)\n", pulDataLength, ulDataLength);
		} else if (memcmp(prgbDataToUnBind, rgbDataToBind, ulDataLength)) {
			printf("ERROR: Content of decrypted data does not match!\n");
		} else {
			print_success( function, result );
			exitCode = 0;
		}
	}

	print_end_test( function );
	Tspi_Context_FreeMemory( hContext, NULL );
	Tspi_Context_Close( hContext );
	exit( exitCode );
}
