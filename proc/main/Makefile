FLAGS= -g -o 
INCLUDE= -I$(CUBELIBPATH)/include
CFLAGS= -g -c -fPIC -DUSER_MODE $(INCLUDE)
CC=gcc
CCP=g++
STRUCTOBJ=read_config.o
DEPENDLIB= -L$(CUBELIBPATH)/lib -L$(CUBESYSPATH)/plugin -lsys_func -lexmodule -ldispatch -lchannel -lmessage -lmemdb -lconnector -lstruct -lcrypto_func -ljson -lbasefunc -lstring -lalloc -lpthread
EXECLIB= -L$(CUBELIBPATH)/lib -lpthread -Wl,-Bstatic -lsys_func -lexmodule -ldispatch -lmessage -lmemdb -lstruct -lcrypto_func -ljson -lbasefunc -lstring -lalloc -Wl,-Bdynamic

main: main_proc lib_tool cube_exec
main_proc:  main_proc.c main_proc_func.h $(STRUCTOBJ)
	$(CC) $(FLAGS) main_proc main_proc.c $(STRUCTOBJ) $(INCLUDE) $(DEPENDLIB) -ldl
lib_tool:  lib_tool.c lib_rw.c main_proc_func.h $(STRUCTOBJ)
	$(CC) $(FLAGS) lib_tool lib_tool.c lib_rw.c $(STRUCTOBJ) $(INCLUDE) $(DEPENDLIB) -ldl
cube_exec:  cube_exec.c $(STRUCTOBJ)
	$(CC) $(FLAGS) cube_exec cube_exec.c $(INCLUDE) $(EXECLIB) 
read_config.o:  read_config.c main_proc_func.h
	$(CC) $(CFLAGS) read_config.c

clean:
	rm -f *~ *.o *.a  main_proc lib_tool
