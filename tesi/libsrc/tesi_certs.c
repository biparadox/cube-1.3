#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>


#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>
//#include "tpmfunc.h"
#include "string.h"
#include "common.h"
#include "tesi.h"
#include "struct_deal.h"
#include "crypto_func.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include "tesi_struct_desc.h"

TSS_HCONTEXT hContext;
TSS_HTPM hTPM;
TSS_HKEY hSRK;
TCPA_ALGORITHM_ID symAlg = TCPA_ALG_AES;
TSS_ALGORITHM_ID tssSymAlg = TSS_ALG_AES;

char * tss_err_string(TSS_RESULT result)
{
	return err_string(result);
}

/****************************************************************************/
/*									  */
/* Perform a SHA1 hash on a single buffer				   */
/*									  */
/****************************************************************************/
void TSS_sha1(void *input, unsigned int len, unsigned char *output)
{

	SHA_CTX sha;
   
	SHA1_Init(&sha);
	SHA1_Update(&sha,input,len);
	SHA1_Final(output,&sha);

}
TSS_RESULT WriteValidation(TSS_VALIDATION * valData,char * name)
{
	TSS_RESULT result;
	char filename[128];
	FILE * file;
	UINT32 datalen;

	sprintf(filename,"%s.val",name);
 	// read file text
	file = fopen(filename,"wb");
  	if (file == NULL)
   	{
   	   printf("Unable to write val file %s.\n",filename);
   	   return TSS_E_KEY_NOT_LOADED;
  	 }


   	result = fwrite(&(valData->versionInfo),1,sizeof(TSS_VERSION),file);
  	 if (result != sizeof(TSS_VERSION))
     	 {
	      printf("I/O Error writing valData\n");
  	    return TSS_E_KEY_NOT_LOADED;
      	 }


	 datalen=valData->ulExternalDataLength;
   	 result = fwrite(&datalen,1,sizeof(UINT32),file);
  	 if (result != sizeof(UINT32))
     	 {
	      printf("I/O Error writing valData\n");
  	      return TSS_E_KEY_NOT_LOADED;
      	 }

   	 result = fwrite(valData->rgbExternalData,1,datalen,file);
  	 if (result != datalen)
     	 {
	      printf("I/O Error writing valData\n");
  	      return TSS_E_KEY_NOT_LOADED;
      	 }

	 datalen=valData->ulDataLength;
   	 result = fwrite(&datalen,1,sizeof(UINT32),file);
  	 if (result != sizeof(UINT32))
     	 {
	      printf("I/O Error writing valData\n");
  	      return TSS_E_KEY_NOT_LOADED;
      	 }

   	 result = fwrite(valData->rgbData,1,datalen,file);
  	 if (result != datalen)
     	 {
	      printf("I/O Error writing valData\n");
  	      return TSS_E_KEY_NOT_LOADED;
	 }

	 datalen=valData->ulValidationDataLength;
   	 result = fwrite(&datalen,1,sizeof(UINT32),file);
  	 if (result != sizeof(UINT32))
     	 {
	      printf("I/O Error writing valData\n");
  	      return TSS_E_KEY_NOT_LOADED;
      	 }

   	 result = fwrite(valData->rgbValidationData,1,datalen,file);
  	 if (result != datalen)
     	 {
	      printf("I/O Error writing valData\n");
  	      return TSS_E_KEY_NOT_LOADED;
	 }
	 fclose(file);
	 return TSS_SUCCESS;
}

TSS_RESULT ReadValidation(TSS_VALIDATION * valData,char * name)
{
	TSS_RESULT result;
	char filename[128];
	FILE * file;
	UINT32 datalen;

	sprintf(filename,"%s.val",name);
 	// read file text
	file = fopen(filename,"rb");
  	if (file == NULL)
   	{
   	   printf("Unable to read val file %s.\n",filename);
   	   return TSS_E_KEY_NOT_LOADED;
  	 }


   	result = fread(&(valData->versionInfo),1,sizeof(TSS_VERSION),file);
  	 if (result != sizeof(TSS_VERSION))
     	 {
	      printf("I/O Error writing valData\n");
  	    return TSS_E_KEY_NOT_LOADED;
      	 }


   	 result = fread(&datalen,1,sizeof(UINT32),file);
  	 if (result != sizeof(UINT32))
     	 {
	      printf("I/O Error writing valData\n");
  	      return TSS_E_KEY_NOT_LOADED;
      	 }

	valData->rgbExternalData=(BYTE *)malloc(datalen);
	if(valData->rgbExternalData == NULL)
		return TSS_E_OUTOFMEMORY;

   	 result = fread((valData->rgbExternalData),1,datalen,file);
  	 if (result != datalen)
     	 {
	      printf("I/O Error writing valData\n");
  	      return TSS_E_KEY_NOT_LOADED;
      	 }
	 valData->ulExternalDataLength=datalen;

   	 result = fread(&datalen,1,sizeof(UINT32),file);
  	 if (result != sizeof(UINT32))
     	 {
	      printf("I/O Error writing valData\n");
  	      return TSS_E_KEY_NOT_LOADED;
      	 }

	valData->rgbData=(BYTE *)malloc(datalen);
	if(valData->rgbData == NULL)
		return TSS_E_OUTOFMEMORY;

   	 result = fread((valData->rgbData),1,datalen,file);
  	 if (result != datalen)
     	 {
	      printf("I/O Error writing valData\n");
  	      return TSS_E_KEY_NOT_LOADED;
	 }
	 valData->ulDataLength=datalen;

   	 result = fread(&datalen,1,sizeof(UINT32),file);
  	 if (result != sizeof(UINT32))
     	 {
	      printf("I/O Error writing valData\n");
  	      return TSS_E_KEY_NOT_LOADED;
      	 }

	valData->rgbValidationData=(BYTE *)malloc(datalen);
	if(valData->rgbValidationData == NULL)
		return TSS_E_OUTOFMEMORY;

   	 result = fread((valData->rgbValidationData),1,datalen,file);
  	 if (result != datalen)
     	 {
	      printf("I/O Error reading valData\n");
  	      return TSS_E_KEY_NOT_LOADED;
	 }
	 valData->ulValidationDataLength=datalen;
	 fclose(file);
	 return TSS_SUCCESS;
}

TSS_RESULT TESI_Local_GetRandom(void * buf,int num)
{
   	TSS_RESULT result;
	BYTE * temp_buf;
	result = Tspi_TPM_GetRandom( hTPM, num, &temp_buf);
   	if (result != TSS_SUCCESS) 
		return result;
	memcpy(buf,temp_buf,num);
	result = Tspi_Context_FreeMemory( hContext, temp_buf);
   	if (result != TSS_SUCCESS) 
		return result;
	return TSS_SUCCESS;	
}

TSS_RESULT TESI_Local_LoadKey(TSS_HKEY hKey,TSS_HKEY hWrapKey, char * pwdk)
{
   TSS_RESULT result;
  char kpass[20];   
  TSS_HPOLICY hPolicy;

  if(hWrapKey==NULL)
	hWrapKey=hSRK;
  if(pwdk!=NULL)
  {
  	TSS_sha1((unsigned char *)pwdk,strlen(pwdk),kpass);

		// Get The SRK policy 
  	 result = Tspi_GetPolicyObject(hKey,TSS_POLICY_USAGE,&hPolicy);
   	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_GetPolicyObject ", result);
		return result;
   	}

	
  	result = Tspi_Policy_SetSecret(hPolicy,TSS_SECRET_MODE_SHA1,
		sizeof(kpass),kpass);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		return result;
	}
    }	

    result=Tspi_Key_LoadKey(hKey,hWrapKey);
    if (result != TSS_SUCCESS) {
	  print_error("Tspi_Key_LoadKey", result);
	  return result;
    }
    return TSS_SUCCESS;
}

TSS_RESULT TESI_Local_ActiveSRK(char * pwds)
{
   TSS_RESULT result;
  char spass[20];   
  TSS_HPOLICY hPolicy;

  TSS_sha1((unsigned char *)pwds,strlen(pwds),spass);

		// Get The SRK policy 
   result = Tspi_GetPolicyObject(hSRK,TSS_POLICY_USAGE,&hPolicy);
   if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_GetPolicyObject ", result);
		return result;
   }

	
  result = Tspi_Policy_SetSecret(hPolicy,TSS_SECRET_MODE_SHA1,
		sizeof(spass),spass);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		return result;
	}

	return TSS_SUCCESS;
}

TSS_RESULT TESI_Local_Init(char * pwdo,char * pwds)
{
	TSS_HPOLICY		hPolicy;
	TSS_HPOLICY		hSrkPolicy;
	TSS_RESULT		result;
	TSS_FLAG	        initFlags;
	TSS_HKEY		hEndorsement;
	int pwd_len;
	BYTE opass[20];
	BYTE spass[20];

		// Create Context
	result = Tspi_Context_Create( &hContext );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init: Tspi_Context_Create", result );
		return result;
	}

		// Connect to Context
	result = Tspi_Context_Connect( hContext, get_server(GLOBALSERVER) );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init: Tspi_Context_Connect", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		return result;
	}

		// Retrieve TPM object of context
	result = Tspi_Context_GetTpmObject( hContext, &hTPM );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init: Tspi_Context_GetTpmObject", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		return result;
	}

	result = Tspi_GetPolicyObject( hTPM, TSS_POLICY_USAGE, &hPolicy );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init: Tspi_GetPolicyObject", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		return result;
	}

	pwd_len=strlen(pwdo);
	TSS_sha1((unsigned char *)pwdo,pwd_len,opass);

	result = Tspi_Policy_SetSecret( hPolicy, TSS_SECRET_MODE_SHA1,
					sizeof(opass),opass);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init: Tspi_Policy_SetSecret", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		return result;
	}

	result = Tspi_TPM_GetPubEndorsementKey( hTPM, 0, NULL, &hEndorsement );
	if ( result != TSS_SUCCESS )
	{
		hEndorsement=NULL_HKEY;
	}
	

		//Load Key By UUID
	initFlags = TSS_KEY_TSP_SRK;
	result = Tspi_Context_CreateObject( hContext, TSS_OBJECT_TYPE_RSAKEY,
						initFlags, &hSRK );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_CreateObject", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	result = Tspi_GetPolicyObject( hSRK, TSS_POLICY_USAGE,
					&hSrkPolicy );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init:Tspi_GetPolicyObject", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		return result;
	}
	pwd_len=strlen(pwds);
	TSS_sha1((unsigned char *)pwds,pwd_len,spass);

	result = Tspi_Policy_SetSecret( hSrkPolicy, TSS_SECRET_MODE_SHA1,
					sizeof(spass), spass);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Policy_SetSecret", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		return result;
	}


	result = Tspi_TPM_TakeOwnership( hTPM, hSRK, hEndorsement );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init:Tspi_TPM_TakeOwnership", result );
		return result;
	}
	return result;
}

TSS_RESULT TESI_Local_Reload()
{
	TSS_HPOLICY		hPolicy;
	TSS_HPOLICY		hSrkPolicy;
	TSS_RESULT		result;
	TSS_FLAG	        initFlags;

		// Create Context
	result = Tspi_Context_Create( &hContext );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init: Tspi_Context_Create", result );
		return result;
	}

		// Connect to Context
	result = Tspi_Context_Connect( hContext, get_server(GLOBALSERVER) );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init: Tspi_Context_Connect", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		return result;
	}

		// Retrieve TPM object of context
	result = Tspi_Context_GetTpmObject( hContext, &hTPM );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init: Tspi_Context_GetTpmObject", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		return result;
	}

	result = Tspi_GetPolicyObject( hTPM, TSS_POLICY_USAGE, &hPolicy );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init: Tspi_GetPolicyObject", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		return result;
	}


	result = Tspi_Context_LoadKeyByUUID(hContext,
				TSS_PS_TYPE_SYSTEM, SRK_UUID, &hSRK);
	if (result != TSS_SUCCESS) {
		print_error("LocalInit:Tspi_Context_LoadKeyByUUID", result);
		print_error_exit(nameOfFunction, err_string(result));
		Tspi_Context_Close(hContext);
		return result;
	}

	return result;
}

TSS_RESULT TESI_Local_ReloadWithAuth(char * pwdo,char * pwds )
{
	TSS_HPOLICY		hPolicy;
	TSS_HPOLICY		hSrkPolicy;
	TSS_RESULT		result;
	TSS_FLAG	        initFlags;
	int pwd_len;
	BYTE opass[20];
	BYTE spass[20];


		// Create Context
	result = Tspi_Context_Create( &hContext );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Reload: Tspi_Context_Create", result );
		return result;
	}

		// Connect to Context
	result = Tspi_Context_Connect( hContext, get_server(GLOBALSERVER) );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init: Tspi_Context_Connect", result );
		return result;
	}

		// Retrieve TPM object of context
	result = Tspi_Context_GetTpmObject( hContext, &hTPM );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init: Tspi_Context_GetTpmObject", result );
		return result;
	}

	result = Tspi_GetPolicyObject( hTPM, TSS_POLICY_USAGE, &hPolicy );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init: Tspi_GetPolicyObject", result );
		return result;
	}
	pwd_len=strlen(pwdo);
	TSS_sha1((unsigned char *)pwdo,pwd_len,opass);

	result = Tspi_Policy_SetSecret( hPolicy, TSS_SECRET_MODE_SHA1,
					sizeof(opass),opass);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init: Tspi_Policy_SetSecret", result );
		return result;
	}



	result = Tspi_Context_LoadKeyByUUID(hContext,
				TSS_PS_TYPE_SYSTEM, SRK_UUID, &hSRK);
	if (result != TSS_SUCCESS) {
		print_error("LocalInit:Tspi_Context_LoadKeyByUUID", result);
		return result;
	}
	result = Tspi_GetPolicyObject( hSRK, TSS_POLICY_USAGE,
					&hSrkPolicy );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_Init:Tspi_GetPolicyObject", result );
		return result;
	}
	pwd_len=strlen(pwds);

	pwd_len=strlen(pwds);
	TSS_sha1((unsigned char *)pwds,pwd_len,spass);

	result = Tspi_Policy_SetSecret( hSrkPolicy, TSS_SECRET_MODE_SHA1,
					sizeof(spass), spass);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Policy_SetSecret", result );
		return result;
	}


	return result;
}

