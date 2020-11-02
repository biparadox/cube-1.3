FLAGS= -g -o
INCLUDE= -I../include/ -I$(CUBELIBPATH)/include
CFLAGS= -g -c -fPIC -DUSER_MODE $(INCLUDE)
CC=gcc
CC=gcc
CCP=g++
STRUCTOBJ= cube_cafunc.o
TEST_LIB= -L$(CUBELIBPATH)/lib -ltesi -ltspi -lcrypto -lstruct -lcrypto_func -ljson -lstring -lalloc -lpthread

main: test_main ca_create_crt
test_main: test_main.c cube_cafunc.o
	$(CC) $(FLAGS) test_main test_main.c $(INCLUDE) $(STRUCTOBJ) $(TEST_LIB)
ca_create_crt: ca_create_crt.c cube_cafunc.o
	$(CC) $(FLAGS) ca_create_crt ca_create_crt.c $(INCLUDE) $(STRUCTOBJ) $(TEST_LIB)
cube_cafunc.o  :  cube_cafunc.c cube_cafunc.h
	$(CC) $(CFLAGS) cube_cafunc.c
clean:
	rm -f test_main *~ *.o *.a
