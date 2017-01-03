FLAGS= -g -o
CFLAGS= -g -c -fPIC -Wall -DUSER_MODE 
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= alloc.o 

main: ../lib/liballoc.a  test_main
test_main: test_main.c ../lib/liballoc.a
	$(CC) $(FLAGS) test_main test_main.c -L../lib -lalloc -lstring
../lib/liballoc.a: $(STRUCTOBJ)
	ar rs liballoc.a $(STRUCTOBJ)
	mv    liballoc.a ../lib
alloc.o:  alloc.c ../include/alloc.h
	$(CC) $(CFLAGS) alloc.c
clean:
	rm -f test_main *~ *.a *.o ../lib/liballoc.a