void TESI_Local_Fin()
{
	Tspi_Context_FreeMemory( hContext, NULL );
	Tspi_Context_Close(hContext);
	return;
}

TSS_RESULT TESI_Local_GetPubKeyFromCA(TSS_HKEY * hCAKey,char * name)
{
	TSS_RESULT	result;
	TSS_FLAG	initFlags;
	TSS_HPOLICY	hUsagePolicy;
	TPM_PUBKEY      pubkey;
	UINT16 offset;
	UINT32 blob_size;
	BYTE *blob, pub_blob[1024];

	
	X509 *x=NULL;
	BIO *b;
	EVP_PKEY * CAPubKey;
	RSA * rsa;
   	char filename[256];             /* file name string of cert file */
	

	unsigned char	n[2048];//, p[2048];
	int		size_n;//, size_p;

        sprintf(filename,"%s.crt",name);

	// Load the CA Key
	
	b=BIO_new_file(filename,"r");
	x=PEM_read_bio_X509(b,NULL,NULL,NULL);

	BIO_free(b);
	CAPubKey=X509_get_pubkey(x);
	long version=X509_get_version(x);
	X509_free(x);
	rsa=EVP_PKEY_get1_RSA(CAPubKey);

		// get the pub CA key
	if ((size_n = BN_bn2bin(rsa->n, n)) <= 0) {
		fprintf(stderr, "BN_bn2bin failed\n");
		RSA_free(rsa);
                return 254;
        }

	result = Tspi_Context_CreateObject(hContext,
					   TSS_OBJECT_TYPE_RSAKEY,
					   TSS_KEY_TYPE_LEGACY|TSS_KEY_SIZE_2048,
					   hCAKey);
	if (result != TSS_SUCCESS) {
		print_error("Get PubKey from CA: Tspi_Context_CreateObject", result);
		return result;
	}
	
	/* Get the TSS_PUBKEY blob from the key object. */
	result = Tspi_GetAttribData(*hCAKey, TSS_TSPATTRIB_KEY_BLOB, TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY,
				    &blob_size, &blob);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_GetAttribData", result);
		return result;
	}

	offset = 0;
	result = TestSuite_UnloadBlob_PUBKEY(&offset, blob, &pubkey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_GetAttribData", result);
		return result;
	}

	Tspi_Context_FreeMemory(hContext, blob);
	/* Free the first dangling reference, putting 'n' in its place */

	free(pubkey.pubKey.key);
	pubkey.pubKey.keyLength = size_n;
	pubkey.pubKey.key = n;

	offset = 0;
	TestSuite_LoadBlob_PUBKEY(&offset, pub_blob, &pubkey);

	/* Free the second dangling reference */
	free(pubkey.algorithmParms.parms);

	/* set the public key data in the TSS object */
	result = Tspi_SetAttribData(*hCAKey, TSS_TSPATTRIB_KEY_BLOB, TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY,
				    (UINT32)offset, pub_blob);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_SetAttribData", result);
		return result;
	}

	return TSS_SUCCESS;
}

//#ifdef ADD_GETPUBEK 

TSS_RESULT TESI_Local_GetPubEK(char * name,char * pwdo)
{
	TSS_RESULT result;
	TSS_HKEY   hPubKey;

	BYTE opass[20];
	
	int pwd_len=strlen(pwdo);
	TSS_sha1((unsigned char *)pwdo,pwd_len,opass);

	result = Tspi_TPM_GetPubEndorsementKey(hTPM,TRUE,opass,&hPubKey);

	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_GetAttribData", result );
		return result;
	}
	
	return TESI_Local_WritePubKey(hPubKey,name);

}
//#endif
TSS_RESULT TESI_Local_GetPubEKWithUUID(char ** uuid,char * pwdo)
{
	TSS_RESULT result;
	TSS_HKEY   hPubKey;

	BYTE opass[20];
	BYTE digest[DIGEST_SIZE];
	char temp_file[DIGEST_SIZE*2+5];
	char filename[DIGEST_SIZE*2+5];
	
	char * ek_uuid;

	if(uuid==NULL)
		return -EINVAL; 
	*uuid=NULL;
		
	ek_uuid=malloc(DIGEST_SIZE*2);
	if(ek_uuid==NULL)
		return -ENOMEM;

	int pwd_len=strlen(pwdo);
	TSS_sha1((unsigned char *)pwdo,pwd_len,opass);

	result = Tspi_TPM_GetPubEndorsementKey(hTPM,TRUE,opass,&hPubKey);

	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_GetAttribData", result );
		free(ek_uuid);
		return result;
	}

	result = Tspi_TPM_GetRandom( hTPM, DIGEST_SIZE, &digest);
	int ret;
	ret=digest_to_uuid(digest,temp_file);
	result=TESI_Local_WritePubKey(hPubKey,temp_file);
	strcat(temp_file,".pem");

	ret=calculate_sm3(temp_file,(UINT32 *)digest);
	if(ret<0)
	{
		printf("calculate sm3 error!");
		return 0;
	}
	ret=digest_to_uuid(digest,ek_uuid);
	sprintf(filename,"%64s\.pem",ek_uuid);
	*uuid=ek_uuid;
	rename(temp_file,filename);
	return result;
}
//#endif

TSS_RESULT Local_CreateKeyWithFlags(TSS_HKEY * hKey,TSS_HKEY hWrapKey,TSS_FLAG initFlags,char * pwdw,char * pwdk)
{
	TSS_RESULT result;
	TSS_HKEY hWKey;
	TSS_HPOLICY WrapkeyUsagePolicy,keyUsagePolicy;
	int pwd_len;
	BYTE wpass[20];
	BYTE kpass[20];

	//Get WrapKey Policy Object
	
	if(hWrapKey==NULL)
		hWrapKey=hSRK;

	//Set Wrapkey Secret
	if(pwdw!=NULL)
	{
		result = Tspi_GetPolicyObject(hWrapKey, TSS_POLICY_USAGE, &WrapkeyUsagePolicy);
		if (result != TSS_SUCCESS) {
			print_error("Tspi_GetPolicyObject", result);
			return result;
		}
		pwd_len=strlen(pwdw);
		TSS_sha1((unsigned char *)pwdw,strlen(pwdw),wpass);
		if(hWrapKey==hSRK)
		{
			{
				result =   Tspi_Policy_SetSecret(WrapkeyUsagePolicy,
					TSS_SECRET_MODE_SHA1,
					sizeof(wpass),wpass);
				if (result != TSS_SUCCESS) {
					print_error("Tspi_Policy_SetSecret", result);
					return result;
				}
			}
		}
		else
		{
			result =  Tspi_Policy_SetSecret(keyUsagePolicy,
				TSS_SECRET_MODE_SHA1,
			  	sizeof(wpass),pwdw);

			if (result != TSS_SUCCESS) {
				print_error("Tspi_Policy_SetSecret", result);
				return result;
			}
		}
	}

	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
		initFlags, hKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		return result;
	}
	//Create Policy Object
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_POLICY, TSS_POLICY_USAGE,
		&keyUsagePolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		return result;
	}
	//Set Secret

	TSS_sha1((unsigned char *)pwdk,strlen(pwdk),kpass);
	result =  Tspi_Policy_SetSecret(keyUsagePolicy,
		  TSS_SECRET_MODE_SHA1,
		  sizeof(kpass),kpass);

	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		return result;
	}
	//Assign policy to key
	result = Tspi_Policy_AssignToObject(keyUsagePolicy, *hKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_AssignToObject", result);
		return result;
	}
	//Create Key
	result = Tspi_Key_CreateKey(*hKey, hWrapKey, 0);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_CreateKey", result);

		return result;
	} 

	return TSS_SUCCESS;
}
	
TSS_RESULT Mig_CreateKeyWithFlags(TSS_HKEY * hKey,TSS_HKEY hWrapKey,TSS_FLAG initFlags,char * pwdw,char * pwdk)
{
	TSS_RESULT result;
	TSS_HKEY hWKey;
	TSS_HPOLICY WrapkeyUsagePolicy,keyUsagePolicy;
	int pwd_len;
	BYTE wpass[20];
	BYTE kpass[20];

	//Get WrapKey Policy Object
	
	if(hWrapKey==NULL)
		hWrapKey=hSRK;
	result =
	    Tspi_GetPolicyObject(hWrapKey, TSS_POLICY_USAGE, &WrapkeyUsagePolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_GetPolicyObject", result);
		return result;
	}

	pwd_len=strlen(pwdw);
	//Set Secret
	if(hWrapKey==hSRK)
	{
		if(pwdw!=NULL)
			TSS_sha1((unsigned char *)pwdw,strlen(pwdw),wpass);
		else
			return TSS_E_TSP_AUTHFAIL;
		result =   Tspi_Policy_SetSecret(WrapkeyUsagePolicy,
			TSS_SECRET_MODE_SHA1,
			sizeof(wpass),wpass);
		if (result != TSS_SUCCESS) {
			print_error("Tspi_Policy_SetSecret", result);
			return result;
		}
	}else
	{
		TSS_sha1((unsigned char *)pwdw,strlen(pwdw),wpass);
		result =  Tspi_Policy_SetSecret(keyUsagePolicy,
			  TSS_SECRET_MODE_SHA1,
			  sizeof(wpass),pwdw);

		if (result != TSS_SUCCESS) {
			print_error("Tspi_Policy_SetSecret", result);
			return result;
		}
	}

	result =
	    Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
				      initFlags, hKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		return result;
	}
	//Create Policy Object
	result =
	    Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_POLICY, TSS_POLICY_MIGRATION,
				      &keyUsagePolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		return result;
	}
	//Set Secret
	
	

	TSS_sha1((unsigned char *)pwdk,strlen(pwdk),kpass);
	result =  Tspi_Policy_SetSecret(keyUsagePolicy,
		  TSS_SECRET_MODE_SHA1,
		  sizeof(kpass),kpass);

	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		return result;
	}
	//Assign policy to key
	result = Tspi_Policy_AssignToObject(keyUsagePolicy, *hKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_AssignToObject", result);
		return result;
	}
	//Create Key
	result = Tspi_Key_CreateKey(*hKey, hWrapKey, 0);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_CreateKey", result);

		return result;
	} 

	return TSS_SUCCESS;
}

TSS_RESULT TESI_Local_CreateSignKey(TSS_HKEY * hKey,TSS_HKEY hWrapKey,char * pwdw,char * pwdk)
{
	TSS_FLAG initFlags;

	initFlags = TSS_KEY_TYPE_SIGNING | TSS_KEY_SIZE_2048 |
	    TSS_KEY_VOLATILE | TSS_KEY_NO_AUTHORIZATION |
	    TSS_KEY_NOT_MIGRATABLE;
	return Local_CreateKeyWithFlags(hKey,hWrapKey,initFlags,pwdw,pwdk);
}
TSS_RESULT TESI_Local_CreateBindKey(TSS_HKEY * hKey,TSS_HKEY hWrapKey,char * pwdw,char * pwdk)
{
	TSS_FLAG initFlags;

	initFlags = TSS_KEY_TYPE_BIND | TSS_KEY_SIZE_2048  |
				TSS_KEY_VOLATILE | TSS_KEY_NO_AUTHORIZATION |
				TSS_KEY_NOT_MIGRATABLE;
	return Local_CreateKeyWithFlags(hKey,hWrapKey,initFlags,pwdw,pwdk);
}


TSS_RESULT TESI_Local_CreateStorageKey(TSS_HKEY * hKey,TSS_HKEY hWrapKey,char * pwdw,char * pwdk)
{
	TSS_FLAG initFlags;

	initFlags = TSS_KEY_TYPE_STORAGE | TSS_KEY_SIZE_2048 |
	   	 		TSS_KEY_VOLATILE | TSS_KEY_NO_AUTHORIZATION |
				TSS_KEY_NOT_MIGRATABLE;
	return Local_CreateKeyWithFlags(hKey,hWrapKey,initFlags,pwdw,pwdk);
}


TSS_RESULT TESI_Mig_CreateSignKey(TSS_HKEY * hKey,TSS_HKEY hWrapKey,char * pwdw,char * pwdk)
{
	TSS_FLAG initFlags;

	initFlags = TSS_KEY_TYPE_SIGNING | TSS_KEY_SIZE_2048 |
	    TSS_KEY_NO_AUTHORIZATION | TSS_KEY_MIGRATABLE;
	return Mig_CreateKeyWithFlags(hKey,hWrapKey,initFlags,pwdw,pwdk);
}


