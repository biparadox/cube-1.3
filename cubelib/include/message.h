/*************************************************
*       project:        973 trust demo, zhongan secure os 
*                       and trust standard verify
*	name:		message_struct.h
*	write date:    	2011-08-04
*	auther:    	Hu jun
*       content:        this file describe the module's extern struct 
*       changelog:       
*************************************************/
#ifndef _CUBE_MESSAGE_H
#define _CUBE_MESSAGE_H

#include "data_type.h"

enum dynamic_message_typelist
{
	TYPE(MESSAGE)=0x200,
	TYPE(MSG_EXPAND),
};
enum subtypelist_message
{
	SUBTYPE_HEAD=0x01,
	SUBTYPE_EXPAND,
	SUBTYPE_EXPAND_HEAD,
	SUBTYPE_CONN_SYNI,
	SUBTYPE_CONN_ACKI,
	SUBTYPE_BASE_MSG,
	SUBTYPE_UUID_RECORD,
	SUBTYPE_CTRL_MSG,
	SUBTYPE_TYPES,
	SUBTYPE_SIZED_BINDATA,
	SUBTYPE_MODULE_STATE,
	SUBTYPE_NUMBER,
	SUBTYPE_INSTANCE_INFO,
	SUBTYPE_STRING_ARRAY
};

enum subtypelist_msg_expand
{
	SUBTYPE_FLOW_TRACE=0x01,
	SUBTYPE_ASPECT_POINT,
	SUBTYPE_ROUTE_RECORD
};

enum subtypelist_message_new
{
	SUBTYPE(MESSAGE,HEAD)=0x01,
	SUBTYPE(MESSAGE,EXPAND),
	SUBTYPE(MESSAGE,EXPAND_HEAD),
	SUBTYPE(MESSAGE,CONN_SYNI),
	SUBTYPE(MESSAGE,CONN_ACKI),
	SUBTYPE(MESSAGE,BASE_MSG),
	SUBTYPE(MESSAGE,UUID_RECORD),
	SUBTYPE(MESSAGE,CTRL_MSG),
	SUBTYPE(MESSAGE,TYPES),
	SUBTYPE(MESSAGE,SIZED_BINDATA),
	SUBTYPE(MESSAGE,MODULE_STATE),
	SUBTYPE(MESSAGE,NUMBER),
	SUBTYPE(MESSAGE,INSTANCE_INFO),
	SUBTYPE(MESSAGE,STRINGARRAY),
	SUBTYPE(MESSAGE,NODEFINE_HEAD)=0x100
};
// for message head's flow para
enum message_flow_type
{
    MSG_FLOW_INIT=0x01,
    MSG_FLOW_DELIVER=0x04,
    MSG_FLOW_QUERY=0x08,
    MSG_FLOW_RESPONSE=0x10,
    MSG_FLOW_DUP=0x20,
    MSG_FLOW_ASPECT=0x40,
    MSG_FLOW_FINISH=0x8000,
    MSG_FLOW_ERROR=0xFFFF,
};
// for message head's state para
enum message_state_type
{
	MSG_STATE_MATCH=0x01,
	MSG_STATE_SEND=0x02,
	MSG_STATE_RECEIVE=0x04,
	MSG_STATE_QUERY=0x20,
	MSG_STATE_RESPONSE=0x40,
    MSG_STATE_FINISH=0x8000,
    MSG_STATE_ERROR=0xFFFF,
};
enum message_flag
{
	MSG_FLAG_LOCAL=0x01,
	MSG_FLAG_RESPONSE=0x02,
	MSG_FLAG_CRYPT=0x10,
	MSG_FLAG_SIGN=0x20,
	MSG_FLAG_ZIP=0x40,
	MSG_FLAG_VERIFY=0x80,
	MSG_FLAG_FOLLOW=0x100,
};

enum message_ctrl
{
	MSG_CTRL_INIT=0x01,
	MSG_CTRL_START=0x02,
	MSG_CTRL_SLEEP=0x03,
	MSG_CTRL_RESUME=0x04,
	MSG_CTRL_STOP=0x05,
	MSG_CTRL_EXIT=0x06
};

typedef struct tagMessage_Head  //强制访问控制标记
{
   char tag[4];            // should be "MESG" to indicate that this information is a message's begin 
   int  version;          //  the message's version, now is 0x00010001
   char sender_uuid[DIGEST_SIZE];     // sender's uuid, or '@' followed with a name, or ':' followed with a connector's name
   char receiver_uuid[DIGEST_SIZE];   // receiver's uuid, or '@" followed with a name, or ':' followed with a connector's name
   char route[DIGEST_SIZE];
   int  flow;
   int  state;
   int  flag;
   int  ljump;
   int  rjump;
   int  record_type;
   int  record_subtype;
   int  record_num;
   int  record_size;
   int  expand_num;   
   int  expand_size;
   BYTE nonce[DIGEST_SIZE];
} __attribute__((packed)) MSG_HEAD;

typedef MSG_HEAD RECORD(MESSAGE,HEAD);

typedef struct tagMessage_Expand_Data_Head //general expand 's head struct
{
   int  data_size;   //this expand data's size
   int  type;
   int  subtype;      //expand data's type
} __attribute__((packed)) MSG_EXPAND_HEAD;

