#ifndef  CRYPTO_FUNC_H
#define  CRYPTO_FUNC_H
#define DIGEST_SIZE 32

int digest_to_uuid(BYTE *digest,char *uuid);
int uuid_to_digest(char * uuid,BYTE *digest);
int comp_proc_uuid(BYTE * dev_uuid,char * proc_name,BYTE * conn_uuid);
int calculate_by_context(BYTE **buffer,int countline,UINT32 *SM3_hash);
int calculate_context_sm3(BYTE* context, int context_size, UINT32 *SM3_hash);
int calculate_context_sha1(BYTE* context,int context_size,UINT32 *SM3_hash);
int extend_pcr_sm3digest(BYTE * pcr_value,BYTE * sm3digest);
int is_valid_uuidstr(char * uuidstr);
int sm4_context_crypt( BYTE * input, BYTE ** output, int size,char * passwd);
int sm4_context_decrypt( BYTE * input, BYTE ** output, int size,char * passwd);
int calculate_sm3(char* filename, UINT32 *SM3_hash);
int get_random_uuid(BYTE * uuid);

int sm4_text_crypt( BYTE * input, BYTE ** output, BYTE * passwd);
int sm4_text_decrypt( BYTE * input, BYTE ** output, BYTE * passwd);
void sm4_data_prepare(int input_len,BYTE * input_data,int * output_len,BYTE * output_data);
int sm4_data_recover(int input_len,BYTE * input_data,int * output_len,BYTE * output_data);

typedef struct
{
  UINT32 total_bytes_High;
  UINT32 total_bytes_Low;
  UINT32 vector[8];
  BYTE  buffer[64];     /* 64 byte buffer                            */

  BYTE ipad[64];       // HMAC: inner padding
  BYTE opad[64];       // HMAC: outer padding	
} sm3_context;

int SM3_init(sm3_context *index);
int SM3_update(sm3_context *index, BYTE *chunk_data, UINT32 chunk_length);
int SM3_final(sm3_context *index, UINT32 *SM3_hash);
void SM3_hmac(BYTE * key,int keylen, BYTE * input, int ilen,BYTE * output);

#endif