TSS_RESULT WriteKeyBlob(BYTE * keyblob, int blobLength,char * keyname)
{
   FILE *blbfile;                  /* output file for encrypted blob */
   char filename[256];             /* file name string of key file */
   TSS_RESULT result;

       sprintf(filename,"%s.key",keyname);
       blbfile = fopen(filename,"wb");
   if (blbfile == NULL)
      {
      printf("Unable to create key file %s.\n",filename);
      return TSS_E_KEY_NOT_LOADED;
      }
   result = fwrite(keyblob,1,blobLength,blbfile);
   if (result != blobLength)
      {
      printf("I/O Error writing key file\n");
      return TSS_E_KEY_NOT_LOADED;
      }
   fclose(blbfile);
   return TSS_SUCCESS;
}

TSS_RESULT TESI_Local_WriteKeyBlob(TSS_HKEY hKey,char * name)
{
	TSS_RESULT result;
	BYTE		*Blob;
	UINT32		blobLength;
		// get blob
	result = Tspi_GetAttribData( hKey, TSS_TSPATTRIB_KEY_BLOB,
					TSS_TSPATTRIB_KEYBLOB_BLOB,
					&blobLength, &Blob );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_GetAttribData", result );
		return result;
	}
	
  	 result = WriteKeyBlob(Blob,blobLength,name);


	return result;
}

TSS_RESULT ReadKeyBlob(BYTE ** Blob, int *blobLength,char * keyname)
{
   FILE *blbfile;                  /* output file for encrypted blob */
   char filename[256];             /* file name string of key file */
   TSS_RESULT result;
   struct stat sbuf;

   sprintf(filename,"%s.key",keyname);
       
   blbfile = fopen(filename,"rb");
   if (blbfile == NULL)
   {
      printf("Unable to read key file %s.\n",filename);
      return TSS_E_KEY_NOT_LOADED;
   }
    stat(filename,&sbuf);
    *blobLength = (int)sbuf.st_size;
    *Blob=(BYTE *)malloc(*blobLength);
    if(*Blob==NULL)
    {
	    printf("Sign File: alloc memory error!\n");
    }
    result = fread(*Blob,1,*blobLength,blbfile);
    if (result != (int)(*blobLength))
	{
	    printf("Unable to read key file\n");
	    return TSS_E_KEY_NOT_LOADED;
	}
    fclose(blbfile);
    return TSS_SUCCESS;
}

TSS_RESULT TESI_Local_ReadKeyBlob(TSS_HKEY * hKey,char * name)
{
	TSS_RESULT result;
	BYTE		*Blob;
	UINT32		blobLength;

	result  = ReadKeyBlob(&Blob,&blobLength,name);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Read Key Blob error %d!", result );
		return result;
	}
	
		//Create Object
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
					TSS_KEY_SIZE_2048, hKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		return result;
	}


		// set blob
	result = Tspi_SetAttribData( *hKey, TSS_TSPATTRIB_KEY_BLOB,
					TSS_TSPATTRIB_KEYBLOB_BLOB,
					blobLength, Blob );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_SetAttribData", result );
		return result;
	}
	return TSS_SUCCESS;
}

/****************************************************************************/
/*                                                                          */
/* Convert a TPM public key to an OpenSSL RSA public key                    */
/*                                                                          */
/****************************************************************************/
RSA *TSS_convpubkey(TPM_KEY *k)
   {
   RSA  *rsa;
   BIGNUM *mod;
   BIGNUM *exp;
   
   /* create the necessary structures */
   rsa = RSA_new();
   mod = BN_new();
   exp = BN_new();
   if (rsa == NULL || mod == NULL || exp == NULL) {
      if (rsa) {
         RSA_free(rsa);
      }
      if (mod) {
         BN_free(mod);
      }
      if (exp) {
         BN_free(exp);
      }
      return NULL;
   }
   /* convert the raw public key values to BIGNUMS */
   BN_bin2bn(k->pubKey.key,k->pubKey.keyLength,mod);

   TPM_RSA_KEY_PARMS * rsaKeyParms;
   rsaKeyParms=(TPM_RSA_KEY_PARMS *)k->algorithmParms.parms;
   if (0 == rsaKeyParms->exponentSize) {
      unsigned char exponent[3] = {0x1,0x0,0x1};
      BN_bin2bn(exponent,3,exp);
   } else {
      BN_bin2bn(rsaKeyParms->exponent,rsaKeyParms->exponentSize,
                exp);
   }
   /* set up the RSA public key structure */
   rsa->n = mod;
   rsa->e = exp;
   return rsa;
 }

TSS_RESULT TSS_setpubkey(TSS_HKEY * hPubKey,UINT32 size_n, BYTE *n)
   {

	TSS_RESULT result;
	UINT16 offset;
	UINT32 blob_size;
	BYTE *blob,pub_blob[1024];
	TCPA_PUBKEY pubkey;


	result = Tspi_Context_CreateObject(hContext,
					   TSS_OBJECT_TYPE_RSAKEY,
					   TSS_KEY_TYPE_LEGACY|TSS_KEY_SIZE_2048,
					   hPubKey);
	if (result != TSS_SUCCESS) {
		print_error("Get PubKey : Tspi_Context_CreateObject", result);
		return result;
	}
	
	/* Get the TSS_PUBKEY blob from the key object. */
	result = Tspi_GetAttribData(*hPubKey, TSS_TSPATTRIB_KEY_BLOB, TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY,
				    &blob_size, &blob);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_GetAttribData", result);
		return result;
	}

	offset = 0;
	result = TestSuite_UnloadBlob_PUBKEY(&offset, blob, &pubkey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_GetAttribData", result);
		return result;
	}

	Tspi_Context_FreeMemory(hContext, blob);
	/* Free the first dangling reference, putting 'n' in its place */

	free(pubkey.pubKey.key);
	pubkey.pubKey.keyLength = size_n;
	pubkey.pubKey.key = n;

	offset = 0;
	TestSuite_LoadBlob_PUBKEY(&offset, pub_blob, &pubkey);

	/* Free the second dangling reference */
	free(pubkey.algorithmParms.parms);

	/* set the public key data in the TSS object */
	result = Tspi_SetAttribData(*hPubKey, TSS_TSPATTRIB_KEY_BLOB, TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY,
				    (UINT32)offset, pub_blob);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_SetAttribData", result);
		return result;
	}

	return TSS_SUCCESS;
 }

TSS_RESULT ReadPubKey(void ** rsa_data,char * keyname)
{
   FILE *keyfile;                  /* output file for public key */
   EVP_PKEY *pkey = NULL;          /* OpenSSL public key */
   char filename[256];             /* file name string of key file */
   TSS_RESULT result;
   RSA * tempRSA;
   RSA ** rsa=(RSA **)rsa_data;	

   tempRSA = RSA_new();

   if (rsa == NULL)
   {
      printf("Error from ReadPubKey\n");
      return TSS_E_KEY_NOT_LOADED;
   }
//   pkey = EVP_PKEY_new();
//   if (pkey == NULL) {
//         printf("Unable to create EVP_PKEY\n");
//         return TSS_E_KEY_NOT_LOADED;
//   }
   sprintf(filename,"%s.pem",keyname);
   keyfile = fopen(filename,"rb");
   if (keyfile == NULL)
      {
      printf("Unable to create public key file\n");
      return TSS_E_KEY_NOT_LOADED;
      }
   pkey = PEM_read_PUBKEY(keyfile,NULL,NULL,NULL);
   if (pkey == NULL)
      {
      printf("I/O Error read public key file\n");
      return TSS_E_KEY_NOT_LOADED;
      }
   fclose(keyfile);
   tempRSA = EVP_PKEY_get1_RSA(pkey);
   if (tempRSA == NULL) {
       printf("Unable to get pubkey from EVP_PKEY\n");
       return TSS_E_KEY_NOT_LOADED;
   }
   EVP_PKEY_free(pkey);
   *rsa=tempRSA;
   return TSS_SUCCESS;
}

TSS_RESULT WritePubKey(void * rsa_data,char * keyname)
{
   FILE *keyfile;                  /* output file for public key */
   EVP_PKEY *pkey = NULL;          /* OpenSSL public key */
   char filename[256];             /* file name string of key file */
   TSS_RESULT result;
   RSA * rsa=(RSA *)rsa_data;	

   if (rsa == NULL)
      {
      printf("Error from TSS_convpubkey\n");
      return TSS_E_KEY_NOT_LOADED;
      }
   pkey = EVP_PKEY_new();
   if (pkey == NULL) {
       printf("Unable to create EVP_PKEY\n");
      return TSS_E_KEY_NOT_LOADED;
   }
   result = EVP_PKEY_assign_RSA(pkey,rsa);
   if (result == 0) {
       printf("Unable to assign public key to EVP_PKEY\n");
      return TSS_E_KEY_NOT_LOADED;
   }
   sprintf(filename,"%s.pem",keyname);
   keyfile = fopen(filename,"wb");
   if (keyfile == NULL)
      {
      printf("Unable to create public key file\n");
      return TSS_E_KEY_NOT_LOADED;
      }
   result = PEM_write_PUBKEY(keyfile,pkey);
   if (result == 0)
      {
      printf("I/O Error writing public key file\n");
      return TSS_E_KEY_NOT_LOADED;
      }
   fclose(keyfile);
   EVP_PKEY_free(pkey);
      return TSS_SUCCESS;

}

TSS_RESULT ReadPrivKey(void ** rsa_data,char * keyname,char * pass_phrase)
{
   X509 *x=NULL;
   BIO *bio;
   ASN1_BIT_STRING * CAString;
   EVP_PKEY * CAKey;
   unsigned char n[2048];
   int size_n;
   char filename[256];             /* file name string of key file */
   char errstring[256];             /* file name string of key file */
   TSS_RESULT result;
   RSA ** rsa=(RSA **)rsa_data;	

   RSA * tempRSA;
   tempRSA = RSA_new();

   if (rsa == NULL)
   {
      printf("Error from ReadPubKey\n");
      return TSS_E_KEY_NOT_LOADED;
   }
   sprintf(filename,"%s.key",keyname);
   bio=BIO_new_file(filename,"rb");
   if (bio == NULL)
   {
      printf("Unable to create public key file\n");
      return TSS_E_KEY_NOT_LOADED;
   }

//   *rsa=PEM_read_bio_RSAPrivateKey(bio,&tempRSA,NULL,pass_phrase);
   *rsa=PEM_read_bio_RSAPrivateKey(bio,NULL,NULL,pass_phrase);
   if (*rsa == NULL)
   {
        printf("I/O Error read private key file\n");
	ERR_error_string(ERR_get_error(),errstring);
        printf("openssl error:%s\n",errstring);
        return TSS_E_KEY_NOT_LOADED;
   }
   BIO_free(bio);
//   *rsa=tempRSA;
   return TSS_SUCCESS;
}

TSS_RESULT WritePrivKey(void * rsa_data,char * keyname,char * pass_phrase)
{
   X509 *x=NULL;
   BIO *bio;
   ASN1_BIT_STRING * CAString;
   EVP_PKEY * CAKey;
   unsigned char n[2048];
   int size_n;
   char filename[256];             /* file name string of key file */
   TSS_RESULT result;
   int ret;
   RSA * rsa=(RSA *)rsa_data;	

   RSA * tempRSA;
   tempRSA = RSA_new();

   if (rsa == NULL)
   {
      printf("Error from WritePrivKey\n");
      return TSS_E_KEY_NOT_LOADED;
   }
   sprintf(filename,"%s.key",keyname);
   bio=BIO_new_file(filename,"wb");
   if (bio == NULL)
   {
      printf("Unable to create private key file\n");
      return TSS_E_KEY_NOT_LOADED;
   }

   if(pass_phrase!=NULL)	
   	ret = PEM_write_bio_RSAPrivateKey(bio,rsa,EVP_aes_128_cbc(),pass_phrase,strlen(pass_phrase),NULL,NULL);
   else
   	ret = PEM_write_bio_RSAPrivateKey(bio,rsa,EVP_aes_128_cbc(),NULL,0,NULL,NULL);
   if (ret < 0)
   {
        printf("I/O Error write private key file\n");
        return TSS_E_KEY_NOT_LOADED;
   }
   BIO_free(bio);
   return TSS_SUCCESS;
}


TSS_RESULT TESI_Local_WritePubKey(TSS_HKEY hPubKey,char * name)
{
	TSS_RESULT result;
	RSA *rsa;                       /* OpenSSL format Public Key */
	TPM_KEY newkey;
	UINT16 offset=0;
	BYTE		*Blob;
	UINT32		blobLength;
		// get blob
	result = Tspi_GetAttribData( hPubKey, TSS_TSPATTRIB_KEY_BLOB,
					TSS_TSPATTRIB_KEYBLOB_BLOB,
					&blobLength, &Blob );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_GetAttribData", result );
		return result;
	}
	
   	OpenSSL_add_all_algorithms();

   	result=TestSuite_UnloadBlob_KEY(&offset,Blob,&newkey);
  
  
   /*
   ** convert the returned public key to OpenSSL format and
   ** export it to a file
   */
	rsa = TSS_convpubkey(&newkey);
   	result = WritePubKey(rsa,name);

	return TSS_SUCCESS;
}

