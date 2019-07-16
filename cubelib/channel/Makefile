FLAGS= -g -o
CFLAGS= -g -c -fPIC -Wall -DUSER_MODE 
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= channel_buf.o  channel.o
LIBNAME=channel
DEPENDLIB= -lchannel -lmemdb -lbasefunc -lstruct -lcrypto_func -ljson -lalloc -lstring

main: ../lib/lib$(LIBNAME).so  ../lib/lib$(LIBNAME).a test_main fixmem_channel
test_main: test_main.c ../lib/lib$(LIBNAME).so
	$(CC) $(FLAGS) test_main test_main.c -L../lib $(DEPENDLIB) -l$(LIBNAME)  
fixmem_channel: fixmem_channel.c ../lib/lib$(LIBNAME).so
	$(CC) $(FLAGS) fixmem_channel fixmem_channel.c -L../lib $(DEPENDLIB) -l$(LIBNAME)  
../lib/lib$(LIBNAME).so: $(STRUCTOBJ)
	$(CC) -shared -o lib$(LIBNAME).so $(STRUCTOBJ)
	mv   lib$(LIBNAME).so ../lib
../lib/lib$(LIBNAME).a: $(STRUCTOBJ)
	ar -rc lib$(LIBNAME).a $(STRUCTOBJ)
	mv   lib$(LIBNAME).a ../lib
channel_buf.o:  channel_buf.c ../include/channel.h 
	$(CC) $(CFLAGS) channel_buf.c
channel.o:  channel.c ../include/channel.h 
	$(CC) $(CFLAGS) channel.c
clean:
	rm -f *~ *.a *.o test_main
