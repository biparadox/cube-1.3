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

#include <stdio.h>
#include "../include/tesi.h"
#define DIGEST_SIZE 32


int main( void )
{
	char			*function = "Tspi_TESI_Quote";

	TSS_RESULT result;
	TSS_HPCRS hPcrComposite;
	BYTE digest[DIGEST_SIZE];
	BYTE PcrValue[PCR_SIZE];
	BYTE PcrValue1[PCR_SIZE];
	int index=8;

	result=TESI_Local_ReloadWithAuth("ooo","sss");

	if ( result != TSS_SUCCESS )
	{
		printf("TESI_Local_Load Err!\n");
		return result;
	}
	result=TESI_Local_CreatePcr(&hPcrComposite);
	if ( result != TSS_SUCCESS )
	{
		printf("TESI_Local_Load Err!\n");
		return result;
	}
	
	char * measure_text="test measure string, we should use sm3 hash algorithm to compress it to digest[DIGEST_SIZE]";
	calculate_context_sm3(measure_text,strlen(measure_text),digest);	
	

	result=TESI_Local_PcrExtend(index,digest);
	if ( result != TSS_SUCCESS )
	{
		printf("TESI_Local_PcrExtend Err!\n");
		return result;
	}
	
	result=TESI_Local_PcrRead(index,PcrValue);
	if ( result != TSS_SUCCESS )
	{
		printf("TESI_Local_PcrRead Err!\n");
		return result;
	}

	TESI_Local_Fin();
	return result;

}