TSS_RESULT TESI_Local_ReadPubKey(TSS_HKEY * hPubKey,char * name)
{

	RSA * rsa;
	TSS_RESULT result;
	unsigned char	n[2048];//, p[2048];
	int		size_n;//, size_p;

	result=ReadPubKey(&rsa,name);
	if ( result != TSS_SUCCESS )
	{
		print_error( "ReadPubKey", result );
		return result;
	}

		// get the pub CA key
	if ((size_n = BN_bn2bin(rsa->n, n)) <= 0) {
		fprintf(stderr, "BN_bn2bin failed\n");
		RSA_free(rsa);
                return TSS_E_KEY_NOT_LOADED;
        }

	result=TSS_setpubkey(hPubKey,size_n,n);
	if ( result != TSS_SUCCESS )
	{
		print_error( "TSS_setpubkey", result );
		return result;
	}
	return TSS_SUCCESS;
}

TSS_RESULT TESI_Local_SignFile(char * filename,TSS_HKEY hSignKey,char * signname,char * pwdk)
{
   FILE *file;                  /* output file for encrypted blob */
   FILE *sigfile;                  /* output file for encrypted blob */
   char sigfilename[256];             /* file name string of key file */
   TSS_RESULT result;
   struct stat sbuf;
   BYTE * blob;
   UINT32 blobLength;
  TSS_HHASH hHash;
  TSS_HPOLICY hPolicy;
	BYTE  * sigdata;
	UINT32 siglen;
   BYTE kpass[20];
   BYTE HashValue[20];
       
   // read file text
   file = fopen(filename,"rb");
   if (file == NULL)
   {
      printf("Unable to read key file %s.\n",filename);
      return TSS_E_KEY_NOT_LOADED;
   }
    stat(filename,&sbuf);
    blobLength = (int)sbuf.st_size;
    blob=(BYTE *)malloc(blobLength);
    if(blob==NULL)
    {
	    printf("Sign File: alloc memory error!\n");
    }
    result = fread(blob,1,blobLength,file);
    if (result != (int)blobLength)
	{
	    printf("Unable to read sign text file\n");
	    return TSS_E_KEY_NOT_LOADED;
	}
    fclose(file);
       	

    TSS_sha1((unsigned char *)pwdk,strlen(pwdk),kpass);

		// Get The SRK policy 
	result = Tspi_GetPolicyObject(hSignKey,TSS_POLICY_USAGE,&hPolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_GetPolicyObject ", result);
		return result;
	}

	
		// Set The SRK Secret 
	result = Tspi_Policy_SetSecret(hPolicy,TSS_SECRET_MODE_SHA1,
			sizeof(kpass),kpass);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		return result;
	}

        result=Tspi_Key_LoadKey(hSignKey,hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_LoadKey", result);
		return result;
	}
	// create hash
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_HASH,
					   TSS_HASH_OTHER, &hHash);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject (hash)", result);
		return result;
	}

        TSS_sha1(blob,blobLength,HashValue);
	result = Tspi_Hash_SetHashValue(hHash,sizeof(HashValue),HashValue);

	if (result != TSS_SUCCESS) {
		print_error("Tspi_Hash_SetHashValue", result);
		return result;
	}

	result = Tspi_Hash_Sign(hHash, hSignKey, &siglen,&sigdata);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Hash_Sign", result);
		return result;
	}
	free(blob);

	//open sig file
        sprintf(sigfilename,"%s.sig",signname);

	// read file text
	sigfile = fopen(sigfilename,"wb");
	if (sigfile == NULL)
	{
		printf("Unable to write signature file %s.\n",sigfilename);
		return TSS_E_KEY_NOT_LOADED;
   	}
	result=fwrite(sigdata,1,siglen,sigfile);
	if(result !=(int)siglen)
	{
	    printf("Unable to write sign text file\n");
	    return TSS_E_KEY_NOT_LOADED;
	}
	fclose(sigfile);
	return TSS_SUCCESS;
}

TSS_RESULT TESI_Local_VerifySignData(TESI_SIGN_DATA * signdata,TSS_HKEY hVerifyKey)
{

   FILE *file;                  /* output file for encrypted blob */
   FILE *sigfile;                  /* output file for encrypted blob */
   char sigfilename[256];             /* file name string of key file */
   TSS_RESULT result;
   struct stat sbuf;
   BYTE * blob;
   UINT32 blobLength;
  TSS_HHASH hHash;
  TSS_HPOLICY hPolicy;
	BYTE  * sigdata;
	UINT32 siglen;
   BYTE kpass[20];
   BYTE HashValue[20];
       
	// create hash
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_HASH,
					   TSS_HASH_OTHER, &hHash);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject (hash)", result);
		return result;
	}

        TSS_sha1(signdata->data,signdata->datalen,HashValue);
	result = Tspi_Hash_SetHashValue(hHash,sizeof(HashValue),HashValue);

	if (result != TSS_SUCCESS) {
		print_error("Tspi_Hash_SetHashValue", result);
		return result;
	}

	result = Tspi_Hash_VerifySignature(hHash, hVerifyKey, signdata->signlen,signdata->sign);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Hash_VerifySignature", result);
		return result;
	}
	return TSS_SUCCESS;
}
TSS_RESULT TESI_Local_VerifyFile(char * filename,TSS_HKEY hVerifyKey,char * signname)
{

   FILE *file;                  /* output file for encrypted blob */
   FILE *sigfile;                  /* output file for encrypted blob */
   char sigfilename[256];             /* file name string of key file */
   TSS_RESULT result;
   struct stat sbuf;
   BYTE * blob;
   UINT32 blobLength;
  TSS_HHASH hHash;
  TSS_HPOLICY hPolicy;
	BYTE  * sigdata;
	UINT32 siglen;
   BYTE kpass[20];
   BYTE HashValue[20];
       
   // read file text
   file = fopen(filename,"rb");
   if (file == NULL)
   {
      printf("Unable to read key file %s.\n",filename);
      return TSS_E_KEY_NOT_LOADED;
   }
    stat(filename,&sbuf);
    blobLength = (int)sbuf.st_size;
    blob=(BYTE *)malloc(blobLength);
    if(blob==NULL)
    {
	    printf("Sign File: alloc memory error!\n");
    }
    result = fread(blob,1,blobLength,file);
    if (result != (int)blobLength)
	{
	    printf("Unable to read sign text file\n");
	    return TSS_E_KEY_NOT_LOADED;
	}
    fclose(file);
       	

        /*result=Tspi_Key_LoadKey(hSignKey,hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_LoadKey", result);
		return result;
	}*/
	// create hash
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_HASH,
					   TSS_HASH_OTHER, &hHash);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject (hash)", result);
		return result;
	}

        TSS_sha1(blob,blobLength,HashValue);
	result = Tspi_Hash_SetHashValue(hHash,sizeof(HashValue),HashValue);

	if (result != TSS_SUCCESS) {
		print_error("Tspi_Hash_SetHashValue", result);
		return result;
	}
	free(blob);

	//open sig file
        sprintf(sigfilename,"%s.sig",signname);

	// read file text
	sigfile = fopen(sigfilename,"rb");
	if (sigfile == NULL)
	{
		printf("Unable to read signature file %s.\n",sigfilename);
		return TSS_E_KEY_NOT_LOADED;
   	}
	stat(sigfilename,&sbuf);
	siglen = (int)sbuf.st_size;
	sigdata=(BYTE *)malloc(siglen);

	result=fread(sigdata,1,siglen,sigfile);
	if (result != (int)siglen)
	{
		printf("Unable to read sign file\n");
		return TSS_E_KEY_NOT_LOADED;
	}
	fclose(sigfile);

	result = Tspi_Hash_VerifySignature(hHash, hVerifyKey, siglen,sigdata);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Hash_VerifySignature", result);
		return result;
	}
	//free(sigdata);

	return TSS_SUCCESS;
}

TSS_RESULT TESI_Local_ClearOwner(char * pwdo)
{
	TSS_RESULT result;
	int pwd_len;
	BYTE opass[20];
	TSS_HPOLICY hPolicy;
		// Clear owner with owner auth (TRUE means PhyPres, FALSE means owner auth)
	
	result = Tspi_GetPolicyObject( hTPM, TSS_POLICY_USAGE, &hPolicy );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_ClearOwner: Tspi_GetPolicyObject", result );
		return result;
	}

	pwd_len=strlen(pwdo);
	TSS_sha1((unsigned char *)pwdo,pwd_len,opass);

	result = Tspi_Policy_SetSecret( hPolicy, TSS_SECRET_MODE_SHA1,
					sizeof(opass),opass);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_ClearOwner: Tspi_Policy_SetSecret", result );
		return result;
	}


	result = Tspi_TPM_ClearOwner( hTPM, TRUE );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Local_ClearOwner: Tspi_TPM_ClearOwner", result );
		return result;
	}
	Tspi_Context_FreeMemory( hContext, NULL );
	Tspi_Context_Close( hContext );
	return TSS_SUCCESS;
}


TSS_RESULT TESI_Local_CreatePcr(TSS_HPCRS * hPcr)
{
	TSS_RESULT result;
	result = Tspi_Context_CreateObject(hContext,
			TSS_OBJECT_TYPE_PCRS, 0,
			hPcr);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		return result;
	}
		//SelectPcrIndex
	return TSS_SUCCESS;

}
TSS_RESULT TESI_Local_CreatePcr_short(TSS_HPCRS * hPcr)
{
        TSS_RESULT result;
        result = Tspi_Context_CreateObject(hContext,
                        TSS_OBJECT_TYPE_PCRS,
			TSS_PCRS_STRUCT_INFO_SHORT,
                        hPcr);
        if (result != TSS_SUCCESS) {
                print_error("Tspi_Context_CreateObject", result);
                return result;
        }
                //SelectPcrIndex
        return TSS_SUCCESS;

}

TSS_RESULT TESI_Local_SelectPcr(TSS_HPCRS hPcr,UINT32 Index)
{

	TSS_RESULT result;
	result = Tspi_PcrComposite_SelectPcrIndex(hPcr, Index);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_PcrComposite_SelectPcrIndex", result);
		return result;
	}
	return TSS_SUCCESS;

}
TSS_RESULT TESI_Local_SelectPcrEx(TSS_HPCRS hPcr,UINT32 Index)
{

        TSS_RESULT result;
        //result = Tspi_PcrComposite_SelectPcrIndexEx(hPcr, Index,TSS_PCRS_DIRECTION_RELEASE);
	result = Tspi_PcrComposite_SelectPcrIndexEx(hPcr, Index,2);
        if (result != TSS_SUCCESS) {
                print_error("Tspi_PcrComposite_SelectPcrIndexEx", result);
                return result;
        }
        return TSS_SUCCESS;

}

TSS_RESULT TESI_Local_PcrExtend(UINT32 index,BYTE * Data) 
{
	BYTE * pRgbValue;
	int Rgblen;
	TSS_RESULT result;
	
	result=Tspi_TPM_PcrExtend(hTPM,index,PCR_SIZE,Data,NULL,&Rgblen,&pRgbValue);
        if (result != TSS_SUCCESS) {
                print_error("TESI_Local_PcrExtend", result);
                return result;
        }
	
	return result;
}


TSS_RESULT TESI_Local_PcrRead(UINT32 index,BYTE * Data) 
{
	int Rgblen;
	TSS_RESULT result;
	BYTE * pRgbValue;
	result=Tspi_TPM_PcrRead(hTPM,index,&Rgblen,&pRgbValue);
        if (result != TSS_SUCCESS) {
                print_error("TESI_Local_PcrRead", result);
                return result;
        }
	if(Rgblen!=PCR_SIZE)
		return -EINVAL;	
	Memcpy(Data,pRgbValue,PCR_SIZE);
	return result;
}

TSS_RESULT TESI_Local_SetPcrLocality(TSS_HPCRS hPcr)
{
	TSS_RESULT result;

	result=Tspi_PcrComposite_SetPcrLocality(hPcr,TPM_LOC_ONE);
        if (result != TSS_SUCCESS) {
                print_error("Tspi_PcrComposite_SetPcrLocality", result);
                return result;
        }
        return TSS_SUCCESS;
}

TSS_RESULT TESI_Local_Quote(TSS_HKEY hIdentKey,TSS_HPCRS hPcr,char *name)
{
	TSS_RESULT result;
	BYTE*		data;
	TSS_VALIDATION valData;

	/* Set the Validation Data */
	result = Tspi_TPM_GetRandom( hTPM, 20, &data );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_TPM_GetRandom", result );
		return result;
	}

	valData.ulExternalDataLength = 20;
	valData.rgbExternalData = data;

		//Call TPM Quote
	result = Tspi_TPM_Quote(hTPM, hIdentKey, hPcr, &valData);

	if (result != TSS_SUCCESS) {
		print_error("Tspi_TPM_Quote", result);
		return result;
	}
	result=WriteValidation(&valData,name);
	return TSS_SUCCESS;
}

