FLAGS= -g -o
CFLAGS= -g -c -fPIC -Wall -DUSER_MODE 
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= alloc.o 

main: ../lib/liballoc.a  test_main ../lib/liballoc.so
test_main: test_main.c ../lib/liballoc.a ../lib/liballoc.so
	$(CC) $(FLAGS) test_main test_main.c -L../lib -lalloc -lstring
../lib/liballoc.a: $(STRUCTOBJ)
	ar rs liballoc.a $(STRUCTOBJ)
	mv    liballoc.a ../lib
../lib/liballoc.so:  $(STRUCTOBJ)
	ar cr liballoc.so $(STRUCTOBJ) 
	mv    liballoc.so ../lib
alloc.o:  alloc.c ../include/alloc.h
	$(CC) $(CFLAGS) alloc.c
clean:
	rm -f test_main *~ *.a *.o ../lib/liballoc.a ../lib/liballoc.so
