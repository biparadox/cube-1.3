FLAGS= -g -o
CFLAGS= -g -c -fPIC -Wall -DUSER_MODE 
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= json.o
LIBNAME=json
DEPENDLIB= -lalloc -lstring

main: ../lib/lib$(LIBNAME).a  test_main
test_main: test_main.c ../lib/lib$(LIBNAME).a
	$(CC) $(FLAGS) test_main test_main.c -L../lib -l$(LIBNAME) $(DEPENDLIB) 
../lib/lib$(LIBNAME).a: $(STRUCTOBJ)
	ar rs lib$(LIBNAME).a $(STRUCTOBJ)
	mv    lib$(LIBNAME).a ../lib
json.o:  json.c 
	$(CC) $(CFLAGS) json.c
clean:
	rm -f test_main *~ *.a *.o ../lib/$(LIBNAME).a
