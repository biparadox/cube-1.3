#ifndef TESI_REPORT_H
#define TESI_REPORT_H

#define PCR_SELECT_NUM 16
#define PCR_SIZE  20
enum dtype_tesi_report
{
	DTYPE_TESI_REPORT=0x604,
};

enum subtype_tesi_report_struct
{
	SUBTYPE_PCR_SELECTION=0x01,
	SUBTYPE_PCR_COMPOSITE,
	SUBTYPE_QUOTE_INFO,
	SUBTYPE_TSS_VALIDATION,
	SUBTYPE_PCR_SET
};

struct tpm_pcr_selection { 
    UINT16 size_of_select;			/* The size in bytes of the pcrSelect structure */
    BYTE * pcr_select;       /* This SHALL be a bit map that indicates if a PCR
                                                   is active or not */
}__attribute__((packed));

struct tpm_pcr_composite
{
	struct tpm_pcr_selection pcr_select;
        int value_size;
	BYTE * pcr_value;
}__attribute((packed));

struct tpm_pcr_set
{
	BYTE uuid[DIGEST_SIZE];        //the uuid of pcr composite ,it is the digests of the tcm_pcr_composite's value,we can find tpm_pcr_composite by it
        int trust_level;
	char * policy_describe;	
}__attribute((packed));

struct tpm_quote_info{
	BYTE version[4]; 
	BYTE fixed[4];
	BYTE digestValue[20];
	BYTE externalData[20];
}__attribute((packed));

struct tpm_validation
{
	BYTE version[4];   // TSS版本号
	UINT32 ulExternalDataLength;  //外部数据长度
	BYTE* rgbExternalData;       //外部数据内容
	UINT32 ulDataLength;        //数据长度
	BYTE* rgbData;             //数据内容
	UINT32 ulValidationLength;   //验证数据长度
	BYTE* rgbValdationData;     //验证数据内容
}__attribute((packed));

//void * verify_key_certify_struct(void * key_cert_file,char * keyuuid,char * aikuuid);

#endif
