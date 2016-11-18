FLAGS= -g -o 
CFLAGS= -g -c -DUSER_MODE 
CC=gcc
CCP=g++
FUNCOBJ= sha1.o sm3.o crypto_func.o radix64.o

main: ../lib/libcrypto_func.a
	rm -f *~ *.o *.a
#policy_test: ui/policy_test.c ui/policy_ui.so
#	$(CC) $(FLAGS) policy_test ui/policy_test.c ui/policy_ui.so
sha1.o: sha1.c 
	$(CC) $(CFLAGS) sha1.c
sm3.o:  sm3.c 
	$(CC) $(CFLAGS) sm3.c
crypto_func.o:  crypto_func.c 
	$(CC) $(CFLAGS) crypto_func.c
radix64.o:  radix64.c ../include/radix64.h 
	$(CC) $(CFLAGS) radix64.c
../lib/libcrypto_func.a: $(FUNCOBJ) 	
	ar rs libcrypto_func.a $(FUNCOBJ) 
	cp libcrypto_func.a ../lib
#	cp $(MEMDBHEADER) ../include
clean:
	rm -f *~ *.o *.a
