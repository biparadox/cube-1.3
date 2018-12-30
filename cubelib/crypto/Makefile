FLAGS= -g -o 
CFLAGS= -g -c -fPIC -Wall -DUSER_MODE 
CC=gcc
CCP=g++
STRUCTOBJ= sha1.o sm3.o sm4.o crypto_func.o radix64.o
LIBNAME=crypto_func
DEPENDLIB=

main: ../lib/lib$(LIBNAME).so  ../lib/lib$(LIBNAME).a test_main
../lib/lib$(LIBNAME).so: $(STRUCTOBJ)
	$(CC) -shared -o lib$(LIBNAME).so $(STRUCTOBJ)
	mv   lib$(LIBNAME).so ../lib
../lib/lib$(LIBNAME).a: $(STRUCTOBJ)
	ar -rc lib$(LIBNAME).a $(STRUCTOBJ)
	mv   lib$(LIBNAME).a ../lib
sha1.o: sha1.c 
	$(CC) $(CFLAGS) sha1.c
sm3.o:  sm3.c 
	$(CC) $(CFLAGS) sm3.c
sm4.o:  sm4.c 
	$(CC) $(CFLAGS) sm4.c
crypto_func.o:  crypto_func.c 
	$(CC) $(CFLAGS) crypto_func.c
radix64.o:  radix64.c ../include/radix64.h 
	$(CC) $(CFLAGS) radix64.c
clean:
	rm -f *~ *.a *.o ../lib/lib$(LIBNAME).so