TSS_RESULT TESI_Local_Quote2(TSS_HKEY hIdentKey,TSS_HPCRS hPcr,char *name)
{
        TSS_RESULT result;
        BYTE*           data;
        TSS_VALIDATION valData;
	UINT32 versionInfoSize;
	BYTE *versionInfo;
	TSS_BOOL fAddversion;
	fAddversion=1;

        /* Set the Validation Data */
        result = Tspi_TPM_GetRandom( hTPM, 20, &data );
        if ( result != TSS_SUCCESS )
        {
                print_error( "Tspi_TPM_GetRandom", result );
                return result;
        }

        valData.ulExternalDataLength = 20;
        valData.rgbExternalData = data;

                //Call TPM Quote
        result = Tspi_TPM_Quote2(hTPM, hIdentKey,fAddversion, hPcr, &valData,&versionInfoSize,&versionInfo);

        if (result != TSS_SUCCESS) {
                print_error("Tspi_TPM_Quote2", result);
                return result;
        }
        result=WriteValidation(&valData,name);
        return TSS_SUCCESS;
}
TSS_RESULT TESI_Local_VerifyValData(TSS_HKEY hIdentKey,char * name)
{
	TSS_RESULT	result;
	TSS_HHASH	hHash;
	TSS_VALIDATION  valData;

	result=ReadValidation(&valData,name);

	// create hash
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_HASH, TSS_HASH_SHA1, &hHash);
	if (result != TSS_SUCCESS) {
		print_error("Testsuite_Verify_Signature : Tspi_Context_CreateObject (hash)",
			    result);
		return result;
	}

	result = Tspi_Hash_UpdateHashValue(hHash, valData.ulDataLength, valData.rgbData);
	if (result != TSS_SUCCESS) {
		print_error("Testsuite_Verify_Signature : Tspi_Hash_SetHashValue", result);
		Tspi_Context_CloseObject(hContext, hHash);
		return result;
	}

	result = Tspi_Hash_VerifySignature(hHash, hIdentKey, valData.ulValidationDataLength,
					   valData.rgbValidationData);
	if (result != TSS_SUCCESS) {
		print_error("Testsuite_Verify_Signature : Tspi_Hash_Verify_Signature", result);
		Tspi_Context_CloseObject(hContext, hHash);
		return result;
	}

	Tspi_Context_CloseObject(hContext, hHash);
	return result;
}


TSS_RESULT TESI_Mig_CreateRewrapTicket(TSS_HKEY hMigKey,char * ticketname,char * pwdo)
{

	FILE *ticketfile;                  /* output file for encrypted blob */
	char filename[256];             /* file name string of key file */
	TSS_RESULT result;
	BYTE * blob;
	UINT32 blobLength;
       	TSS_HPOLICY hPolicy;
	int pwd_len;
	char opass[20];

	result = Tspi_GetPolicyObject( hTPM, TSS_POLICY_USAGE, &hPolicy );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Create Rewrap Ticket: Tspi_GetPolicyObject", result );
		return result;
	}

	pwd_len=strlen(pwdo);
	TSS_sha1((unsigned char *)pwdo,pwd_len,opass);

	result = Tspi_Policy_SetSecret( hPolicy, TSS_SECRET_MODE_SHA1,
					sizeof(opass),opass);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Create Rewrap Ticket: Tspi_Policy_SetSecret", result );
		return result;
	}

	//Authorize Migration Ticket
	result =    Tspi_TPM_AuthorizeMigrationTicket(hTPM, hMigKey,
		TSS_MS_REWRAP, &blobLength, &blob);
	if (result != TSS_SUCCESS) {
		print_error("Tpsi_TPM_AuthorizeMigrationTicket ", result);
		return result;
	}

	 //open sig file
	sprintf(filename,"%s.tic",ticketname);
	// read file text
	ticketfile = fopen(filename,"wb");
	if (ticketfile == NULL)
	{
		printf("Unable to open ticket file file %s.\n",filename);
    		return TSS_E_KEY_NOT_LOADED;
  	}

	result=fwrite(blob,1,blobLength,ticketfile);
	if (result != (int)blobLength)
	{
		printf("Unable to write ticket file\n");
		return TSS_E_KEY_NOT_LOADED;
	}
	fclose(ticketfile);

	return TSS_SUCCESS;
}

TSS_RESULT TESI_Mig_CreateTicket(TSS_HKEY hMigKey,char * ticketname,char * pwdo)
{

	FILE *ticketfile;                  /* output file for encrypted blob */
	char filename[256];             /* file name string of key file */
	TSS_RESULT result;
	BYTE * blob;
	UINT32 blobLength;
       	TSS_HPOLICY hPolicy;
	int pwd_len;
	char opass[20];

	result = Tspi_GetPolicyObject( hTPM, TSS_POLICY_USAGE, &hPolicy );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Create Rewrap Ticket: Tspi_GetPolicyObject", result );
		return result;
	}

	pwd_len=strlen(pwdo);
	TSS_sha1((unsigned char *)pwdo,pwd_len,opass);

	result = Tspi_Policy_SetSecret( hPolicy, TSS_SECRET_MODE_SHA1,
					sizeof(opass),opass);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Create Rewrap Ticket: Tspi_Policy_SetSecret", result );
		return result;
	}

	//Authorize Migration Ticket
//	result =    Tspi_TPM_AuthorizeMigrationTicket(hTPM, hMigKey,
//		TSS_MS_REWRAP, &blobLength, &blob);
	//Authorize Migration Ticket
	result =    Tspi_TPM_AuthorizeMigrationTicket(hTPM, hMigKey,
		TSS_MS_MIGRATE, &blobLength, &blob);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_TPM_AuthorizeMigrationTicket ", result);
		return result;
	}

	 //open sig file
	sprintf(filename,"%s.tic",ticketname);
	// read file text
	ticketfile = fopen(filename,"wb");
	if (ticketfile == NULL)
	{
		printf("Unable to open ticket file file %s.\n",filename);
    		return TSS_E_KEY_NOT_LOADED;
  	}

	result=fwrite(blob,1,blobLength,ticketfile);
	if (result != (int)blobLength)
	{
		printf("Unable to write ticket file\n");
		return TSS_E_KEY_NOT_LOADED;
	}
	fclose(ticketfile);

	return TSS_SUCCESS;
}

TSS_RESULT TESI_Mig_CreateKeyBlob(TSS_HKEY hKey,TSS_HKEY hParentKey,char * pwdk,char * ticket,char * MigBlob)
{
   FILE *ticketfile;                  /* output file for encrypted blob */
   FILE *blbfile;                  /* output file for encrypted blob */
   char filename[256];             /* file name string of key file */
   TSS_RESULT result;
   struct stat sbuf;
	BYTE * blob;
 	UINT32 blobLength;
	BYTE *randomData;
	UINT32 randomLength;
   BYTE * ticketData;
   UINT32 ticketLength;
   TSS_HPOLICY hPolicy;
   BYTE kpass[20];
   BYTE ppass[20];
       

   sprintf(filename,"%s.tic",ticket);
   // read file text
   ticketfile = fopen(filename,"rb");
   if (ticketfile == NULL)
   {
      printf("Unable to read key file %s.\n",filename);
      return TSS_E_KEY_NOT_LOADED;
   }
    stat(filename,&sbuf);
    ticketLength = (int)sbuf.st_size;
    ticketData=(BYTE *)malloc(ticketLength);
    if(ticketData==NULL)
    {
	    printf("Create Mig Key blob: alloc memory error!\n");
    }
	result = fread(ticketData,1,ticketLength,ticketfile);
  	if (result != (int)ticketLength)
	{
	    printf("Unable to read ticket file\n");
	    return TSS_E_KEY_NOT_LOADED;
	}
 	fclose(ticketfile);
       	
   // load hKey 

	result=TESI_Local_LoadKey(hKey,hParentKey,kpass);
	if (result != TSS_SUCCESS) {
		print_error("TESI_Local_LoadKey ", result);
		return result;
	}
	
	if(hParentKey==(TSS_HKEY)NULL)
		hParentKey=hSRK;
	//Create Migration Blob
	result = Tspi_Key_CreateMigrationBlob(hKey, hParentKey,
					      ticketLength, ticketData,
					      &randomLength, &randomData,
					      &blobLength, &blob);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_CreateMigrationBlob", result);
		return result;
	}

	sprintf(filename,"%s.key",MigBlob);
 	// read file text
	blbfile = fopen(filename,"wb");
  	if (blbfile == NULL)
   	{
   	   printf("Unable to write blb file %s.\n",filename);
   	   return TSS_E_KEY_NOT_LOADED;
  	 }
   	result = fwrite(blob,1,blobLength,blbfile);
  	 if (result != blobLength)
     	 {
	      printf("I/O Error writing blob file\n");
  	    return TSS_E_KEY_NOT_LOADED;
      	 }

	fclose(blbfile);
 	return TSS_SUCCESS;

}

TSS_RESULT TESI_Mig_CreateMigBlob(TSS_HKEY hKey,TSS_HKEY hParentKey,char * pwdk,char * random,char * MigBlob);
TSS_RESULT TESI_Mig_ConvertMigBlob(TSS_HKEY * hKey,TSS_HKEY hParentKey,char *MigBlob)
{
	TSS_RESULT result;
	BYTE		*Blob;
	UINT32		blobLength;

	result  = ReadKeyBlob(&Blob,&blobLength,MigBlob);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Read Key Blob error %d!", result );
		return result;
	}
	
		//Create Object
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
					TSS_KEY_SIZE_2048, hKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		return result;
	}


		// set blob
	result = Tspi_SetAttribData( *hKey, TSS_TSPATTRIB_KEY_BLOB,
					TSS_TSPATTRIB_KEYBLOB_BLOB,
					blobLength, Blob );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_SetAttribData", result );
		return result;
	}
	return TSS_SUCCESS;

}

TSS_RESULT TESI_Local_EnableTpm(char * path,int port)
{
    char *newargv[10];
    char *newenv[10];
    char argbuf[10][128];
    char envbuf[10][128];

    memset(newargv,0,sizeof(char *)*10);
    memset(newenv,0,sizeof(char *)*10);
    sprintf(envbuf[0],"TPM_SERVER_PORT=%d",port);
    sprintf(envbuf[1],"TPM_SERVER_NAME=127.0.0.1");
    sprintf(envbuf[2],"TPM_PORT=%d",port);
    sprintf(envbuf[3],"TPM_PATH=%s/tpmdata/%d",path,port);
    newenv[0]=envbuf[0];
    newenv[1]=envbuf[1];
    newenv[2]=envbuf[2];
    newenv[3]=envbuf[3];

    char cmd[128];
    char buf[128];

    int server_pid;

    server_pid=fork();

    if(server_pid==0)
    {
        //child process run tpm_server
        sprintf(cmd,"%s/libtpm/utils/physicalenable",path);
        execve(cmd,newargv,newenv);
    }

    return TSS_SUCCESS;

}

/* substitute this for TPM_IDENTITY_CREDENTIAL in the TPM docs */
struct trousers_ca_tpm_identity_credential
{
	/* This should be set up according to the TPM 1.1b spec section
	 * 9.5.5.  For now, just use it to verify we got the right stuff
	 * back from the activate identity call. */
	X509 cert;
} ca_cred;
#define CERT_VERIFY_BYTE 0x5a

/* helper struct for passing stuff from function to function */
struct ca_blobs
{
	UINT32 asymBlobSize, symBlobSize;
	BYTE *asymBlob;
	BYTE *symBlob;
} __attribute__((packed));

static struct struct_elem_attr ca_blobs_desc[] = 
{
	{"asymBlobSize",CUBE_TYPE_INT,sizeof(int),NULL},
	{"symBlobSize",CUBE_TYPE_INT,sizeof(int),NULL},
	{"asymBlob",CUBE_TYPE_DEFINE,1,"asymBlobSize"},
	{"symBlob",CUBE_TYPE_DEFINE,1,"symBlobSize"},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL}
};

TSS_RESULT TESI_AIK_ReadCABlob(void * blob, char * blob_file)
{
	TSS_RESULT result;
	void * blob_template;
	blob_template=create_struct_template(tpm_identity_req_desc);
	int fd;
	char filename[256];
	int retval;
	int blob_size;
	struct stat file_stat;

	sprintf(filename,"%s.req",blob_file);
	fd=open(filename,O_RDONLY);
	if(fd<=0)
		return -EINVAL;
	retval = fstat(fd,&file_stat);
	if(retval<0)
	{
		return -EACCES;
	}

	char *buffer;
	buffer=malloc(file_stat.st_size);
	if(buffer==NULL)
		return -ENOMEM;
	retval=read(fd,buffer,file_stat.st_size);
	blob_size=blob_2_struct(buffer,blob,blob_template);
	close(fd);
	free(buffer);
	return TSS_SUCCESS;
}

TSS_RESULT TESI_AIK_WriteCABlob(void * blob, char * blob_file)
{
	TSS_RESULT result;
	void * blob_template;
	blob_template=create_struct_template(tpm_identity_req_desc);
	int fd;
	char filename[256];
	int retval;
	int blob_size;
	sprintf(filename,"%s.req",blob_file);
	fd=open(filename,O_WRONLY|O_CREAT|O_TRUNC,0666);
	if(fd<=0)
		return -EINVAL;
	char buffer[4096];
	blob_size=struct_2_blob(blob,buffer,blob_template);
	retval=write(fd,buffer,blob_size);
	close(fd);
	return blob_size;
}

