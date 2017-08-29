FLAGS= -g -o 
INCLUDE= -I$(CUBELIBPATH)/include
CFLAGS= -g -c -fPIC -DUSER_MODE $(INCLUDE)
CC=gcc
CCP=g++
STRUCTOBJ= read_config.o
DEPENDLIB= -L$(CUBELIBPATH)/lib -lexmodule -ldispatch -lmessage -lmemdb -lconnector -lstruct -lcrypto_func -ljson -lbasefunc -lstring -lalloc -lpthread

main: main_proc lib_tool 
main_proc:  main_proc.c main_proc_func.h $(STRUCTOBJ)
	$(CC) $(FLAGS) main_proc main_proc.c $(STRUCTOBJ) $(INCLUDE) $(DEPENDLIB) -ldl
lib_tool:  lib_tool.c main_proc_func.h $(STRUCTOBJ)
	$(CC) $(FLAGS) lib_tool lib_tool.c $(STRUCTOBJ) $(INCLUDE) $(DEPENDLIB) -ldl
read_config.o:  read_config.c main_proc_func.h
	$(CC) $(CFLAGS) read_config.c

clean:
	rm -f *~ *.o *.a  main_proc lib_tool
