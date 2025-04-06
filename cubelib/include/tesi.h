#ifndef TCG_TSS_ENCAPSULATE_H
#define TCG_TSS_ENCAPSULATE_H
#include <tss/tddl_error.h>
#include <tss/tcs_error.h>
#include <tss/tspi.h>

//typedef UINT32 TPM_ALGORITHM_ID;                            /* 1.1b */
//typedef UINT16 TPM_ENC_SCHEME;                              /* 1.1b */
//typedef UINT16 TPM_SIG_SCHEME;                              /* 1.1b */
#define PCR_SIZE 20

char * tss_err_string(TSS_RESULT);
typedef struct tagsigndata
{
	int datalen;
	BYTE * data;
	int signlen;
	BYTE * sign;

}__attribute__((packed)) TESI_SIGN_DATA;

typedef struct tagtpm_key_parms                              /* 1.1b */
{
    TPM_ALGORITHM_ID  algorithmID; //UINT32
    TPM_ENC_SCHEME    encScheme;   //UINT16
    TPM_SIG_SCHEME    sigScheme;   //UINT16
    UINT32            parmSize;   
    BYTE         *parms;
} __attribute__((packed)) tpm_key_parms;


typedef struct tagtpm_identity_req                       /* 1.1b */
{
    UINT32         asymSize;
    UINT32         symSize;
    tpm_key_parms asymAlgorithm;
    tpm_key_parms symAlgorithm;
    BYTE      *asymBlob;
    BYTE      *symBlob;
} __attribute__((packed)) tpm_identity_req;

TSS_RESULT TESI_Local_Init(char * pwdo,char * pwds);
TSS_RESULT TESI_Local_Reload();
TSS_RESULT TESI_Local_ReloadWithAuth(char * pwdo,char * pwds);
TSS_RESULT TESI_Local_ActiveSRK(char * pwds);
void TESI_Local_Fin();
TSS_RESULT TESI_Local_GetRandom(void * buf,int num);
TSS_RESULT TESI_Local_GetPubEK(char * name,char * pwdo);
TSS_RESULT TESI_Local_GetPubEKWithUUID(char ** uuid,char * pwdo);
TSS_RESULT TESI_Local_LoadKey(TSS_HKEY hKey,TSS_HKEY hWrapKey, char * pwdk);
TSS_RESULT TESI_GetPubKeyFromCA(TSS_HKEY * hCAKey,char * name);
TSS_RESULT TESI_Local_CreateSignKey(TSS_HKEY * hKey,TSS_HKEY hWrapKey,char *pwdw,char * pwdk);
TSS_RESULT TESI_Local_CreateBindKey(TSS_HKEY * hKey,TSS_HKEY hWrapKey,char * pwdw,char * pwdk);
TSS_RESULT TESI_Local_CreateStorageKey(TSS_HKEY * hKey,TSS_HKEY hWrapKey,char * pwdw,char * pwdk);

TSS_RESULT TESI_Local_WriteKeyBlob(TSS_HKEY hKey,char * name);
TSS_RESULT TESI_Local_ReadKeyBlob(TSS_HKEY * hKey,char * name);
TSS_RESULT TESI_Local_WritePubKey(TSS_HKEY hPubKey,char * name);
TSS_RESULT TESI_Local_ReadPubKey(TSS_HKEY * hPubKey,char * name);
TSS_RESULT TESI_Local_SignFile(char * filename,TSS_HKEY hSignKey,char * signname,char * pwdk);
TSS_RESULT TESI_Local_VerifyFile(char * filename,TSS_HKEY hSignKey,char * signname);
TSS_RESULT TESI_Local_Seal(char * plainname, TSS_HKEY hKey, char * ciphername,TSS_HPCRS hPcr );
TSS_RESULT TESI_Local_UnSeal(char * plainname, TSS_HKEY hKey, char * ciphername);
TSS_RESULT TESI_Local_Bind(char * plainname, TSS_HKEY hKey, char * ciphername);
TSS_RESULT TESI_Local_BindBuffer(void * inbuffer, int inlength, TSS_HKEY hKey, void ** outbuffer, int * outlength);
TSS_RESULT TESI_Local_UnBind(char * ciphername, TSS_HKEY hKey, char * plainname);
TSS_RESULT TESI_Local_UnBindBuffer(void * inbuffer,int inlength, TSS_HKEY hKey, void** outbuffer, int * outlength);
TSS_RESULT TESI_Local_ClearOwner(char * pwdo);