TSS_RESULT TESI_AIK_CreatePubIdentKey(TSS_HKEY * hKey)
{
	TSS_FLAG initFlags;
	TSS_RESULT result;

	initFlags	= TSS_KEY_TYPE_IDENTITY | TSS_KEY_SIZE_2048  |
			TSS_KEY_VOLATILE | TSS_KEY_NO_AUTHORIZATION |
			TSS_KEY_NOT_MIGRATABLE;
	return Tspi_Context_CreateObject(hContext,
			   TSS_OBJECT_TYPE_RSAKEY,
			   initFlags, hKey);
}

TSS_RESULT TESI_AIK_CreateIdentKey(TSS_HKEY * hKey,TSS_HKEY * hWrapKey,char * pwdw,char * pwdk)
{
	TSS_FLAG initFlags;
	TSS_RESULT result;
	int pwd_len;
	BYTE wpass[20];
	BYTE kpass[20];
	TSS_HPOLICY WrapkeyUsagePolicy,keyUsagePolicy;


	initFlags	= TSS_KEY_TYPE_IDENTITY | TSS_KEY_SIZE_2048  |
			TSS_KEY_VOLATILE | TSS_KEY_NO_AUTHORIZATION |
			TSS_KEY_NOT_MIGRATABLE;
	//Get WrapKey Policy Object
	
	if(hWrapKey==NULL)
		hWrapKey=hSRK;
	result =
	    Tspi_GetPolicyObject(hWrapKey, TSS_POLICY_USAGE, &WrapkeyUsagePolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_GetPolicyObject", result);
		return result;
	}

	pwd_len=strlen(pwdw);
	//Set Secret
	if(hWrapKey==hSRK)
	{
		if(pwdw!=NULL)
			TSS_sha1((unsigned char *)pwdw,strlen(pwdw),wpass);
		else
			return TSS_E_TSP_AUTHFAIL;
		result =   Tspi_Policy_SetSecret(WrapkeyUsagePolicy,
			TSS_SECRET_MODE_SHA1,
			sizeof(wpass),wpass);
		if (result != TSS_SUCCESS) {
			print_error("Tspi_Policy_SetSecret", result);
			return result;
		}
	}else
	{
		TSS_sha1((unsigned char *)pwdw,strlen(pwdw),wpass);
		result =  Tspi_Policy_SetSecret(keyUsagePolicy,
			  TSS_SECRET_MODE_SHA1,
			  sizeof(wpass),pwdw);

		if (result != TSS_SUCCESS) {
			print_error("Tspi_Policy_SetSecret", result);
			return result;
		}
	}

	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
				      initFlags, hKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		return result;
	}
	//Create Policy Object
	result =  Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_POLICY, TSS_POLICY_USAGE,
				      &keyUsagePolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		return result;
	}
	//Set Secret

	TSS_sha1((unsigned char *)pwdk,strlen(pwdk),kpass);
	result =  Tspi_Policy_SetSecret(keyUsagePolicy,
		  TSS_SECRET_MODE_SHA1,
		  sizeof(kpass),kpass);

	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		return result;
	}
	//Assign policy to key
	result = Tspi_Policy_AssignToObject(keyUsagePolicy, *hKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_AssignToObject", result);
		return result;
	}

	return TSS_SUCCESS;
}

TSS_RESULT  TESI_AIK_GenerateReq(TSS_HKEY hCAKey,int labelLen,BYTE * labelData, TSS_HKEY hAIKey,char * req)
{
	int ulTCPAIdentityReqLength;
	BYTE * rgbTCPAIdentityReq;
	TSS_RESULT result;
	char filename[256];
	int fd;

	result = Tspi_TPM_CollateIdentityRequest(hTPM, hSRK, hCAKey, labelLen,
						 labelData,
						 hAIKey, TSS_ALG_AES,
						 &ulTCPAIdentityReqLength,
						 &rgbTCPAIdentityReq);
	if (result != TSS_SUCCESS)
		return result;
	sprintf(filename,"%s.req",req);

	fd=open(filename,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR,0666);
	if(fd<0)
	{	
		printf("open req blob file error!\n");
		return -EEXIST;
	}
	write(fd,rgbTCPAIdentityReq,ulTCPAIdentityReqLength);
	close(fd);
}

TSS_RESULT
ca_verify_identity_binding(TSS_HKEY hCAKey, 
		TSS_HKEY hIdentityKey,
		TCPA_IDENTITY_PROOF *proof)
{
	TSS_RESULT result;
	BYTE blob[2048];
	UINT16 offset;
	TCPA_KEY key;
	TCPA_DIGEST digest, chosenId;
	UINT32 keyBlobSize, subCap = TSS_TPMCAP_VERSION, versionSize;
	BYTE *keyBlob;
	TSS_HHASH hHash;
	BYTE *versionBlob;

	if ((result = Tspi_Context_CreateObject(hContext,
						TSS_OBJECT_TYPE_HASH,
						TSS_HASH_SHA1, &hHash))) {
		print_error("Tspi_Context_CreateObject", result);
		return result;
	}

	if ((result = Tspi_GetAttribData(hCAKey, TSS_TSPATTRIB_KEY_BLOB,
					 TSS_TSPATTRIB_KEYBLOB_BLOB,
					 &keyBlobSize, &keyBlob))) {
		print_error("Tspi_GetAttribData", result);
		return result;
	}

	offset = 0;
	if ((result = TestSuite_UnloadBlob_KEY(&offset, keyBlob, &key))) {
		print_error("TestSuite_UnloadBlob_KEY", result);
		return result;
	}

	Tspi_Context_FreeMemory(hContext, keyBlob);

	/* make the chosen id hash */
	offset = 0;
	TestSuite_LoadBlob(&offset, proof->labelSize, blob, proof->labelArea);
	TestSuite_LoadBlob_KEY_PARMS(&offset, blob, &key.algorithmParms);
	TestSuite_LoadBlob_STORE_PUBKEY(&offset, blob, &key.pubKey);

	if ((result = TestSuite_Hash(TSS_HASH_SHA1, offset, blob,
				 (BYTE *)&chosenId.digest)))
		return result;

#ifndef TPM_ORD_MakeIdentity
#define TPM_ORD_MakeIdentity 121
#endif

	offset = 0;
	TestSuite_LoadBlob_TCPA_VERSION(&offset, blob,proof->ver);
	TestSuite_LoadBlob_UINT32(&offset, TPM_ORD_MakeIdentity, blob);
	TestSuite_LoadBlob(&offset, 20, blob, (BYTE *)&chosenId.digest);
	TestSuite_LoadBlob_PUBKEY(&offset, blob, &proof->identityKey);

	free(key.algorithmParms.parms);
	free(key.pubKey.key);
	free(key.encData);
	free(key.PCRInfo);

	if ((result = TestSuite_Hash(TSS_HASH_SHA1, offset, blob,
				 (BYTE *)&digest.digest)))
		return result;

	if ((result = Tspi_Hash_SetHashValue(hHash, 20, (BYTE *)&digest.digest))) {
		print_error("Tspi_Hash_SetHashValue", result);
		return result;
	}

	return Tspi_Hash_VerifySignature(hHash, hIdentityKey,
					 proof->identityBindingSize,
					 proof->identityBinding);
}

int TESI_AIK_CreateSignData(TESI_SIGN_DATA * signdata,void * privkey,BYTE * blob,int blobsize)
{
	BYTE * signbuf;
	int buflen;
	int ret;

	BYTE digest[DIGEST_SIZE];
	// create ca sign data 
	RSA * ca_priv=(RSA *)privkey;
	
	
	signbuf=malloc(0x2000);
	if(signbuf==NULL)
		return -ENOMEM;

	calculate_context_sm3(blob,blobsize,digest);

	
	ret=RSA_sign(NID_sha1,digest,DIGEST_SIZE,signbuf,&buflen,ca_priv);
	if(ret!=1)
		return -EINVAL;
	
	signdata->datalen=blobsize;
	signdata->data=blob;
	signdata->signlen=buflen;
	signdata->sign=malloc(buflen);
	if(signdata->sign==NULL)
	{
		free(signbuf);
		return -ENOMEM;
	}
	memcpy(signdata->sign,signbuf,buflen);
	return 0;
}

int TESI_AIK_VerifySignData(TESI_SIGN_DATA * signdata,char *name)
{
	BYTE *signbuf;
	BYTE *blob;
	int buflen;
	int blobsize;
	int ret;

	X509 *x=NULL;
        BIO *b;
        EVP_PKEY * CAPubKey;
        RSA * rsa_pub;
        char filename[256];             /* file name string of cert file */

	BYTE digest[DIGEST_SIZE];

        unsigned char   n[2048];//, p[2048];
        int             size_n;//, size_p;

        sprintf(filename,"%s.crt",name);
	
	b=BIO_new_file(filename,"r");
	x=PEM_read_bio_X509(b,NULL,NULL,NULL);

        BIO_free(b);
        CAPubKey=X509_get_pubkey(x);
        X509_free(x);
        rsa_pub=EVP_PKEY_get1_RSA(CAPubKey);

	blobsize=signdata->datalen;
	blob=malloc(blobsize);
	if(blob==NULL)
        {
                free(blob);
                return -ENOMEM;
        }
	memcpy(blob,signdata->data,blobsize);
	buflen=signdata->signlen;
	signbuf=malloc(buflen);
	if(signbuf==NULL)
        {
                free(signbuf);
                return -ENOMEM;
        }
	memcpy(signbuf,signdata->sign,buflen);

	calculate_context_sm3(blob,blobsize,digest);

	ret=RSA_verify(NID_sha1,digest,DIGEST_SIZE,signbuf,buflen,rsa_pub);
	if(ret!=1)
	{
		RSA_free(rsa_pub);
		return -ENOMEM;
	}
	RSA_free(rsa_pub);
	return 0;
	
}



int WriteSignDataToFile(void * data,char * name)
{
	char filename[128]; 
	TESI_SIGN_DATA * sign_data = (TESI_SIGN_DATA *)data;
	int namelen;
	int fd;
	void * signdata_template;
	BYTE buffer[4096];
	int ret;
	int signdatalen;
	namelen=strlen(name);
	if(namelen>120)
		return -EINVAL;
	memcpy(filename,name,namelen);
	memcpy(filename+namelen,".sda",5);
	
	fd=open(filename,O_CREAT|O_WRONLY|O_TRUNC,0666);	
	if(fd<0)
		return -EIO;
		
	signdata_template = create_struct_template(&tesi_sign_data_desc);
	if(signdata_template==NULL)
		return -EINVAL;
	signdatalen=struct_2_blob(sign_data,buffer,signdata_template);
	if(signdatalen<0)
		return signdatalen;
	ret=write(fd,buffer,signdatalen);
	if(ret!=signdatalen)
		return -EIO;
	close(fd);
	return signdatalen;	
}

int ReadSignDataFromFile(void * data,char * name)
{
	int ret;
	void * signdata_template;
	signdata_template=create_struct_template(&tesi_sign_data_desc);
	int fd;
	char filename[128];
	int retval;
	int signdatalen;
	struct stat file_stat;

	sprintf(filename,"%s.sda",name);
	fd=open(filename,O_RDONLY);
	if(fd<=0)
		return -EIO;
	ret = fstat(fd,&file_stat);
	if(ret<0)
	{
		return -EIO;
	}

	signdatalen=file_stat.st_size;
	char *buffer;
	buffer=malloc(signdatalen);
	if(buffer==NULL)
		return -ENOMEM;
	retval=read(fd,buffer,signdatalen);
	if(retval!=signdatalen)
		return -EIO;
	retval=blob_2_struct(buffer,data,signdata_template);
	if(retval<0)
		return -EINVAL;
	close(fd);
	free(buffer);
	return retval;
}

