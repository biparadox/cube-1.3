FLAGS= -g -o 
CFLAGS= -g -c -fPIC -Wall -DUSER_MODE -I$(CUBELIBPATH)/include
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= basefunc.o 
LIBNAME=basefunc
DEPENDLIB= -lalloc -lstring

main: ../lib/lib$(LIBNAME).so  ../lib/lib$(LIBNAME).a test_main
test_main: test_main.c ../lib/lib$(LIBNAME).so
	$(CC) $(FLAGS) test_main test_main.c -I$(CUBELIBPATH)/include -L../lib $(DEPENDLIB) -l$(LIBNAME)  
basefunc.o:  basefunc.c ../include/basefunc.h
	$(CC) $(CFLAGS) basefunc.c
../lib/lib$(LIBNAME).so: $(STRUCTOBJ)
	$(CC) -shared -o lib$(LIBNAME).so $(STRUCTOBJ)
	mv   lib$(LIBNAME).so ../lib
../lib/lib$(LIBNAME).a: $(STRUCTOBJ)
	ar -rc lib$(LIBNAME).a $(STRUCTOBJ)
	mv   lib$(LIBNAME).a ../lib
clean:
	rm -f test_main *~ *.a *.o ../lib/lib$(LIBNAME).so
