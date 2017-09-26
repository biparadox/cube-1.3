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
#include "data_type.h"
#include "../common.h"
#include "tesi.h"
//#include "../sha1.h"

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
	char		*function = "TESI_bind_genkey";
	TSS_HKEY	hKey;
	TSS_RESULT	result;
	UINT32		exitCode;

	print_begin_test( function );

	result=TESI_Local_ReloadWithAuth("ooo","sss");
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Key_LoadKey", result );
		exit( result );
	}

	result=TESI_Local_CreateBindKey(&hKey,NULL,"sss","kkk");
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Key_LoadKey", result );
		exit( result );
	}

	TESI_Local_WriteKeyBlob(hKey,"bind/bindkey");
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Key_LoadKey", result );
		exit( result );
	}
	TESI_Local_WritePubKey(hKey,"bind/bindpubkey");
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Key_LoadKey", result );
		exit( result );
	}
		// Load the newly created key
	TESI_Local_Fin();
	return ;
}
