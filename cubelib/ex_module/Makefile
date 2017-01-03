FLAGS= -g -o 
CFLAGS= -g -c -fPIC -DUSER_MODE 
CC=gcc
CCP=g++
STRUCTOBJ= ex_module.o sec_msgqueue.o
LIBNAME=exmodule
DEPENDLIB= -lmessage -lmemdb -lbasefunc -lcrypto_func -lstruct -ljson -lalloc -lstring -lpthread

main: ../lib/lib$(LIBNAME).so  test_main
test_main: test_main.c ../lib/lib$(LIBNAME).so 
	$(CC) $(FLAGS) test_main test_main.c -L../lib -l$(LIBNAME) $(DEPENDLIB)   
../lib/lib$(LIBNAME).so: $(STRUCTOBJ)
	$(CC) -shared -o lib$(LIBNAME).so $(STRUCTOBJ)
	mv   lib$(LIBNAME).so ../lib
ex_module.o: ex_module.c  
	$(CC) $(CFLAGS) ex_module.c
sec_msgqueue.o:  sec_msgqueue.c ../include/sec_entity.h
	$(CC) $(CFLAGS) sec_msgqueue.c 
clean:
	rm -f test_main  *~ *.o *.a ../lib/lib$(LIBNAME).so
