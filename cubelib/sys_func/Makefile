FLAGS= -g -o
CFLAGS= -g -c -fPIC -Wall -DUSER_MODE -I../include
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= read_config.o
LIBNAME=sys_func
DEPENDLIB= -lmessage -lmemdb -lstruct -lbasefunc -lcrypto_func -ljson -lalloc -lstring 

main: ../lib/lib$(LIBNAME).so  ../lib/lib$(LIBNAME).a test_main
test_main: test_main.c ../lib/lib$(LIBNAME).so
	$(CC) $(FLAGS) test_main test_main.c -L../lib -l$(LIBNAME) $(DEPENDLIB) 
../lib/lib$(LIBNAME).so: $(STRUCTOBJ)
	$(CC) -shared -o lib$(LIBNAME).so $(STRUCTOBJ)
	mv   lib$(LIBNAME).so ../lib
../lib/lib$(LIBNAME).a: $(STRUCTOBJ)
	ar -rc lib$(LIBNAME).a $(STRUCTOBJ)
	mv   lib$(LIBNAME).a ../lib
read_config.o:  read_config.c 
	$(CC) $(CFLAGS) read_config.c
clean:
	rm -f test_main *~ *.a *.o ../lib/$(LIBNAME).so
