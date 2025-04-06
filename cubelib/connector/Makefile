FLAGS= -g -o
CFLAGS= -g -c -fPIC -Wall -DUSER_MODE -I../include
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= connector.o connector_inet.o connector_hub.o connect_message.o
LIBNAME=connector
DEPENDLIB= -lsys_func -lmessage -lmemdb -lbasefunc -lcrypto_func -lstruct -ljson -lalloc -lstring

main: ../lib/lib$(LIBNAME).so ../lib/lib$(LIBNAME).a
test_main: test_main.c ../lib/lib$(LIBNAME).so 
	$(CC) $(FLAGS) test_main test_main.c -L../lib -l$(LIBNAME) $(DEPENDLIB)   
test_srv:  test_srv.c $(FUNC_OBJ) ../lib/libmemdb.a ../lib/libstruct.a ../lib/libconnector.a
	$(CC) $(FLAGS) test_srv test_srv.c -L../lib -lmemdb -lstruct -lconnector
test_client1:  test_client1.c $(FUNC_OBJ) ../lib/libconnector.a ../lib/libmemdb.a ../lib/libstruct.a
	$(CC) $(FLAGS) test_client1 test_client1.c -L../lib -lmemdb -lstruct -lconnector
test_client:  test_client.c $(FUNC_OBJ) ../lib/libconnector.a ../lib/libmemdb.a ../lib/libstruct.a
	$(CC) $(FLAGS) test_client test_client.c -L../lib -lmemdb -lstruct -lconnector
../lib/lib$(LIBNAME).so: $(STRUCTOBJ)
	$(CC) -shared -o lib$(LIBNAME).so $(STRUCTOBJ)
	mv   lib$(LIBNAME).so ../lib
../lib/lib$(LIBNAME).a: $(STRUCTOBJ)
	ar -rc lib$(LIBNAME).a $(STRUCTOBJ)
	mv   lib$(LIBNAME).a ../lib
connector.o: connector.c  
	$(CC) $(CFLAGS) connector.c
connector_unix.o: connector_unix.c  
	$(CC) $(CFLAGS) connector_unix.c
connector_inet.o: connector_inet.c  
	$(CC) $(CFLAGS) connector_inet.c
connector_hub.o: connector_hub.c  
	$(CC) $(CFLAGS) connector_hub.c
connect_message.o: connect_message.c  
	$(CC) $(CFLAGS) connect_message.c
clean:
	rm -f test_srv test_client* *~ *.o *.a ../lib/lib$(LIBNAME).so
