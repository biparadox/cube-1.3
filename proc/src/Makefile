dirs= aik_casign aik_client echo_plugin file_dealer hello recordlib symm_crypt sys_plugin tpm_key_create websocket_port
all:
	$(foreach N,$(dirs),make -C $(N);)
clean:
	$(foreach N,$(dirs),make clean -C $(N);)
	rm -f *~
	rm -f lib/*.so


