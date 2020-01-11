FLAGS= -g -o 
CUBELIBPATH=../../cubelib
INCLUDE= -I$(CUBELIBPATH)/include
CFLAGS= -g -c -fPIC -DUSER_MODE $(INCLUDE)
CC=gcc
CCP=g++
STRUCTOBJ=read_config.o
DEPENDLIB= -L$(CUBELIBPATH)/lib -lsys_func -lexmodule -ldispatch -lchannel -lmessage -lmemdb -lconnector -lstruct -lcrypto_func -ljson -lbasefunc -lstring -lalloc -lpthread
EXECLIB= -L$(CUBELIBPATH)/lib -lpthread -Wl,-Bstatic -lsys_func -lexmodule -ldispatch -lchannel -lmessage -lmemdb -lconnector -lstruct -lcrypto_func -ljson -lbasefunc -lstring -lalloc -Wl,-Bdynamic

main: main_proc lib_tool cube_exec envset_proc
main_proc:  main_proc.c main_proc_func.h $(STRUCTOBJ)
	$(CC) $(FLAGS) main_proc main_proc.c $(STRUCTOBJ) $(INCLUDE) $(DEPENDLIB) -ldl
lib_tool:  lib_tool.c lib_rw.c main_proc_func.h $(STRUCTOBJ)
	$(CC) $(FLAGS) lib_tool lib_tool.c lib_rw.c $(STRUCTOBJ) $(INCLUDE) $(DEPENDLIB) -ldl
cube_exec:  cube_exec.c $(STRUCTOBJ)
	$(CC) $(FLAGS) cube_exec cube_exec.c $(INCLUDE) $(EXECLIB) 
envset_proc:  envset_proc.c $(STRUCTOBJ) envset_func.o
	$(CC) $(FLAGS) envset_proc envset_proc.c $(STRUCTOBJ) $(INCLUDE) envset_func.o $(DEPENDLIB) -ldl
read_config.o:  read_config.c main_proc_func.h
	$(CC) $(CFLAGS) read_config.c
envset_func.o:  envset_func.c
	$(CC) $(CFLAGS) envset_func.c

clean:
	rm -f *~ *.o *.a  main_proc lib_tool
