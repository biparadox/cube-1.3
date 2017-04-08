#ifndef TRUST_DEMO_APPSTRUCT_H
#define TRUST_DEMO_APPSTRUCT_H

enum  trust_demo_type
{
	DTYPE_TRUST_DEMO=0x1010
};
enum trust_demo_subtype
{
	SUBTYPE_KEY_INFO=0x01,
};

struct trust_demo_keyinfo
{
        BYTE uuid[DIGEST_SIZE];    
        BYTE vtpm_uuid[DIGEST_SIZE]; 
        char * owner;   
	char * peer;
	char * usage;
	int  key_type;
} __attribute__((packed));

#endif
