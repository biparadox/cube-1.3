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
 *	Tspi_TPM_TakeOwnership01.c
 *
 * DESCRIPTION
 *	This test will verify that Tspi_TPM_TakeOwnership succeeds when a
 *	handle to the public endorsement key is passed in to it explicitly.
 *
 * ALGORITHM
 *	Setup:
 *		Create Context
 *		Connect Context
 *		Get TPM Object
 *		Get Policy Object
 *		Set Secret
 *		Get Public Endorsement Key
 *		Create SRK
 *		Get Policy Object
 *		Set Secret
 *
 *	Test:
 *		Call TPM_TakeOwnership then if it does not succeed
 *		Make sure that it returns the proper return codes
 *		Print results
 *
 *	Cleanup:
 *		Free memory relating to hContext
 *		Close context
 *
 * USAGE
 *      First parameter is --options
 *                         -v or --version
 *      Second parameter is the version of the test case to be run
 *      This test case is currently only implemented for v1.1
 *
 * HISTORY
 *      Megan Schneider, mschnei@us.ibm.com, 7/06.
 *      Kent Yoder, kyoder@users.sf.net
 *
 * RESTRICTIONS
 *	None.
 */

#include <stdio.h>
#include "../include/tesi.h"

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

int
main_v1_1( void )
{
	char			*function = "TESI_clear";
	TSS_RESULT		result;
	
	result=TESI_Local_Reload();


	if ( result != TSS_SUCCESS )
	{
		printf("%s %s\n",function,tss_err_string(result));
		return result;
	}
	else
	{
		printf("%s success\n",function);
	}
	result=TESI_Local_ClearOwner("ooo");
	if ( result != TSS_SUCCESS )
	{
		printf("%s %s\n",function,tss_err_string(result));
		return result;
	}

	return result;
}
