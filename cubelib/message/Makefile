FLAGS= -g -o
FLAGS= -g -o 
CFLAGS= -g -c -fPIC -DUSER_MODE 
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= message.o json_message.o
LIBNAME=message
DEPENDLIB= -lmessage -lmemdb -lbasefunc -lcrypto_func -lstruct -ljson -lalloc -lstring

main: ../lib/lib$(LIBNAME).so  ../lib/lib$(LIBNAME).a test_main
test_main: test_main.c ../lib/lib$(LIBNAME).so 
	$(CC) $(FLAGS) test_main test_main.c -L../lib -l$(LIBNAME) $(DEPENDLIB)   
../lib/lib$(LIBNAME).so: $(STRUCTOBJ)
	$(CC) -shared -o lib$(LIBNAME).so $(STRUCTOBJ)
	mv   lib$(LIBNAME).so ../lib
../lib/lib$(LIBNAME).a: $(STRUCTOBJ)
	ar -rc lib$(LIBNAME).a $(STRUCTOBJ)
	mv   lib$(LIBNAME).a ../lib
message.o:  message.c ../include/message.h 
	$(CC) $(CFLAGS) message.c
json_message.o:  json_message.c ../include/message.h 
	$(CC) $(CFLAGS) json_message.c
clean:
	rm -f *~ *.a *.o test_main ../lib/lib$(LIBNAME).so
