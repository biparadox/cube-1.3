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
	char			*function = "Tspi_TESI_getpubek";
	TSS_RESULT		result;
	TSS_HKEY 		hCAKey;
	TSS_HKEY 		hSignKey;
	TSS_HKEY 		hReloadKey;
	TSS_HKEY 		hReloadPubKey;
	
	char uuid[DIGEST_SIZE*2];

	result=TESI_Local_ReloadWithAuth("ooo","sss");

	if ( result != TSS_SUCCESS )
	{
		printf("TESI_Local_Load Err!\n");
		return result;
	}
	TESI_Local_GetPubEKWithUUID(uuid,"ooo");

	TESI_Local_Fin();
	return result;
}
