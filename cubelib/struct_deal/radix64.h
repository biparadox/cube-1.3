#ifndef RADIX_64_H
#define RADIX_64_H

int bin_to_radix64_len(int length);
int radix_to_bin_len(int length);
int bin_to_radix64(char * radix64,int length,unsigned char * bin); 
int radix64_to_bin(unsigned char * bin,int length, char * radix64); 

#endif
