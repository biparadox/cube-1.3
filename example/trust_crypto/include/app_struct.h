#ifndef TRUST_DEMO_APPSTRUCT_H
#define TRUST_DEMO_APPSTRUCT_H

enum  trust_demo_type
{
	DTYPE_TRUST_DEMO=0x1010
};
enum trust_demo_subtype
{
	SUBTYPE_KEY_INFO=0x01,
	SUBTYPE_ENCDATA_INFO,
	SUBTYPE_FILECRYPT_INFO,
	SUBTYPE_FILE_OPTION
};

struct trust_demo_keyinfo
{
        BYTE uuid[DIGEST_SIZE];    
        BYTE vtpm_uuid[DIGEST_SIZE]; 
	int  ispubkey;
        char * owner;   
	char * peer;
	char * usage;
	char * passwd;
	int  key_type;
} __attribute__((packed));

struct trust_encdata_info
{
        BYTE uuid[DIGEST_SIZE];    
        BYTE vtpm_uuid[DIGEST_SIZE]; 
        BYTE enckey_uuid[DIGEST_SIZE]; 
        char *filename; 
} __attribute__((packed));

struct trust_filecrypt_info
{
        BYTE plain_uuid[DIGEST_SIZE];    
        BYTE cipher_uuid[DIGEST_SIZE]; 
        BYTE vtpm_uuid[DIGEST_SIZE]; 
        BYTE encdata_uuid[DIGEST_SIZE]; 
} __attribute__((packed));

struct trust_file_option
{
        char * filename;
	char * option;    
} __attribute__((packed));

/*
秘钥管理主类别
*/
enum key_manage_type
{
	DTYPE_KEY_MANAGE=0x2100;
};

/*
秘钥管理子类别
*/
enum key_manage_subtype
{
	SUBTYPE_AIK_BIND=0x01;
};

/*
aik关联结构
*/
struct key_manage_aik_bind
{
	BYTE machine_uuid[DIGEST_SIZE];
	char * user_name;
	BYTE aik_pub_uuid[DIGEST_SIZE];
	BYTE aik_pri_uuid[DIGEST_SIZE];
	BYTE aik_cert_uuid[DIGEST_SIZE];
}__attribute__((packed));
#endif
