#ifndef TESI_AIK_STRUCT_H
#define TESI_AIK_STRUCT_H

enum dtype_aik_struct
{
	DTYPE_AIK_STRUCT=0x603,
};

enum subtype_aik_struct
{
	SUBTYPE_AIK_USER_INFO=0x01,
	SUBTYPE_AIK_CERT_INFO,
	SUBTYPE_CA_CERT
};


struct aik_user_info  
{
	BYTE uuid[DIGEST_SIZE];
	char * org;
	char user_id[DIGEST_SIZE];
	char * user_name;
	char * role;
}__attribute__((packed));


struct aik_cert_info{                 // CETI
	BYTE machine_uuid[DIGEST_SIZE];
	struct aik_user_info user_info;
	BYTE pubkey_uuid[DIGEST_SIZE];
}__attribute__((packed));

struct ca_cert{
	struct aik_cert_info reqinfo;
	char AIPubKey_uuid[DIGEST_SIZE];
}__attribute__((packed));

/*
static struct struct_elem_attr tesi_sign_data_desc[] = 
{
	{"datalen",TPM_TYPE_UINT32,sizeof(UINT32),NULL},
	{"data",OS210_TYPE_DEFINE,sizeof(BYTE),"datalen"},
	{"signlen",TPM_TYPE_UINT32,sizeof(UINT32),NULL},
	{"sign",OS210_TYPE_DEFINE,sizeof(BYTE),"signlen"},
	{NULL,OS210_TYPE_ENDDATA,0,NULL}
};
static struct struct_elem_attr tpm_key_parms_desc[] = 
{
	{"algorithmID",TPM_TYPE_UINT32,sizeof(UINT32),NULL},
	{"encScheme",TPM_TYPE_UINT16,sizeof(UINT16),NULL},
	{"sigScheme",TPM_TYPE_UINT16,sizeof(UINT16),NULL},
	{"parmSize",TPM_TYPE_UINT32,sizeof(UINT32),NULL},
	{"parms",OS210_TYPE_DEFINE,1,"parmSize"},
	{NULL,OS210_TYPE_ENDDATA,0,NULL}
};

static struct struct_elem_attr tpm_identity_req_desc[] = 
{
	{"asymBlobSize",TPM_TYPE_UINT32,sizeof(UINT32),NULL},
	{"symBlobSize",TPM_TYPE_UINT32,sizeof(UINT32),NULL},
	{"asymAlgorithm",OS210_TYPE_ORGCHAIN,0,tpm_key_parms_desc},
	{"symAlgorithm",OS210_TYPE_ORGCHAIN,0,tpm_key_parms_desc},
	{"asymBlob",OS210_TYPE_DEFINE,1,"asymBlobSize"},
	{"symBlob",OS210_TYPE_DEFINE,1,"symBlobSize"},
	{NULL,OS210_TYPE_ENDDATA,0,NULL}
};
*/
#endif
