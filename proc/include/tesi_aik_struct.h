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
	SUBTYPE_CA_CERT,
	SUBTYPE_AIK_INFO

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
aik关联结构
*/
struct key_manage_aik_info
{
	BYTE machine_uuid[DIGEST_SIZE];
	char * user_name;
	BYTE aik_pub_uuid[DIGEST_SIZE];
	BYTE aik_pri_uuid[DIGEST_SIZE];
	BYTE aik_cert_uuid[DIGEST_SIZE];
}__attribute__((packed));

#endif
