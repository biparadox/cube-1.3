FLAGS= -g -shared -o
INCLUDE= -I../include/ -I$(CUBELIBPATH)/include
CFLAGS= -g -c -fPIC -DUSER_MODE $(INCLUDE)
CC=gcc
CCP=g++
PLUGIN=msg_broadcast
PLUGIN_OBJ=
TEST_LIB= -L$(CUBELIBPATH)/lib -lmessage -lmemdb -lconnector -lstruct -lcrypto_func -lpthread

main: lib$(PLUGIN).so
lib$(PLUGIN).so:  $(PLUGIN).o $(PLUGIN_PROC_OBJ) $(INIT_DB_OBJ)
	$(CC) $(FLAGS) lib$(PLUGIN).so $(PLUGIN).o $(PLUGIN_OBJ) $(TEST_LIB)
	mv lib$(PLUGIN).so $(CUBE_SYS_PLUGIN)
	cp $(PLUGIN).cfg $(CUBE_SYS_PLUGIN)
$(PLUGIN).o: $(PLUGIN).c $(PLUGIN).h
	$(CC) $(CFLAGS) $(PLUGIN).c
clean:
	rm -f test_main lib$(PLUGIN).so 
	rm  *.o *~
