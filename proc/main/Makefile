FLAGS= -g -shared -o 
INCLUDE= -I../include/
CFLAGS= -g -c -fPIC -DUSER_MODE $(INCLUDE)
CC=gcc
CCP=g++
TEST_LIB= -L../lib -lexmodule -lmessage -lmemdb -lconnector -lbasefunc -lstruct -ljson -lcrypto_func -lstring -lalloc -lpthread

main: libconnector_process_func.so  
#main_proc:  main_proc.c main_proc_func.h
#	$(CC) $(FLAGS) main_proc main_proc.c $(TEST_LIB) -ldl
libconnector_process_func.so: connector_process_func.o connector_process_func.h
	$(CC) $(FLAGS) libconnector_process_func.so connector_process_func.o $(TEST_LIB)
	mv libconnector_process_func.so ../plugin
librouter_process_func.so: router_process_func.o router_process_func.h
	$(CC) $(FLAGS) librouter_process_func.so router_process_func.o $(TEST_LIB)
	mv librouter_process_func.so ../plugin
connector_process_func.o: connector_process_func.c connector_process_func.h
	$(CC) $(CFLAGS) connector_process_func.c 
router_process_func.o: router_process_func.c router_process_func.h
	$(CC) $(CFLAGS) router_process_func.c
libtest: libtest.c
	$(CC)  $(FLAGS) libtest libtest.c -L../lib $(TEST_LIB)
clean:
	rm -f *~ *.o *.a *.so
