#ifndef CONN_STRUCT_H
#define CONN_STRUCT_H

struct connect_ack
{
	char uuid[DIGEST_SIZE];    //client's uuid
	char * client_name;	     // this client's name
	char * client_process;       // this client's process
	char * client_addr;          // client's address
	char server_uuid[DIGEST_SIZE];  //server's uuid
	char * server_name;               //server's name
	char * service;
	char * server_addr;              // server's addr
	int flags;
	char nonce[DIGEST_SIZE];
} __attribute__((packed));

struct connect_syn
{
	char uuid[DIGEST_SIZE];
	char * server_name;
	char * service;
	char * server_addr;
	int  flags;
	char nonce[DIGEST_SIZE];
}__attribute__((packed));

#endif
