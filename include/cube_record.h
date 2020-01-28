#ifndef CUBE_RECORD_HEADER
#define CUBE_RECORD_HEADER

// memdb record start

// memdb record end
//
// message record start
typedef MSG_HEAD RECORD(MESSAGE,HEAD);
typedef MSG_EXPAND_HEAD RECORD(MESSAGE,EXPAND_HEAD);
typedef MSG_EXPAND RECORD(MESSAGE,MSG_EXPAND);

typedef struct basic_message  // record (MESSAGE,BASIC_MSG)
{
	char * message;
}__attribute__((packed))
RECORD(MESSAGE,BASE_MSG);

typedef struct basic_number  // record (MESSAGE,BASIC_MSG)
{
	int number;
}__attribute__((packed))
RECORD(MESSAGE,NUMBER);

typedef struct record_instance_info // record (MESSAGE,INSTANCE_INFO)
{
	char proc_name[DIGEST_SIZE];
	BYTE node_uuid[DIGEST_SIZE];
	char user_name[DIGEST_SIZE];
}__attribute__((packed))
RECORD(MESSAGE,INSTANCE_INFO);

typedef struct record_string_array // record (MESSAGE,STRING_ARRAY)
{
	int num;
	BYTE *strings;// string length is 32
}__attribute__((packed))
RECORD(MESSAGE,STRING_ARRAY);

typedef struct uuid_record    // record (MESSAGE,UUID_RECORD)
{
	BYTE uuid[DIGEST_SIZE];

}__attribute__((packed))
RECORD(MESSAGE,UUID_RECORD);

typedef struct sized_bindata    // record (MESSAGE,BINDATA)
{
	int size;
	BYTE * bindata;

}__attribute__((packed))
RECORD(MESSAGE,SIZED_BINDATA);

typedef struct module_state
{
	char name[DIGEST_SIZE];
	int state;
}__attribute__((packed))
RECORD(MESSAGE,MODULE_STATE);

typedef struct types_pair     // record (MESSAGE,TYPES)
{
	int type;
	int subtype;
}__attribute__((packed))
RECORD(MESSAGE,TYPES);

typedef struct ctrl_message     // record (MESSAGE,CTRL)
{
	enum message_ctrl ctrl;
	char * name;
}__attribute__((packed))
RECORD(MESSAGE,CTRL_MSG);

// message record end

// connect record start
typedef struct connect_ack
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
} __attribute__((packed))
RECORD(MESSAGE,CONN_ACKI);

typedef struct connect_syn
{
	char uuid[DIGEST_SIZE];
	char * server_name;
	char * service;
	char * server_addr;
	int  flags;
	char nonce[DIGEST_SIZE];
}__attribute__((packed))
RECORD(MESSAGE,CONN_SYNI);
// connect record end
#endif
