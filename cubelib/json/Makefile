FLAGS= -g -o
CFLAGS= -g -c -DUSER_MODE 
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= json.o

main: ../lib/libjson.a  test_main
test_main: test_main.c ../lib/libjson.a 
	$(CC) $(FLAGS) test_main test_main.c -L../lib -ljson -lalloc -lstring 
../lib/libjson.a: $(STRUCTOBJ)
	ar rs libjson.a $(STRUCTOBJ)
	mv    libjson.a ../lib
json.o:  json.c 
	$(CC) $(CFLAGS) json.c
clean:
	rm -f *~ *.a *.o 
