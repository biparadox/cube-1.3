FLAGS= -g -o 
LIBFLAGS = -g -shared -o
INCLUDE= ../include/ 
CFLAGS= -g -c -fPIC -DUSER_MODE -I$(INCLUDE)
CC=gcc
CCP=g++
MAIN_PROC=example_proc
INIT_OBJ=lib$(MAIN_PROC).so
TEST_LIB= -L../lib -lexmodule -ldispatch -lmessage -lmemdb -lconnector -lstruct -lcrypto_func -ljson -lbasefunc -lstring -lalloc -lpthread

main: $(INIT_OBJ)
$(INIT_OBJ): $(MAIN_PROC)_func.o
	$(CC) $(LIBFLAGS) $(INIT_OBJ) $(MAIN_PROC)_func.o
$(MAIN_PROC)_func.o: $(MAIN_PROC)_func.c
	$(CC) $(CFLAGS) $(MAIN_PROC)_func.c
clean:
	rm -f *~ *.o *.a $(INIT_OBJ)
