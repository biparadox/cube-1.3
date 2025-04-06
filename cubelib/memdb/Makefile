FLAGS= -g -o
CFLAGS= -g -c -fPIC -Wall -DUSER_MODE 
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= memdb_init.o type_subtype_ops.o memdb.o
LIBNAME=memdb
DEPENDLIB= -lbasefunc -lcrypto_func -lstruct -ljson -lalloc -lstring

main: ../lib/lib$(LIBNAME).so  ../lib/lib$(LIBNAME).a test_main
test_main: test_main.c ../lib/lib$(LIBNAME).so 
	$(CC) $(FLAGS) test_main test_main.c -L../lib -l$(LIBNAME) $(DEPENDLIB)   
../lib/lib$(LIBNAME).so: $(STRUCTOBJ)
	$(CC) -shared -o lib$(LIBNAME).so $(STRUCTOBJ)
	mv   lib$(LIBNAME).so ../lib
../lib/lib$(LIBNAME).a: $(STRUCTOBJ)
	ar -rc lib$(LIBNAME).a $(STRUCTOBJ)
	mv   lib$(LIBNAME).a ../lib
type_subtype_ops.o:  type_subtype_ops.c ../include/memdb.h memdb_internal.h
	$(CC) $(CFLAGS) type_subtype_ops.c
memdb_init.o:  memdb_init.c ../include/memdb.h memdb_internal.h
	$(CC) $(CFLAGS) memdb_init.c
memdb.o:  memdb.c ../include/memdb.h memdb_internal.h
	$(CC) $(CFLAGS) memdb.c
clean:
	rm -f test_main *~ *.a *.o ../lib/lib$(LIBNAME).so