TSS_RESULT
TESI_AIK_CreateAIKCert(TSS_HKEY hIdentityKey,void * privkey,BYTE * data,int datasize,
	char * pubek,char * repname)
{
	RSA * ca_priv=(RSA *)privkey;
	TCPA_SYM_CA_ATTESTATION symAttestation;
	TCPA_ASYM_CA_CONTENTS asymContents;
	BYTE blob[2048], blob2[2048];
	BYTE *identityBlob;
	UINT32 identityBlobSize, blobSize = 2048, blob2Size = 2048;
	TCPA_KEY idKey;
	UINT16 offset;
	TSS_HKEY hPubEK;
	TSS_HENCDATA hEncData;
	TSS_RESULT result;
	BYTE *pubEK;
	UINT32 pubEKSize, tmpblobsize = 0x2000, i;
	BYTE tmpblob[0x2000];
	int ret;
	tpm_identity_req * b;
	TESI_SIGN_DATA signdata;
	BYTE * signbuf;
	int buflen;

	if ((result = Tspi_Context_CreateObject(hContext,
						TSS_OBJECT_TYPE_ENCDATA,
						TSS_ENCDATA_BIND,
						&hEncData))) {
		print_error("Tspi_Context_CreateObject", result);
		return result;
	}

	/* fill out the asym contents structure */
	asymContents.sessionKey.algId = symAlg;
	asymContents.sessionKey.encScheme = TCPA_ES_NONE;

	switch (asymContents.sessionKey.algId) {
		case TCPA_ALG_AES:
			asymContents.sessionKey.size = 128/8;
			break;
		case TCPA_ALG_DES:
			asymContents.sessionKey.size = 64/8;
			break;
		case TCPA_ALG_3DES:
			asymContents.sessionKey.size = 192/8;
			break;
		default:
			print_error("Invalid symmetric algorithm!", -1);
			return TSS_E_INTERNAL_ERROR;
	}

	asymContents.sessionKey.data=malloc(asymContents.sessionKey.size);
	ret=RAND_bytes(asymContents.sessionKey.data,
			asymContents.sessionKey.size);		


	/* TPM 1.1b spec pg. 277 step 4 says "digest of the public key of
	 * idKey"...  I'm assuming that this is a TCPA_PUBKEY */
	if ((result = Tspi_GetAttribData(hIdentityKey,
					 TSS_TSPATTRIB_KEY_BLOB,
					 TSS_TSPATTRIB_KEYBLOB_BLOB,
					 &identityBlobSize,
					 &identityBlob))) {
		print_error("Tspi_GetAttribData", result);
		Tspi_Context_FreeMemory(hContext,
					asymContents.sessionKey.data);
		return result;
	}

	offset = 0;
	if ((result = TestSuite_UnloadBlob_KEY(&offset, identityBlob, &idKey))) {
		print_error("TestSuite_UnloadBlob_KEY", result);
		return result;
	}

	offset = 0;
	TestSuite_LoadBlob_KEY_PARMS(&offset, blob, &idKey.algorithmParms);
	TestSuite_LoadBlob_STORE_PUBKEY(&offset, blob, &idKey.pubKey);
	TestSuite_Hash(TSS_HASH_SHA1, offset, blob,
		   (BYTE *)&asymContents.idDigest.digest);

	/* create the TCPA_SYM_CA_ATTESTATION structure */
	symAttestation.algorithm.algorithmID = symAlg;
	symAttestation.algorithm.encScheme = TCPA_ES_NONE;
	symAttestation.algorithm.sigScheme = 0;
	symAttestation.algorithm.parmSize = 0;
	symAttestation.algorithm.parms = NULL;

	ret=TESI_AIK_CreateSignData(&signdata,ca_priv,data,datasize);	
	if(ret<0)
		return -EINVAL;
	void * template=create_struct_template(tesi_sign_data_desc);
	buflen=struct_2_blob(&signdata,tmpblob,template);

	if(buflen<=0)
		return -EINVAL;

	if ((result = TestSuite_SymEncrypt(symAlg, TCPA_ES_NONE, asymContents.sessionKey.data, NULL,
					   tmpblob,buflen,
					   blob2, &blob2Size))) {
		print_error("TestSuite_SymEncrypt", result);
		Tspi_Context_FreeMemory(hContext, asymContents.sessionKey.data);
		return result;
	}
	/* blob2 now contains the encrypted credential, which is part of the TCPA_SYM_CA_ATTESTATION
	 * structure that we'll pass into ActivateIdentity. */
	symAttestation.credential = blob2;
	symAttestation.credSize = blob2Size;
	offset = 0;
	TestSuite_LoadBlob_SYM_CA_ATTESTATION(&offset, blob, &symAttestation);

	b=malloc(sizeof(tpm_identity_req));
	if(b==NULL)
		return TSS_E_OUTOFMEMORY;

	memset(b,0,sizeof(tpm_identity_req));

	/* blob now contains the serialized TCPA_SYM_CA_ATTESTATION struct */
	if ((b->symBlob = malloc(offset)) == NULL) {
		fprintf(stderr, "malloc failed.");
		Tspi_Context_FreeMemory(hContext, asymContents.sessionKey.data);
		return TSS_E_OUTOFMEMORY;
	}

	memcpy(b->symBlob, blob, offset);
	b->symSize = offset;

	/* encrypt the TCPA_ASYM_CA_CONTENTS blob with CA Key */
	offset = 0;
	TestSuite_LoadBlob_ASYM_CA_CONTENTS(&offset, blob, &asymContents);

	/* free the malloc'd structure member now that its in the blob */
	Tspi_Context_FreeMemory(hContext, asymContents.sessionKey.data);

	if(result = TESI_Local_ReadPubKey(&hPubEK,pubek))
	{
		free(b->symBlob);
		b->symBlob = NULL;
		b->symSize = 0;
		return result;

	}


	if ((result = Tspi_GetAttribData(hPubEK, TSS_TSPATTRIB_RSAKEY_INFO,
					TSS_TSPATTRIB_KEYINFO_RSA_MODULUS,
					&pubEKSize, &pubEK))) {
		free(b->symBlob);
		b->symBlob = NULL;
		b->symSize = 0;
		free(b);
		return result;
	}
	/* encrypt using the TPM's custom OAEP padding parameter */


//	RSA_sign_ASN1_OCTET_STRING(0,blob,offset,tmpblob,&tmpblobsize,CAKey);
	
	

	if ((result = TestSuite_TPM_RSA_Encrypt(blob, offset, tmpblob,
						&tmpblobsize, pubEK,
						pubEKSize))) {
		Tspi_Context_FreeMemory(hContext, pubEK);
		free(b->symBlob);
		b->symBlob = NULL;
		b->symSize = 0;
		free(b);
		return result;
	}

	Tspi_Context_FreeMemory(hContext, pubEK);

	if ((b->asymBlob = malloc(tmpblobsize)) == NULL) {
		free(b->symBlob);
		b->symBlob = NULL;
		b->symSize = 0;
		free(b);
		return TSS_E_OUTOFMEMORY;
	}

	memcpy(b->asymBlob, tmpblob, tmpblobsize);
	b->asymSize = tmpblobsize;

	TESI_AIK_WriteCABlob(b,repname);
	free(b->symBlob);
	free(b);
	return TSS_SUCCESS;
}

TSS_RESULT
ca_create_credential(TSS_HKEY hIdentityKey, 
		char * pubek,char * repname)
{
	TCPA_SYM_CA_ATTESTATION symAttestation;
	TCPA_ASYM_CA_CONTENTS asymContents;
	BYTE blob[2048], blob2[2048];
	BYTE *identityBlob;
	UINT32 identityBlobSize, blobSize = 2048, blob2Size = 2048;
	TCPA_KEY idKey;
	UINT16 offset;
	TSS_HKEY hPubEK;
	TSS_HENCDATA hEncData;
	TSS_RESULT result;
	BYTE *pubEK;
	UINT32 pubEKSize, tmpblobsize = 0x2000, i;
	BYTE tmpblob[0x2000];
	int ret;
	tpm_identity_req * b;

	if ((result = Tspi_Context_CreateObject(hContext,
						TSS_OBJECT_TYPE_ENCDATA,
						TSS_ENCDATA_BIND,
						&hEncData))) {
		print_error("Tspi_Context_CreateObject", result);
		return result;
	}

	/* fill out the asym contents structure */
	asymContents.sessionKey.algId = symAlg;
	asymContents.sessionKey.encScheme = TCPA_ES_NONE;

	switch (asymContents.sessionKey.algId) {
		case TCPA_ALG_AES:
			asymContents.sessionKey.size = 128/8;
			break;
		case TCPA_ALG_DES:
			asymContents.sessionKey.size = 64/8;
			break;
		case TCPA_ALG_3DES:
			asymContents.sessionKey.size = 192/8;
			break;
		default:
			print_error("Invalid symmetric algorithm!", -1);
			return TSS_E_INTERNAL_ERROR;
	}

	asymContents.sessionKey.data=malloc(asymContents.sessionKey.size);
	ret=RAND_bytes(asymContents.sessionKey.data,
			asymContents.sessionKey.size);		


	/* TPM 1.1b spec pg. 277 step 4 says "digest of the public key of
	 * idKey"...  I'm assuming that this is a TCPA_PUBKEY */
	if ((result = Tspi_GetAttribData(hIdentityKey,
					 TSS_TSPATTRIB_KEY_BLOB,
					 TSS_TSPATTRIB_KEYBLOB_BLOB,
					 &identityBlobSize,
					 &identityBlob))) {
		print_error("Tspi_GetAttribData", result);
		Tspi_Context_FreeMemory(hContext,
					asymContents.sessionKey.data);
		return result;
	}

	offset = 0;
	if ((result = TestSuite_UnloadBlob_KEY(&offset, identityBlob, &idKey))) {
		print_error("TestSuite_UnloadBlob_KEY", result);
		return result;
	}

	offset = 0;
	TestSuite_LoadBlob_KEY_PARMS(&offset, blob, &idKey.algorithmParms);
	TestSuite_LoadBlob_STORE_PUBKEY(&offset, blob, &idKey.pubKey);
	TestSuite_Hash(TSS_HASH_SHA1, offset, blob,
		   (BYTE *)&asymContents.idDigest.digest);

	/* create the TCPA_SYM_CA_ATTESTATION structure */
	symAttestation.algorithm.algorithmID = symAlg;
	symAttestation.algorithm.encScheme = TCPA_ES_NONE;
	symAttestation.algorithm.sigScheme = 0;
	symAttestation.algorithm.parmSize = 0;
	symAttestation.algorithm.parms = NULL;

	/* set the credential to something known before encrypting it
	 * so that we can compare the credential that the TSS returns to
	 * something other than all 0's */
	memset(&ca_cred, CERT_VERIFY_BYTE, sizeof(struct trousers_ca_tpm_identity_credential));

	/* encrypt the credential w/ the sym key */
	if ((result = TestSuite_SymEncrypt(symAlg, TCPA_ES_NONE, asymContents.sessionKey.data, NULL,
					   (BYTE *)&ca_cred,
					   sizeof(struct trousers_ca_tpm_identity_credential),
					   blob2, &blob2Size))) {
		print_error("TestSuite_SymEncrypt", result);
		Tspi_Context_FreeMemory(hContext, asymContents.sessionKey.data);
		return result;
	}

	/* blob2 now contains the encrypted credential, which is part of the TCPA_SYM_CA_ATTESTATION
	 * structure that we'll pass into ActivateIdentity. */
	symAttestation.credential = blob2;
	symAttestation.credSize = blob2Size;
	offset = 0;
	TestSuite_LoadBlob_SYM_CA_ATTESTATION(&offset, blob, &symAttestation);

	b=malloc(sizeof(tpm_identity_req));
	if(b==NULL)
		return TSS_E_OUTOFMEMORY;

	memset(b,0,sizeof(tpm_identity_req));

	/* blob now contains the serialized TCPA_SYM_CA_ATTESTATION struct */
	if ((b->symBlob = malloc(offset)) == NULL) {
		fprintf(stderr, "malloc failed.");
		Tspi_Context_FreeMemory(hContext, asymContents.sessionKey.data);
		return TSS_E_OUTOFMEMORY;
	}

	memcpy(b->symBlob, blob, offset);
	b->symSize = offset;

	/* encrypt the TCPA_ASYM_CA_CONTENTS blob with CA Key */
	offset = 0;
	TestSuite_LoadBlob_ASYM_CA_CONTENTS(&offset, blob, &asymContents);

	/* free the malloc'd structure member now that its in the blob */
	Tspi_Context_FreeMemory(hContext, asymContents.sessionKey.data);

	if(result = TESI_Local_ReadPubKey(&hPubEK,pubek))
	{
		free(b->symBlob);
		b->symBlob = NULL;
		b->symSize = 0;
		return result;

	}


	if ((result = Tspi_GetAttribData(hPubEK, TSS_TSPATTRIB_RSAKEY_INFO,
					TSS_TSPATTRIB_KEYINFO_RSA_MODULUS,
					&pubEKSize, &pubEK))) {
		free(b->symBlob);
		b->symBlob = NULL;
		b->symSize = 0;
		free(b);
		return result;
	}
	/* encrypt using the TPM's custom OAEP padding parameter */


//	RSA_sign_ASN1_OCTET_STRING(0,blob,offset,tmpblob,&tmpblobsize,CAKey);
	
	

	if ((result = TestSuite_TPM_RSA_Encrypt(blob, offset, tmpblob,
						&tmpblobsize, pubEK,
						pubEKSize))) {
		Tspi_Context_FreeMemory(hContext, pubEK);
		free(b->symBlob);
		b->symBlob = NULL;
		b->symSize = 0;
		free(b);
		return result;
	}

	Tspi_Context_FreeMemory(hContext, pubEK);

	if ((b->asymBlob = malloc(tmpblobsize)) == NULL) {
		free(b->symBlob);
		b->symBlob = NULL;
		b->symSize = 0;
		free(b);
		return TSS_E_OUTOFMEMORY;
	}

	memcpy(b->asymBlob, tmpblob, tmpblobsize);
	b->asymSize = tmpblobsize;

	TESI_AIK_WriteCABlob(b,repname);
	free(b->symBlob);
	free(b);
	return TSS_SUCCESS;
}

