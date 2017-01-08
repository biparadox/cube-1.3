FLAGS= -g -o 
INCLUDE= -I../include/
CFLAGS= -g -c -fPIC -DUSER_MODE $(INCLUDE)
CC=gcc
CCP=g++
TEST_LIB= -L../lib -lexmodule -ldispatch -lmessage -lmemdb -lconnector -lstruct -lcrypto_func -ljson -lbasefunc -lstring -lalloc -lpthread

main: main_proc 
main_proc:  main_proc.c main_proc_func.h
	$(CC) $(FLAGS) main_proc main_proc.c $(INCLUDE) $(TEST_LIB) -ldl
clean:
	rm -f *~ *.o *.a  main_proc
