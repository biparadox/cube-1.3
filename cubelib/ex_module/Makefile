FLAGS= -g -o 
CFLAGS= -g -c -fPIC -DUSER_MODE -I../include
CC=gcc
CCP=g++
STRUCTOBJ= ex_module.o sec_msgqueue.o ex_module_slot.o
LIBNAME=exmodule
DEPENDLIB= -lmessage -lmemdb -lbasefunc -lcrypto_func -lstruct -ljson -lalloc -lstring -lpthread

main: ../lib/lib$(LIBNAME).so  ../lib/lib$(LIBNAME).a test_main
test_main: test_main.c ../lib/lib$(LIBNAME).so 
	$(CC) $(FLAGS) test_main test_main.c -L../lib -l$(LIBNAME) $(DEPENDLIB)   
../lib/lib$(LIBNAME).so: $(STRUCTOBJ)
	$(CC) -shared -o lib$(LIBNAME).so $(STRUCTOBJ)
	mv   lib$(LIBNAME).so ../lib
../lib/lib$(LIBNAME).a: $(STRUCTOBJ)
	ar -rc lib$(LIBNAME).a $(STRUCTOBJ)
	mv   lib$(LIBNAME).a ../lib
ex_module.o: ex_module.c  
	$(CC) $(CFLAGS) ex_module.c
sec_msgqueue.o:  sec_msgqueue.c 
	$(CC) $(CFLAGS) sec_msgqueue.c 
ex_module_slot.o:  ex_module_slot.c 
	$(CC) $(CFLAGS) ex_module_slot.c 
clean:
	rm -f test_main  *~ *.o *.a ../lib/lib$(LIBNAME).so
