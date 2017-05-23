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


#define MAX_WRAPPED_LAYER 10
TSS_RESULT _load_tpm_key(BYTE * wrapkey_uuid, TSS_HKEY * hKey)
{
	struct vTPM_wrappedkey * wrapkey_frame[MAX_WRAPPED_LAYER];
	DB_RECORD * db_record[MAX_WRAPPED_LAYER];
	int i=0;
	struct vTPM_wrappedkey * temp_frame;
	BYTE digest[DIGEST_SIZE];
	TSS_HKEY hWrapKey;
	TSS_HKEY hTempKey;
	TSS_RESULT result;
	char filename[DIGEST_SIZE*7];
	
	Memset(digest,0,DIGEST_SIZE);
	if(Memcmp(digest,wrapkey_uuid,DIGEST_SIZE)==0)
	{
		*hKey=NULL;
		return TSS_SUCCESS;
	}

	Memset(db_record,0,sizeof(db_record));
	Memset(wrapkey_frame,0,sizeof(wrapkey_frame));
	db_record[0]=memdb_find_first(DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,"uuid",wrapkey_uuid);
	if(db_record[0]==NULL)
		return -EINVAL;
	wrapkey_frame[0]=db_record[0]->record;

	for(i=0;i<MAX_WRAPPED_LAYER;i++)
	{
		if(Memcmp(digest,wrapkey_frame[i]->wrapkey_uuid,DIGEST_SIZE)==0)
			break;
		db_record[i+1]=memdb_find_first(DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,"uuid",wrapkey_frame[i]->wrapkey_uuid);
		wrapkey_frame[i+1]=db_record[i+1]->record;
	}
	if(i>=MAX_WRAPPED_LAYER)
		return -EINVAL;	

	// get hWrapKey
	hWrapKey=NULL;
	for(;i>=0;i--)
	{
		if(db_record[i]->tail==NULL)
		{
			Strcpy(filename,wrapkey_frame[i]->key_filename);
			filename[Strlen(filename)-4]=0;
			result=TESI_Local_ReadKeyBlob(&hTempKey,filename);
			if(result!=TSS_SUCCESS)
			{
				return result;	
			}
			result=TESI_Local_LoadKey(hTempKey,hWrapKey,wrapkey_frame[i]->keypass);
			if(result!=TSS_SUCCESS)
			{
				return result;	
			}
			hWrapKey=hTempKey;
			db_record[i]->tail=hTempKey;	
		}	
		else
		{
			hTempKey=db_record[i]->tail;
			result=TESI_Local_LoadKey(hTempKey,hWrapKey,wrapkey_frame[i]->keypass);
			if(result!=TSS_SUCCESS)
			{
				return result;	
			}
			hWrapKey=hTempKey;
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


int create_tpm_key(struct vTPM_wrappedkey * key_frame,struct vTPM_publickey ** pubkey)
{
	TSS_RESULT result;
	TSS_HKEY   hKey;
	TSS_HKEY   hWrapKey;
	int i;
	int ret;

	char filename[DIGEST_SIZE*3];
	
	char buffer[DIGEST_SIZE*8];
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
		
	switch(key_frame->key_type)
	{
		case TPM_KEY_SIGNING:
			result=TESI_Local_CreateSignKey(&hKey,hWrapKey,NULL,key_frame->keypass);
			break;	
				
		case TPM_KEY_STORAGE:
			result=TESI_Local_CreateStorageKey(&hKey,hWrapKey,NULL,key_frame->keypass);
			break;	
		case TPM_KEY_IDENTITY:
			result=TESI_AIK_CreateIdentKey(&hKey,hWrapKey,NULL,key_frame->keypass);
			break;	
		case TPM_KEY_BIND:			
			result=TESI_Local_CreateBindKey(&hKey,hWrapKey,NULL,key_frame->keypass);
			break;	
		default:
			return -EINVAL;
	}

	if (result != TSS_SUCCESS) {
		printf("Create AIK error %s!\n", tss_err_string(result));
		exit(result);
	}

	result=TESI_Local_LoadKey(hKey,hWrapKey,NULL);
	if(result!=TSS_SUCCESS)
	{
			return result;	
	}

	struct vTPM_publickey * pubkey_frame;
	pubkey_frame=Talloc(sizeof(*pubkey_frame));
	if(pubkey_frame==NULL)
		return -ENOMEM;
	*pubkey=pubkey_frame;
	// Write the wrapped Key
	TESI_Local_WriteKeyBlob(hKey,"privkey/temp");

	ret=convert_uuidname("privkey/temp",".key",key_frame->uuid,filename);


	if(ret<0)
		return ret;
	getcwd(buffer,DIGEST_SIZE*5-6);
	Strcat(buffer,"/");	
	Strcat(buffer,filename);	
	
	
	key_frame->key_filename=dup_str(buffer,DIGEST_SIZE*7);


	// Write the public key

	TESI_Local_WritePubKey(hKey,"pubkey/temp");
	ret=convert_uuidname("pubkey/temp",".pem",pubkey_frame->uuid,filename);
	if(ret<0)
		return ret;
	Memcpy(pubkey_frame->vtpm_uuid,key_frame->vtpm_uuid,DIGEST_SIZE);
	pubkey_frame->ispubek=0;
	pubkey_frame->key_type=key_frame->key_type;
	pubkey_frame->key_alg=key_frame->key_alg;
	pubkey_frame->key_size=key_frame->key_size;
	Memcpy(pubkey_frame->key_binding_policy_uuid,key_frame->key_binding_policy_uuid,DIGEST_SIZE);
	Memcpy(pubkey_frame->privatekey_uuid,key_frame->uuid,DIGEST_SIZE);
	pubkey_frame->keypass=NULL;

	getcwd(buffer,DIGEST_SIZE*12);
	Strcat(buffer,"/");	
	Strcat(buffer,filename);	
	
	pubkey_frame->key_filename=dup_str(buffer,DIGEST_SIZE*7);

	Memcpy(key_frame->pubkey_uuid,pubkey_frame->uuid,DIGEST_SIZE);

	// Write the privkey and pubkey
	DB_RECORD * db_record;
	db_record = memdb_store(key_frame,DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,NULL);
	if(db_record==NULL)
		return -EINVAL;
	db_record->tail=hKey;
	db_record = memdb_store(pubkey_frame,DTYPE_TESI_KEY_STRUCT,SUBTYPE_PUBLIC_KEY,NULL);
	if(db_record==NULL)
		return -EINVAL;
	db_record->tail=hKey;

	return 0;
}

int find_pubkey_by_privuuid(BYTE * priv_uuid,struct vTPM_publickey ** pubkey_frame)
{
	
	struct vTPM_wrappedkey * key_frame;
	*pubkey_frame=NULL;
	DB_RECORD * db_record;
	db_record=memdb_find_first(DTYPE_TESI_KEY_STRUCT,SUBTYPE_WRAPPED_KEY,"uuid",priv_uuid);
	if(db_record==NULL)
		return 0;

	key_frame=db_record->record;	

	db_record=memdb_find_first(DTYPE_TESI_KEY_STRUCT,SUBTYPE_PUBLIC_KEY,"uuid",key_frame->pubkey_uuid);
	if(db_record==NULL)
		return 0;
	*pubkey_frame=db_record->record;
	return 1;
	
}


int find_tpm_key(struct vTPM_wrappedkey * key_frame,struct vTPM_publickey ** pubkey)
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
	
	fd=access(key_frame->key_filename,O_RDONLY);
	if(fd<0)
		return 0;
		
	calculate_sm3(key_frame->key_filename,digest);

	ret=Memcmp(digest,key_frame->uuid,DIGEST_SIZE);
	if(ret!=0)
		return 0;

	ret=find_pubkey_by_privuuid(key_frame->uuid,pubkey);
	if(ret!=1)
		return 0;

	struct 	vTPM_publickey * pubkey_frame=*pubkey;

	if(!Isemptyuuid(key_frame->pubkey_uuid))
	{
		ret=Memcmp(key_frame->pubkey_uuid,pubkey_frame->uuid,DIGEST_SIZE);
		if(ret!=0)
			return 0;
	}
	else
	{
		Memcpy(key_frame->pubkey_uuid,pubkey_frame->uuid,DIGEST_SIZE);
	}
	
	
	fd=access(pubkey_frame->key_filename,O_RDONLY);
	if(fd<0)
		return 0;
		
	calculate_sm3(pubkey_frame->key_filename,digest);

	ret=Memcmp(digest,pubkey_frame->uuid,DIGEST_SIZE);
	if(ret!=0)
		return 0;

	_load_tpm_key(key_frame->uuid,&hKey);

	return 1;
}
