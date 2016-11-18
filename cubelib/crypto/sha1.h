#ifndef _SHA1_H_
#define _SHA1_H_

/*
SHA-1 in C
By Steve Reid <steve@edmweb.com>
100% Public Domain
*/
typedef  struct tagSHA1_CTX{
    UINT32 state[5];
    UINT32 count[2];
    unsigned char buffer[64];
}SHA1_CTX;

void SHA1Transform(UINT32 state[5], BYTE buffer[64]);
void SHA1Init(void* context);
void SHA1Update(void* context, unsigned char* data, UINT32 len);
void SHA1Final(unsigned char  digest[20], void* context);

#endif /* _SHA1_H_ */