TSS_RESULT TESI_Local_CreatePcr(TSS_HPCRS * hPcr);
TSS_RESULT TESI_Local_SetCurrPCRValue(TSS_HPCRS hPcr,UINT32 Index);
TSS_RESULT TESI_Local_SelectPcr(TSS_HPCRS hPcr,UINT32 Index);
TSS_RESULT TESI_Local_PcrExtend(UINT32 index, BYTE * value);
TSS_RESULT TESI_Local_PcrRead(UINT32 index, BYTE * value);
TSS_RESULT TESI_Local_Quote(TSS_HKEY hIdentKey,TSS_HPCRS hPcr,char *name);
TSS_RESULT TESI_Local_VerifyValData(TSS_HKEY hIdentKey,char * name);

TSS_RESULT TESI_Mig_CreateSignKey(TSS_HKEY * hKey,TSS_HKEY hWrapKey,char * pwdw,char * pwdk);
TSS_RESULT TESI_Mig_CreateRewrapTicket(TSS_HKEY hMigKey,char * ticketname,char * pwdo);
TSS_RESULT TESI_Mig_CreateTicket(TSS_HKEY hMigKey,char * ticketname,char * pwdo);
TSS_RESULT TESI_Mig_CreateKeyBlob(TSS_HKEY hKey,TSS_HKEY hParentKey,char * pwdk,char * ticket,char * MigBlob);
TSS_RESULT TESI_Mig_ConvertMigBlob(TSS_HKEY * hKey,TSS_HKEY hParentKey,char *MigBlob);

TSS_RESULT TESI_AIK_CreatePubIdentKey(TSS_HKEY * hKey);
TSS_RESULT TESI_AIK_CreateIdentKey(TSS_HKEY * hKey,TSS_HKEY * hWrapKey,char * pwdw,char * pwdk);
TSS_RESULT  TESI_AIK_GenerateReq(TSS_HKEY hCAKey,int labelLen,BYTE * labelData, TSS_HKEY hAIKey,char * req);
TSS_RESULT TESI_AIK_VerifyReq(void * privkey,TSS_HKEY hCAKey, char * req,TSS_HKEY * hAIKey,TPM_IDENTITY_PROOF * identityProof );
TSS_RESULT TESI_AIK_CreateAIKCert(TSS_HKEY hIdentityKey,void *privkey,BYTE * data,int datasize,
	char * pubek,char * repname);
TSS_RESULT TESI_AIK_Activate(TSS_HKEY hAIK, char * req, TESI_SIGN_DATA * signdata);
TSS_RESULT TESI_AIK_ReadCABlob(void * blob, char * blob_file);
TSS_RESULT TESI_AIK_WriteCABlob(void * blob, char * blob_file);
int TESI_AIK_CreateSignData(TESI_SIGN_DATA * signdata,void * privkey,BYTE * blob,int blobsize);
int TESI_AIK_VerifySignData(TESI_SIGN_DATA * signdata,char *name);
TSS_RESULT TESI_Local_Bind(char * plainname, TSS_HKEY hKey, char * ciphername);
TSS_RESULT TESI_Local_UnBind(char * plainname, TSS_HKEY hKey, char * ciphername);
TSS_RESULT TESI_Local_BindBuffer(void * inbuffer,int inlength, TSS_HKEY hKey, void** outbuffer, int * outlength);
TSS_RESULT TESI_Local_UnBindBuffer(void * inbuffer,int inlength, TSS_HKEY hKey, void** outbuffer, int * outlength);


TSS_RESULT ReadPubKey(void ** rsa,char * keyname);
TSS_RESULT WritePubKey(void * rsa,char * keyname);
TSS_RESULT ReadPrivKey(void ** rsa,char * keyname,char * pass_phrase);
TSS_RESULT WritePrivKey(void * rsa,char * keyname,char * pass_phrase);

int WriteSignDataToFile(void * data,char * name);
int ReadSignDataFromFile(void * data,char * name);
TSS_RESULT WriteValidation(TSS_VALIDATION * valData,char * name);
TSS_RESULT ReadValidation(TSS_VALIDATION * valData,char * name);

int TESI_Report_CertifyKey(TSS_HKEY hKey,TSS_HKEY hAIKey, char * valdataname);
int TESI_Report_GetKeyDigest(TSS_HKEY * hKey,BYTE * digest );
/*
TSS_RESULT TESI_AIK_Request(TSS_HKEY hCAKey,char * labelname,TSS_HKEY * hAIKey,char * reqname);
TSS_RESULT TESI_AIK_Sign(char * reqname, char * CAKeyname,char * keypass,char * userek, char * certdata);
TSS_RESULT TESI_AIK_Active(char * reqname, TSS_HKEY hCAKey,TSS_HKEY hAIKey,char * certname);
*/
#endif
