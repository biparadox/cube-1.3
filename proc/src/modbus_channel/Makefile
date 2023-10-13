FLAGS= -g -shared -o
INCLUDE= -I$(CUBESYSPATH)/include/ -I$(CUBELIBPATH)/include -I$(CUBEAPPPATH)/include 
CFLAGS= -g -c -fPIC -DUSER_MODE $(INCLUDE)
CC=gcc
CCP=g++
PLUGIN=modbus_channel
PLUGIN_OBJ=
TEST_LIB= -L$(CUBELIBPATH)/lib -lexmodule -lmessage -lchannel -lmemdb -lconnector -lstruct -lcrypto_func -lpthread
INIT_DB_OBJ = -L$(CUBEAPPPATH)/locallib/bin -lzuc

main: lib$(PLUGIN).so
lib$(PLUGIN).so:  $(PLUGIN).o $(PLUGIN_PROC_OBJ)
	$(CC) $(FLAGS) lib$(PLUGIN).so $(PLUGIN).o $(PLUGIN_OBJ) $(TEST_LIB) $(INIT_DB_OBJ)
	mv lib$(PLUGIN).so $(CUBEAPPPATH)/plugin
	cp $(PLUGIN).cfg $(CUBEAPPPATH)/plugin
$(PLUGIN).o: $(PLUGIN).c $(PLUGIN).h
	$(CC) $(CFLAGS) $(PLUGIN).c
clean:
	rm -f test_main lib$(PLUGIN).so 
	rm  *.o *~
