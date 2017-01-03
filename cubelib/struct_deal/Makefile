FLAGS= -g -o
CFLAGS= -g -c -fPIC -Wall -DUSER_MODE 
CC=gcc
LD=ld
CCP=g++
STRUCTOBJ= struct_ops.o enum_flag_ops.o struct_deal.o struct_init.o
LIBNAME=struct
DEPENDLIB= -ljson -lalloc -lstring

main: ../lib/lib$(LIBNAME).a  test_main 
test_main: test_main.c ../lib/lib$(LIBNAME).a
	$(CC) $(FLAGS) test_main test_main.c -L../lib $(DEPENDLIB) -l$(LIBNAME)  
../lib/lib$(LIBNAME).a: $(STRUCTOBJ)
	ar rs lib$(LIBNAME).a $(STRUCTOBJ)
	mv    lib$(LIBNAME).a ../lib
struct_deal.o:  struct_deal.c ../include/struct_deal.h
	$(CC) $(CFLAGS) struct_deal.c
struct_ops.o:  struct_ops.c struct_ops.h
	$(CC) $(CFLAGS) struct_ops.c
struct_init.o:  struct_init.c struct_ops.h
	$(CC) $(CFLAGS) struct_init.c
enum_flag_ops.o:  enum_flag_ops.c struct_ops.h
	$(CC) $(CFLAGS) enum_flag_ops.c
clean:
	rm -f test_main *~ *.a *.o ../lib/$(LIBNAME).a 
