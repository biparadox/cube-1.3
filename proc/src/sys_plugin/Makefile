FLAGS= -g -shared -o 
INCLUDE= -I$(CUBESYSPATH)/include/ -I$(CUBELIBPATH)/include 
CFLAGS= -g -c -fPIC -DUSER_MODE $(INCLUDE)
CC=gcc
CCP=g++
TEST_LIB= -L$(CUBELIBPATH)/lib -lsys_func -lexmodule -ldispatch -lmessage -lmemdb -lconnector -lstruct -lcrypto_func -ljson -lbasefunc -lstring -lalloc -lpthread 

main: libconnector_process_func.so librouter_process_func.so 
libconnector_process_func.so: connector_process_func.o connector_process_func.h
	$(CC) $(FLAGS) libconnector_process_func.so connector_process_func.o $(TEST_LIB)
	mv libconnector_process_func.so $(CUBE_SYS_PLUGIN)/
librouter_process_func.so: router_process_func.o router_process_func.h
	$(CC) $(FLAGS) librouter_process_func.so router_process_func.o $(TEST_LIB)
	mv librouter_process_func.so $(CUBE_SYS_PLUGIN)/
connector_process_func.o: connector_process_func.c connector_process_func.h
	$(CC) $(CFLAGS) connector_process_func.c 
router_process_func.o: router_process_func.c router_process_func.h
	$(CC) $(CFLAGS) router_process_func.c
clean:
	rm -f *~ *.o *.a *.so
