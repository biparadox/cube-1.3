FLAGS= -g -o 
CFLAGS= -g -c -fPIC -Wall -DUSER_MODE 
CC=gcc
CCP=g++
STRUCTOBJ= sha1.o sm3.o crypto_func.o radix64.o
LIBNAME=crypto_func
DEPENDLIB=

main: ../lib/lib$(LIBNAME).a  ../lib/lib$(LIBNAME).so
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
../lib/lib$(LIBNAME).a: $(STRUCTOBJ)
	ar rs lib$(LIBNAME).a $(STRUCTOBJ)
	mv    lib$(LIBNAME).a ../lib
../lib/lib$(LIBNAME).so:  $(STRUCTOBJ)
	ar cr lib$(LIBNAME).so $(STRUCTOBJ) 
	mv    lib$(LIBNAME).so ../lib
clean:
	rm -f *~ *.a *.o ../lib/$(LIBNAME).a ../lib/lib$(LIBNAME).so
