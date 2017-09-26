#ifndef _SHA1_H_
#define _SHA1_H_

/*
SHA-1 in C
By Steve Reid <steve@edmweb.com>
100% Public Domain
*/
#include <inttypes.h>
typedef  struct tagSHA1_CTX{
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
}SHA1_CTX;

void SHA1Transform(uint32_t state[5], uint8_t buffer[64]);
void SHA1Init(void* context);
void SHA1Update(void* context, unsigned char* data, uint32_t len);
void SHA1Final(unsigned char  digest[20], void* context);

#endif /* _SHA1_H_ */
