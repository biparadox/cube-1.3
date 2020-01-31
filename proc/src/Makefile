CUBELIBPATH= ../../../cubelib
CUBE_SYS_PATH= ../../
dirs= echo_plugin file_dealer file_channel hello recordlib symm_crypt sm4crypt sys_plugin tcp_channel tcp_client websocket_channel echo_channel output_channel msgrecord_store msgfile_send msgrecord_print msg2channel term_io msgtype_get
all:
	$(foreach N,$(dirs),make -C $(N);)
clean:
	$(foreach N,$(dirs),make clean -C $(N);)
	rm -f *~
	rm -f lib/*.so


