FLAGS= -g -shared -o
INCLUDE= -I$(CUBELIBPATH)/include/ -I$(CUBESYSPATH)/include 
CFLAGS= -g -c -fPIC -DUSER_MODE $(INCLUDE)
CC=gcc
CCP=g++
PLUGIN=sm4crypt
PLUGIN_OBJ= 
TEST_LIB= -L$(CUBELIBPATH)/lib -lexmodule -lmessage -lmemdb -lconnector -lstruct -lcrypto_func -lpthread 

main: lib$(PLUGIN).so 
lib$(PLUGIN).so:  $(PLUGIN).o $(PLUGIN_OBJ) 
	$(CC) $(FLAGS) lib$(PLUGIN).so $(PLUGIN).o $(PLUGIN_OBJ) $(TEST_LIB)
	mv lib$(PLUGIN).so $(CUBESYSPATH)/plugin
	cp $(PLUGIN).cfg $(CUBESYSPATH)/plugin
$(PLUGIN).o: $(PLUGIN).c $(PLUGIN).h
	$(CC) $(CFLAGS) $(PLUGIN).c
clean:
	rm -f test_main lib$(PLUGIN).so 
	rm  *.o *~
