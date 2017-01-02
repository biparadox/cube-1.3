FLAGS= -g -o
CFLAGS= -g -c -fPIC -Wall -DUSER_MODE 
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= basefunc.o 
LIBNAME=basefunc
DEPENDLIB= -lalloc -lstring

main: ../lib/lib$(LIBNAME).a  test_main ../lib/lib$(LIBNAME).so
test_main: test_main.c ../lib/lib$(LIBNAME).a ../lib/lib$(LIBNAME).so
	$(CC) $(FLAGS) test_main test_main.c -L../lib $(DEPENDLIB) -l$(LIBNAME)  
basefunc.o:  basefunc.c ../include/basefunc.h
	$(CC) $(CFLAGS) basefunc.c
../lib/lib$(LIBNAME).a: $(STRUCTOBJ)
	ar rs lib$(LIBNAME).a $(STRUCTOBJ)
	mv    lib$(LIBNAME).a ../lib
../lib/lib$(LIBNAME).so:  $(STRUCTOBJ)
	ar cr lib$(LIBNAME).so $(STRUCTOBJ) 
	mv    lib$(LIBNAME).so ../lib
clean:
	rm -f test_main *~ *.a *.o ../lib/$(LIBNAME).a ../lib/lib$(LIBNAME).so
