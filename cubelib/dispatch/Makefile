FLAGS= -g -o
CFLAGS= -g -c -DUSER_MODE 
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= dispatch.o 

main: ../lib/libdispatch.a  test_main
test_main: test_main.c ../lib/libdispatch.a  
	$(CC) $(FLAGS) test_main test_main.c -L../lib -ldispatch -lmessage -lmemdb -lbasefunc -lcrypto_func -lstruct -ljson -lalloc -lstring -lpthread
../lib/libdispatch.a: $(STRUCTOBJ)
	ar rs libdispatch.a $(STRUCTOBJ)
	mv    libdispatch.a ../lib
dispatch.o:  dispatch.c ../include/dispatch.h  dispatch_struct.h
	$(CC) $(CFLAGS) dispatch.c
clean:
	rm -f *~ *.a *.o test_main
