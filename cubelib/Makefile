dirs= string alloc basefunc json crypto struct_mod memdb message dispatch ex_module connector
all:
	$(foreach N,$(dirs),make -C $(N);)
clean:
	$(foreach N,$(dirs),make clean -C $(N);)
	rm -f *~
	rm -f lib/*.so


