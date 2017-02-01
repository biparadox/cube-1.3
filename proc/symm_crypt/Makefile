FLAGS= -g -shared -o
INCLUDE= -I../include/ 
CFLAGS= -g -c -fPIC -DUSER_MODE $(INCLUDE)
CC=gcc
CCP=g++
PLUGIN=symm_crypt
PLUGIN_OBJ= sm4.o
TEST_LIB= -L../lib -lexmodule -lmessage -lmemdb -lconnector -lstruct -lcrypto_func -lpthread 

main: lib$(PLUGIN).so 
lib$(PLUGIN).so:  $(PLUGIN).o $(PLUGIN_OBJ) 
	$(CC) $(FLAGS) lib$(PLUGIN).so $(PLUGIN).o $(PLUGIN_OBJ) $(TEST_LIB)
	mv lib$(PLUGIN).so $(CUBE_SYS_PLUGIN)
	cp $(PLUGIN).cfg $(CUBE_SYS_PLUGIN)
$(PLUGIN).o: $(PLUGIN).c $(PLUGIN).h
	$(CC) $(CFLAGS) $(PLUGIN).c
sm4.o:	sm4.c sm4.h
	$(CC) $(CFLAGS) sm4.c
clean:
	rm -f test_main lib$(PLUGIN).so 
	rm  *.o *~
