#ifndef CUBE_CAFUNC_H
#define CUBE_CAFUNC_H

struct entry
{
	char *key;
	char *value;
}__attribute__((packed));

int linux_get_random(void * buf,int num);
int print_openssl_err();
void * Generate_RSA_Key();
int Sign_RSA_file(char * filename, void * rsa_data,char * signfile);
int Pubcrypt_RSA_file(char * filename, void * rsa_data,char * cipherfile);
int Privdecrypt_RSA_file(char * filename, void * rsa_data,char * plainfile);
int Verify_RSA_file(char * filename, void * rsa_data,char * signfile);

int Create_X509_RSA_Cert(char * cert_name,int entry_count,void * entry,void * pubkey,void * signkey);

#endif

