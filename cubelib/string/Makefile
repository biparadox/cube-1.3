FLAGS= -g -o
CFLAGS= -g -c -Wall -DUSER_MODE 
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= memfunc.o 

main: ../lib/libstring.a  test_main
test_main: test_main.c ../lib/libstring.a 
	$(CC) $(FLAGS) test_main test_main.c -L../lib -lstring  
../lib/libstring.a: $(STRUCTOBJ)
	ar rs libstring.a $(STRUCTOBJ)
	mv    libstring.a ../lib
memfunc.o:  memfunc.c ../include/string.h
	$(CC) $(CFLAGS) memfunc.c
clean:
	rm -f test_main *~ *.a *.o ../lib/libstring.a