typedef MSG_EXPAND_HEAD RECORD(MESSAGE,EXPAND_HEAD);

typedef struct tagMessage_Expand_Data  //general expand data struct
{
   int  data_size;   //this expand data's size
   int  type;
   int  subtype;      //expand data's type
   void * expand;
} __attribute__((packed)) MSG_EXPAND;
typedef MSG_EXPAND RECORD(MESSAGE,MSG_EXPAND);

typedef struct expand_extra_info  //expand data struct to store one or more DIGEST_SIZE info
{
   int  data_size;   //this expand data's size
   char tag[4];      //expand data's type, 
   BYTE  uuid[DIGEST_SIZE];
} __attribute__((packed)) MSGEX_UUID;

int message_get_state(void * message);

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

void * message_init();
int message_record_init(void * message);
void message_free(void * message);
int message_free_blob(void * message);

void * message_set_activemsg(void * message,void * active_msg);
void * message_get_activemsg(void * message);
int message_get_type(void * message);
int message_get_subtype(void * message);
int message_get_uuid(void * message,BYTE * uuid);
const char * message_get_sender(void * message);
const char * message_get_receiver(void * message);
int message_set_sender(void * message,BYTE * sender);
int message_set_sender_uuid(void * message,BYTE * sender_uuid);
int message_set_receiver(void * message,BYTE * receiver);
int message_set_receiver_uuid(void * message,BYTE * receiver_uuid);
int message_set_policy(void * message,void * policy);
void * message_get_policy(void * message);

void * message_create(int type,int subtype,void * active_msg);
void * message_clone(void * message);
int  message_add_record(void * message,void * record);
int  message_add_record_blob(void * message,int record_size, BYTE * record);


int  message_add_expand(void * message,void * expand);
int  message_add_expand_data(void * message,int type,int subtype,void * expand);
int  message_add_expand_blob(void * message,int type,int subtype,void * expand);

int  message_remove_expand(void * message,int type,int subtype,void ** expand);
int  message_remove_indexed_expand(void * message,int index,void ** expand) ;

int  message_get_record(void * message, void ** msg_record,int record_no);
int  message_get_expand(void * message, void ** msg_expand,int expand_no);

void * load_record_template(char * type);
void * load_record_desc(char * type);
int message_read_from_blob(void ** message,void * blob,int blob_size);
int read_message_head(void * message,void * blob,int blob_size);
int read_message_data(void * message,void * blob,int data_size);
int read_message_from_src(void * message,void * src,
           int (*read_func)(void *,char *,int size));

int read_message_head_elem(void * message,char * item_name, void * value);

int load_message_record(void * message,void ** record); 
int load_all_record(void * message); 
// load next record from message , it should be called after read_message_data finished, or load_message_expand or another load_message_record;
// that is, message_box's state must be MSG_BOX_LOAD,MSG_BOX_LOAD_EXPAND or MSG_BOX_LOAD_RECORD
// if load record success, function return value is 1 and the message_box's state is MSG_BOX_LOAD_RECORD 
// if all the record in message are loaded, function return 0 and the message_box's state is MSG_BOX_LOAD_FINISH
// if load message error, function return negtive value

//int set_message_head(void * message,char * item_name, void * value);
int message_set_flag(void * message, int flag);
int message_set_state(void * message, int state);
int message_get_state(void * message);
int message_set_flow(void * message, int flow);
int message_get_flow(void * message);
int message_get_flag(void * message);
int message_read_elem(void * message,char * item_name, int index, void ** value);
int message_comp_head_elem_text(void * message,char * item_name, char * text);
int message_comp_elem_text(void * message,char * item_name, int index, char * text);
int message_comp_expand_elem_text(void * message,char * item_name, int index, char * text);

void * message_get_head(void * message);
int message_output_blob(void * message, BYTE ** blob);
int message_output_json(void * message, char * text);

//   //  _________________________________________________________________________ //

int message_load_record(void * message);
int message_load_expand(void * message);

int message_get_define_expand(void * message,void ** addr,int type,int subtype);
//int add_message_define_expand(void * message, void * expand, char * type);

//int add_message_expand_state(void * message,int state_machine_no,int state);
//int add_message_expand_identify(void * message,char * user_name,int type,int blob_size,BYTE * blob);
//int add_message_expand_forward(void * message,char * sender_uuid,char * sender_name,char * receiver_uuid, char * receiver_name);

//int add_message_expand(void * message, int record_size, int expand_no,BYTE * expand);

//char * get_message_expand_type(void * message,int expand_no);
//int  get_message_expand(void * message, void ** msg_expand,int expand_no);

int message_get_blob(void * message, void ** blob);
int message_set_blob(void * message,void * blob, int size);

int message_record_blob2struct(void * message);
int message_record_struct2blob(void * message);
int message_expand_struct2blob(void * message);
int message_2_json(void * message,char * json_str);
int json_2_message(char * json_str,void ** message);
int message_output_record_blob(void * message, BYTE ** blob);

char * message_get_typestr(void * message);
void * message_gen_typesmsg(int type,int subtype,void * active_msg);
#endif
