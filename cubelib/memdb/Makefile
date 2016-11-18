FLAGS= -g -o
CFLAGS= -g -c -DUSER_MODE 
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= memdb_init.o type_subtype_ops.o memdb.o

main: ../lib/libmemdb.a  test_main
test_main: test_main.c ../lib/libstruct.a 
	$(CC) $(FLAGS) test_main test_main.c -L../lib -lmemdb -lbasefunc -lcrypto_func -lstruct -ljson -lalloc -lstring 
../lib/libmemdb.a: $(STRUCTOBJ)
	ar rs libmemdb.a $(STRUCTOBJ)
	mv    libmemdb.a ../lib
type_subtype_ops.o:  type_subtype_ops.c ../include/memdb.h memdb_internal.h
	$(CC) $(CFLAGS) type_subtype_ops.c
memdb_init.o:  memdb_init.c ../include/memdb.h memdb_internal.h
	$(CC) $(CFLAGS) memdb_init.c
memdb.o:  memdb.c ../include/memdb.h memdb_internal.h
	$(CC) $(CFLAGS) memdb.c
clean:
	rm -f *~ *.a *.o test_main
