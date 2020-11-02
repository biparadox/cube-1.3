FLAGS= -g -o 
INCLUDE= -I$(CUBELIBPATH)/include
CFLAGS= -g -c -fPIC -DUSER_MODE $(INCLUDE)
CC=gcc
CCP=g++
STRUCTOBJ= read_config.o
DEPENDLIB= -L$(CUBELIBPATH)/lib -L$(CUBESYSPATH)/plugin -lsys_func -lchannel -lmessage -lmemdb -lconnector -lstruct -lcrypto_func -ljson -lbasefunc -lstring -lalloc 

main: shm_proc 
shm_proc:  shm_proc.c lib_rw.c main_proc_func.h $(STRUCTOBJ)
	$(CC) $(FLAGS) shm_proc shm_proc.c lib_rw.c $(STRUCTOBJ) $(INCLUDE) $(DEPENDLIB) -ldl
read_config.o:  read_config.c main_proc_func.h
	$(CC) $(CFLAGS) read_config.c

clean:
	rm -f *~ *.o *.a  shm_proc
