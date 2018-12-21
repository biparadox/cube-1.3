dirs= echo_plugin file_dealer hello recordlib symm_crypt sys_plugin tcp_channel tcp_client websocket_port websocket_channel echo_channel output_channel msgrecord_store msgfile_send msgrecord_print
all:
	$(foreach N,$(dirs),make -C $(N);)
clean:
	$(foreach N,$(dirs),make clean -C $(N);)
	rm -f *~
	rm -f lib/*.so


