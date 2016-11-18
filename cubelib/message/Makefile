FLAGS= -g -o
CFLAGS= -g -c -DUSER_MODE 
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= message.o json_message.o

main: ../lib/libmessage.a  test_main
test_main: test_main.c ../lib/libmessage.a 
	$(CC) $(FLAGS) test_main test_main.c -L../lib -lmessage -lmemdb -lbasefunc -lcrypto_func -lstruct -ljson -lalloc -lstring 
../lib/libmessage.a: $(STRUCTOBJ)
	ar rs libmessage.a $(STRUCTOBJ)
	mv    libmessage.a ../lib
message.o:  message.c ../include/message.h 
	$(CC) $(CFLAGS) message.c
json_message.o:  json_message.c ../include/message.h 
	$(CC) $(CFLAGS) json_message.c
clean:
	rm -f *~ *.a *.o test_main
