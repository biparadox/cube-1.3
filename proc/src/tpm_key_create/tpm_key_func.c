#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>

#include "data_type.h"
#include "errno.h"
#include "alloc.h"
#include "string.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "message.h"
#include "ex_module.h"
#include "tesi.h"

#include "file_struct.h"
#include "tesi_key.h"
#include "tesi_aik_struct.h"
#include "tpm_key_create.h"

int print_error(char * str, int result)
{
	printf("%s %s",str,tss_err_string(result));
}


#define MAX_WRAPPED_LAYER 10
TSS_RESULT _load_tpm_key(BYTE * wrapkey_uuid, TSS_HKEY * hKey)
{
	struct vTPM_wrappedkey * wrapkey_frame[MAX_WRAPPED_LAYER];
	DB_RECORD * db_record[MAX_WRAPPED_LAYER];
	int i=0;
	struct vTPM_wrappedkey * temp_frame;
	BYTE digest[DIGEST_SIZE];
	TSS_HKEY hWrapKey;
	TSS_RESULT result;
	
	Memset(digest,0,DIGEST_SIZE);
	db_record[0]=memdb_find_first(DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,"uuid",wrapkey_uuid);
	wrapkey_frame[0]=db_record[0]->record;
	Memset(db_record,0,sizeof(db_record));
	Memset(&wrapkey_frame[1],0,sizeof(wrapkey_frame)-sizeof(void *));

	for(i=0;i<MAX_WRAPPED_LAYER;i++)
	{
		if(db_record[i]->tail!=NULL)
			break;
		if(Memcpy(digest,wrapkey_frame[i]->wrappedkey_uuid,DIGEST_SIZE)==0)
			break;
		db_record[i+1]=memdb_find_first(DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,"uuid",wrapkey_frame[i]->wrappedkey_uuid);
		wrapkey_frame[i+1]=db_record[i+1]->record;
	}
	if(i>=MAX_WRAPPED_LAYER)
		return -EINVAL;	

	// get hWrapKey
	hWrapkey=NULL;
	for(;i<=0;i--)
	{
		if(db_record[i]->tail==NULL)
		{
			result=TESI_Local_ReadKeyBlob(&hTempKey,wrapkey_frame[i]->key_filename);
			if(result!=TSS_SUCCESS)
			{
				return result;	
			}
			result=TESI_Local_LoadKey(hTempKey,hWrapKey,wrapkey_frame[i]->keypass);
			if(result!=TSS_SUCCESS)
			{
				return result;	
			}
			hWrapKey=kTempKey;
			db_record[i]->tail=hTempKey;	
		}	
	}
	// get hkey
	if(result!=TSS_SUCCESS)
	{
		*hKey=NULL;
	}
	else
	{
		*hKey=hTempKey;
	}
	return result;	
}


int create_tpm_key(struct vTPM_wrappedkey * key_frame)
{
	TSS_RESULT result;
	TSS_HKEY   hKey;
	TSS_HKEY   hWrapKey;
	int i;
	int ret;

	char filename[DIGEST_SIZE*3];
	
	char buffer[1024];
	char digest[DIGEST_SIZE];
	int blobsize=0;
	int fd;

	result=_load_tpm_key(key_frame->wrapkey_uuid,&hWrapKey);

	if(result!=TSS_SUCCESS)
		return -EINVAL;
	if(hWrapKey==NULL)
		key_frame->issrkwrapped=0;
	else
		key_frame->issrkwrapped=1;

	key_frame->key_size=1024;	
		
	switch(key_type)
	{
		case TPM_KEY_SIGNING:
			result=TESI_Local_CreateSignKey(&hKey,hWrapKey,NULL,key_frame->keypass);
			break;	
				
		case TPM_KEY_STORAGE:
			result=TESI_Local_CreateStorageKey(&hKey,hWrapKey,NULL,key_frame->keypass);
			break;	
		case TPM_KEY_IDENTITY:
			result=TESI_AIK_CreateIdentityKey(&hKey,hWrapKey,NULL,key_frame->keypass);
			break;	
		case TPM_KEY_BIND:			
			result=TESI_Local_CreateBindKey(&hKey,hWrapKey,NULL,key_frame->keypass);
			break;	
		default:
			return -EINVAL;
		}
	}

	if (result != TSS_SUCCESS) {
		printf("Create AIK error %s!\n", tss_err_string(result));
		exit(result);
	}

	TESI_Local_WriteKeyBlob(hKey,"privkey/temp");

	ret=convert_uuidname("privkey/temp",".key",key_frame->uuid,filename);

	if(ret<0)
		return ret;
	key_frame->key_filename=dup_str(filename,DIGEST_SIZE*4);


	DB_RECORD * db_record;
	db_record = memdb_store(key_frame,DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,NULL);
	if(db_record==NULL)
		return -EINVAL;
	db_record->tail=hKey;

	return 0;
}
