FLAGS= -g -o 
CFLAGS= -g -c -fPIC -DUSER_MOD
CC=gcc
CCP=g++
STRUCTOBJ= common.o tesi_certs.o key_certify.o
LIBNAME=tesi
DEPENDLIB= -lmessage -lmemdb -lbasefunc -lcrypto_func -lstruct -ljson -lalloc -lstring -lcrypto -ltspi

main: ../lib/lib$(LIBNAME).so  test_main
test_main: test_main.c ../lib/lib$(LIBNAME).so 
	$(CC) $(FLAGS) test_main test_main.c -L../lib -l$(LIBNAME) $(DEPENDLIB)   
../lib/lib$(LIBNAME).so: $(STRUCTOBJ)
	$(CC) -shared -o lib$(LIBNAME).so $(STRUCTOBJ)
	mv   lib$(LIBNAME).so ../lib
common.o: common.c common.h
	$(CC) $(CFLAGS) common.c
tesi_certs.o: tesi_certs.c 
	$(CC) $(CFLAGS) tesi_certs.c
key_certify.o: key_certify.c 
	$(CC) $(CFLAGS) key_certify.c
clean:
	rm -f *~ *.o *.a