TSS_RESULT TESI_AIK_VerifyReq(void * privkey,TSS_HKEY hCAKey, char * req,TSS_HKEY * hAIKey,TPM_IDENTITY_PROOF * identityProof )
{
	TSS_RESULT result;
	RSA * rsa=(RSA *)privkey;
	int ret;
	tpm_identity_req b;
	UINT32			utilityBlobSize;
	UINT16 			offset;
	BYTE			utilityBlob[USHRT_MAX], *cred;
	int			padding = RSA_PKCS1_OAEP_PADDING, tmp;
	TCPA_SYMMETRIC_KEY	symKey;
	TCPA_ALGORITHM_ID	algID;
	TCPA_SYM_CA_ATTESTATION symAttestation;

	result=TESI_AIK_ReadCABlob(&b,req);
	if(result!=TSS_SUCCESS)	
		return -EINVAL;

	if ((ret = RSA_private_decrypt(b.asymSize,b.asymBlob,
				       utilityBlob,
				       rsa, padding)) <= 0) {
		print_error("RSA_private_decrypt", ret);
		return result;
	}
	offset = 0;
	if ((result = TestSuite_UnloadBlob_SYMMETRIC_KEY(&offset, utilityBlob,
						     &symKey))) {
		print_error("TestSuite_UnloadBlob_SYMMETRIC_KEY", result);
		return result;
	}

	switch (symKey.algId) {
		case TCPA_ALG_DES:
			algID = TSS_ALG_DES;
			break;
		case TCPA_ALG_3DES:
			algID = TSS_ALG_3DES;
			break;
		case TCPA_ALG_AES:
			algID = TSS_ALG_AES;
			break;
		default:
			fprintf(stderr, "symmetric blob encrypted with an "
				"unknown cipher\n");
			return result;
			break;
	}

	utilityBlobSize = sizeof(utilityBlob);
	if ((result = TestSuite_SymDecrypt(algID, symKey.encScheme, symKey.data, NULL,
				       b.symBlob, b.symSize, utilityBlob,
				       &utilityBlobSize))) {
		print_error("TestSuite_SymDecrypt", result);
		RSA_free(rsa);
		exit(result);
	}

	offset = 0;
	if ((result = TestSuite_UnloadBlob_IDENTITY_PROOF(&offset, utilityBlob,
						      identityProof))) {
		print_error("TestSuite_UnloadBlob_IDENTITY_PROOF", result);
		RSA_free(rsa);
		exit(result);
	}


	printf("create pub CA Key succeed!\n");

	// Get the IdentKey
	offset=0;
	TestSuite_LoadBlob_PUBKEY(&offset,utilityBlob,
			&(identityProof->identityKey));

	TESI_AIK_CreatePubIdentKey(hAIKey); 
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		exit(result);
	}


	/* TPM 1.1b spec pg. 277 step 4 says "digest of the public key of
	 * idKey"...  I'm assuming that this is a TCPA_PUBKEY */
	if ((result = Tspi_SetAttribData(*hAIKey,
					 TSS_TSPATTRIB_KEY_BLOB,
					 TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY,
					 offset,
					 utilityBlob))) {
		print_error("Tspi_SetAttribData", result);
		return result;
	}

	printf("set ident key Succeed!\n");

	/* verify the identity binding */
	
	if ((result = ca_verify_identity_binding(hCAKey,
				*hAIKey, identityProof))) {
		if (TSS_ERROR_CODE(result) == TSS_E_FAIL)
			fprintf(stderr, "Identity Binding signature doesn't "
				"match!\n");
		print_error("ca_verify_identity_binding", result);
		return result;
	}
	return TSS_SUCCESS;

}

TSS_RESULT TESI_AIK_Activate(TSS_HKEY hAIK, char * req, TESI_SIGN_DATA * signdata)
{
	TSS_RESULT result;
	tpm_identity_req b;
	char filename[256];
	int cred_len;
	BYTE * cred_data;
	int ret;

	result=TESI_AIK_ReadCABlob(&b,req);


	result = Tspi_TPM_ActivateIdentity(hTPM, hAIK, b.asymSize,
					   b.asymBlob, b.symSize,
					   b.symBlob, &cred_len, &cred_data);

	if (result) {
		print_error("Tspi_TPM_ActivateIdentity", result);
		return result;
	}

	void * template=create_struct_template(tesi_sign_data_desc);
	ret=blob_2_struct(cred_data,signdata,template);

	if(ret<0)
		return -EINVAL;
	
	return TSS_SUCCESS;
}

TSS_RESULT TESI_Local_Bind(char * plainname, TSS_HKEY hKey, char * ciphername)
{
	TSS_HENCDATA hEncData;
	TSS_RESULT result;
	FILE * file;
	FILE* oFile;
	UINT32 datalen;
	void* rgbExternalData;
	void* rgbEncryptedData = NULL;
	UINT32	ulEncryptedDataLength = 0;

 	// read file text
	file = fopen(plainname,"rb");
  	if (file == NULL)
   	{
   	   printf("Unable to read plain file %s.\n",plainname);
   	   return TSS_E_KEY_NOT_LOADED;
  	 }
	 fseek(file,0,SEEK_END);
	 datalen=ftell(file);
	 fseek(file,0,SEEK_SET);
	 
	rgbExternalData=(BYTE *)malloc(datalen);
	if(rgbExternalData == NULL)
		return TSS_E_OUTOFMEMORY;

   	 result = fread((rgbExternalData),datalen,1,file);
  	 if (result != 1)
  	 {
			fclose(file);
			free(rgbExternalData);
			printf("I/O Error reading plain file\n");
			return TSS_E_OUTOFMEMORY;
     	 }

	 fclose(file);
	
	result = Tspi_Context_CreateObject( hContext,
						TSS_OBJECT_TYPE_ENCDATA,
						TSS_ENCDATA_BIND, &hEncData );
						
	if ( result != TSS_SUCCESS )
	{
		free(rgbExternalData);
		print_error( "Tspi_Context_CreateObject (hEncData)", result );
		return result;
	}

		// Data Bind
	result = Tspi_Data_Bind( hEncData, hKey, datalen, rgbExternalData );
	if ( result != TSS_SUCCESS )
	{
		free(rgbExternalData);
		print_error("Tspi_Data_Bind", result);
		return result;
	}
	free(rgbExternalData);
	//Get data
	result = Tspi_GetAttribData(hEncData, TSS_TSPATTRIB_ENCDATA_BLOB,
					TSS_TSPATTRIB_ENCDATABLOB_BLOB,
					&ulEncryptedDataLength, &rgbEncryptedData);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_GetAttribData", result );
		return result;
	}
	
	printf("Data after encrypting:\n");
	print_hex(rgbEncryptedData, ulEncryptedDataLength);
	
	oFile=fopen(ciphername,"wb");
	fwrite(rgbEncryptedData,ulEncryptedDataLength,1,oFile);
	fclose(oFile);
}

TSS_RESULT TESI_Local_BindBuffer(void * inbuffer,int inlength, TSS_HKEY hKey, void** outbuffer, int * outlength)
{
	TSS_HENCDATA hEncData;
	TSS_RESULT result;
	UINT32 datalen;
	void* rgbExternalData;
	void* rgbEncryptedData = NULL;
	UINT32	ulEncryptedDataLength = 0;

	result = Tspi_Context_CreateObject( hContext,
						TSS_OBJECT_TYPE_ENCDATA,
						TSS_ENCDATA_BIND, &hEncData );
						
	if ( result != TSS_SUCCESS )
	{
		free(rgbExternalData);
		print_error( "Tspi_Context_CreateObject (hEncData)", result );
		return result;
	}

		// Data Bind
	result = Tspi_Data_Bind( hEncData, hKey, inlength,inbuffer );
	if ( result != TSS_SUCCESS )
	{
		print_error("TESI_Local_BindBuffer", result);
		return result;
	}
	//Get data
	result = Tspi_GetAttribData(hEncData, TSS_TSPATTRIB_ENCDATA_BLOB,
					TSS_TSPATTRIB_ENCDATABLOB_BLOB,
					outlength, outbuffer);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_GetAttribData", result );
		return result;
	}
	return result;
}


TSS_RESULT TESI_Local_UnBind(char * ciphername, TSS_HKEY hKey, char * plainname)
{
	TSS_HENCDATA hEncData;
	TSS_RESULT result;
	FILE * file;
	FILE* oFile;
	UINT32 datalen;
	void* rgbExternalData;
	void* rgbEncryptedData = NULL;
	UINT32	ulEncryptedDataLength = 0;

 	// read file text
	file = fopen(ciphername,"rb");
  	if (file == NULL)
   	{
   	   printf("Unable to read cinpher file %s.\n",ciphername);
   	   return TSS_E_KEY_NOT_LOADED;
  	}
	 fseek(file,0,SEEK_END);
	 datalen=ftell(file);
	 fseek(file,0,SEEK_SET);
	 
	rgbExternalData=(BYTE *)malloc(datalen);
	if(rgbExternalData == NULL)
		return TSS_E_OUTOFMEMORY;

   	 result = fread((rgbExternalData),datalen,1,file);
  	 if (result != 1)
   	 {
		 fclose(file);
		 free(rgbExternalData);
	      printf("I/O Error reading data cipher file\n");
 	      return TSS_E_OUTOFMEMORY;
     	 }

	 fclose(file);
	
	result = Tspi_Context_CreateObject( hContext,
						TSS_OBJECT_TYPE_ENCDATA,
						TSS_ENCDATA_BIND, &hEncData );
	if ( result != TSS_SUCCESS )
	{
		free(rgbExternalData);
		print_error( "Tspi_Context_CreateObject (hEncData)", result );
		return result;
	}
	
	//Get data
	result = Tspi_SetAttribData(hEncData, TSS_TSPATTRIB_ENCDATA_BLOB,
					TSS_TSPATTRIB_ENCDATABLOB_BLOB,
					datalen, rgbExternalData);
	free(rgbExternalData);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_SetAttribData", result );
		return result;
	}
	
	//Unbind data
	result = Tspi_Data_Unbind( hEncData, hKey, &ulEncryptedDataLength,
					&rgbEncryptedData );
	
	printf("Data after decrypting:\n");
	print_hex(rgbEncryptedData, ulEncryptedDataLength);
	
	oFile=fopen(plainname,"wb");
	fwrite(rgbEncryptedData,ulEncryptedDataLength,1,oFile);
	fclose(oFile);
}

TSS_RESULT TESI_Local_UnBindBuffer(void * inbuffer,int inlength, TSS_HKEY hKey, void** outbuffer, int * outlength)
{
	TSS_HENCDATA hEncData;
	TSS_RESULT result;
	UINT32 datalen;
	void* rgbEncryptedData = NULL;
	UINT32	ulEncryptedDataLength = 0;

	result = Tspi_Context_CreateObject( hContext,
						TSS_OBJECT_TYPE_ENCDATA,
						TSS_ENCDATA_BIND, &hEncData );
						
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_CreateObject (hEncData)", result );
		return result;
	}
	//Set data
	result = Tspi_SetAttribData(hEncData, TSS_TSPATTRIB_ENCDATA_BLOB,
					TSS_TSPATTRIB_ENCDATABLOB_BLOB,
					inlength, inbuffer);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_SetAttribData", result );
		return result;
	}
	

		// Data UnBind
	result = Tspi_Data_Unbind( hEncData, hKey, outlength,outbuffer );
	if ( result != TSS_SUCCESS )
	{
		print_error("TESI_Local_UnBindBuffer", result);
		return result;
	}
	return result;
}


TSS_RESULT TESI_Local_Seal(char * plainname, TSS_HKEY hKey, char * ciphername,TSS_HPCRS hPcr)
{
	TSS_HENCDATA hEncData;
	TSS_RESULT result;
	FILE * file;
	FILE* oFile;
	UINT32 datalen;
	void* rgbExternalData;
	void* rgbEncryptedData = NULL;
	UINT32	ulEncryptedDataLength = 0;

 	// read file text
	file = fopen(plainname,"rb");
  	if (file == NULL)
   	{
   	   printf("Unable to read plain file %s.\n",plainname);
   	   return TSS_E_KEY_NOT_LOADED;
  	 }
	 fseek(file,0,SEEK_END);
	 datalen=ftell(file);
	 fseek(file,0,SEEK_SET);
	 
	rgbExternalData=(BYTE *)malloc(datalen);
	if(rgbExternalData == NULL)
		return TSS_E_OUTOFMEMORY;

   	 result = fread((rgbExternalData),datalen,1,file);
  	 if (result != 1)
  	 {
			fclose(file);
			free(rgbExternalData);
			printf("I/O Error reading plain file\n");
			return TSS_E_OUTOFMEMORY;
     	 }

	 fclose(file);
	
	result = Tspi_Context_CreateObject( hContext,
						TSS_OBJECT_TYPE_ENCDATA,
						TSS_ENCDATA_SEAL, &hEncData );
						
	if ( result != TSS_SUCCESS )
	{
		free(rgbExternalData);
		print_error( "Tspi_Context_CreateObject (hEncData)", result );
		return result;
	}

		// Data Bind
	result = Tspi_Data_Seal( hEncData, hKey, datalen, rgbExternalData,NULL );
	if ( result != TSS_SUCCESS )
	{
		free(rgbExternalData);
		print_error("Tspi_Data_Bind", result);
		return result;
	}
	free(rgbExternalData);
	//Get data
	result = Tspi_GetAttribData(hEncData, TSS_TSPATTRIB_ENCDATA_BLOB,
					TSS_TSPATTRIB_ENCDATABLOB_BLOB,
					&ulEncryptedDataLength, &rgbEncryptedData);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_GetAttribData", result );
		return result;
	}
	
	printf("Data after encrypting:\n");
	print_hex(rgbEncryptedData, ulEncryptedDataLength);
	
	oFile=fopen(ciphername,"wb");
	fwrite(rgbEncryptedData,ulEncryptedDataLength,1,oFile);
	fclose(oFile);
}

