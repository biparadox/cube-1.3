FLAGS= -g -o
CFLAGS= -g -c -fPIC -Wall -DUSER_MODE 
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= memfunc.o 
LIBNAME=string
DEPENDLIB=

main: ../lib/lib$(LIBNAME).so  ../lib/lib$(LIBNAME).a test_main
test_main: test_main.c ../lib/lib$(LIBNAME).so
	$(CC) $(FLAGS) test_main test_main.c -L../lib $(DEPENDLIB) -l$(LIBNAME)  
../lib/lib$(LIBNAME).so: $(STRUCTOBJ)
	$(CC) -shared -o lib$(LIBNAME).so $(STRUCTOBJ)
	mv   lib$(LIBNAME).so ../lib
../lib/lib$(LIBNAME).a: $(STRUCTOBJ)
	ar -rc lib$(LIBNAME).a $(STRUCTOBJ)
	mv   lib$(LIBNAME).a ../lib
memfunc.o:  memfunc.c 
	$(CC) $(CFLAGS) memfunc.c
clean:
	rm -f test_main *~ *.a *.o ../lib/$(LIBNAME).so
