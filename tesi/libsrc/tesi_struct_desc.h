#ifndef TESI_STRUCT_DESC_H
#define TESI_STRUCT_DESC_H

#include "../include/struct_deal.h"

static struct struct_elem_attr tesi_sign_data_desc[] = 
{
	{"datalen",TPM_TYPE_UINT32,sizeof(UINT32),NULL,NULL},
	{"data",CUBE_TYPE_DEFINE,sizeof(BYTE),NULL,"datalen"},
	{"signlen",TPM_TYPE_UINT32,sizeof(UINT32),NULL,NULL},
	{"sign",CUBE_TYPE_DEFINE,sizeof(BYTE),NULL,"signlen"},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};
static struct struct_elem_attr tpm_key_parms_desc[] = 
{
	{"algorithmID",TPM_TYPE_UINT32,sizeof(UINT32),NULL,NULL},
	{"encScheme",TPM_TYPE_UINT16,sizeof(UINT16),NULL,NULL},
	{"sigScheme",TPM_TYPE_UINT16,sizeof(UINT16),NULL,NULL},
	{"parmSize",TPM_TYPE_UINT32,sizeof(UINT32),NULL,NULL},
	{"parms",CUBE_TYPE_DEFINE,1,NULL,"parmSize"},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};

static struct struct_elem_attr tpm_identity_req_desc[] = 
{
	{"asymBlobSize",TPM_TYPE_UINT32,0,NULL,NULL},
	{"symBlobSize",TPM_TYPE_UINT32,0,NULL,NULL},
	{"asymAlgorithm",CUBE_TYPE_SUBSTRUCT,0,&tpm_key_parms_desc,NULL},
	{"symAlgorithm",CUBE_TYPE_SUBSTRUCT,0,&tpm_key_parms_desc,NULL},
	{"asymBlob",CUBE_TYPE_DEFINE,1,NULL,"asymBlobSize"},
	{"symBlob",CUBE_TYPE_DEFINE,1,NULL,"symBlobSize"},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL,NULL}
};

#endif
