/*writen by tjs in 2011-4-19*/

#ifndef _SM3_
#define _SM3_

#include <inttypes.h>

//#define DEBUG

typedef struct
{
  uint32_t total_bytes_High;
  uint32_t total_bytes_Low;
  uint32_t vector[8];
  uint8_t  buffer[64];     /* 64 byte buffer                            */
} SM3_context;


#define rol(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))
/*
inline int rol(uint32_t operand, uint8_t width){ 
	 asm volatile("rol %%cl, %%eax" 
               : "=a" (operand) 
               : "a" (operand), "c" (width) 
               ); 
}
*/
#define P0(x) ((x^(rol(x,9))^(rol(x,17))))
#define P1(x) ((x^(rol(x,15))^(rol(x,23))))

#define CONCAT_4_BYTES( w32, w8, w8_i)            \
{                                                 \
    (w32) = ( (uint32_t) (w8)[(w8_i)    ] << 24 ) |  \
            ( (uint32_t) (w8)[(w8_i) + 1] << 16 ) |  \
            ( (uint32_t) (w8)[(w8_i) + 2] <<  8 ) |  \
            ( (uint32_t) (w8)[(w8_i) + 3]       );   \
}

#define SPLIT_INTO_4_BYTES( w32, w8, w8_i)        \
{                                                 \
    (w8)[(w8_i)] = (uint8_t) ( (w32) >> 24 );    \
    (w8)[(w8_i) + 1] = (uint8_t) ( (w32) >> 16 );    \
    (w8)[(w8_i) + 2] = (uint8_t) ( (w32) >>  8 );    \
    (w8)[(w8_i) + 3] = (uint8_t) ( (w32)       );    \
}

static uint8_t SM3_padding[64] =
{
 (uint8_t) 0x80, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0,
 (uint8_t)    0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0,
 (uint8_t)    0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0,
 (uint8_t)    0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0
};

#endif
