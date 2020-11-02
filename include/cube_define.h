#ifndef CUBE_DEFINE_HEADER
#define CUBE_DEFINE_HEADER

// struct define start
enum cube_struct_elem_type   // describe types could be used in the struct
{
	CUBE_TYPE_STRING=0x01,   // an string with fixed size
	CUBE_TYPE_UUID,     // an string with fixed size
	CUBE_TYPE_INT,      // an 32-bit int
	CUBE_TYPE_ENUM,      // a 32-bit enum
	CUBE_TYPE_SENUM,      // a 16-bit enum
	CUBE_TYPE_BENUM,      // an 8-bit enum
	CUBE_TYPE_FLAG,      // a 32-bit flag
	CUBE_TYPE_SFLAG,      // a 16-bit flag
	CUBE_TYPE_BFLAG,      // an 8-bit flag
	CUBE_TYPE_TIME,     // an struct of time_t
	CUBE_TYPE_UCHAR,    // a unsigned octet
	CUBE_TYPE_USHORT,   // a 16-bit unsigned word
	CUBE_TYPE_LONGLONG, // a 64-bit longlong integer
	CUBE_TYPE_BINDATA,  // a sequence of octets with fixed size
	CUBE_TYPE_BITMAP,   // a sequence of octets with fixed size(just like BINDATA),but we use eight bin string (like 01001010) to show them	 
	CUBE_TYPE_HEXDATA,   // a sequence of octets with fixed size(just like BINDATA),but we use 2 hex string (like ce) to show them	 
	CUBE_TYPE_BINARRAY,   // an array of sequence of octets with fixed size, attr is the sequence's size, size is array's length	 
	CUBE_TYPE_UUIDARRAY,   // an array of sequence of octets with fixed size, attr is the sequence's size, size is array's length	 
	CUBE_TYPE_DEFUUIDARRAY,   // an array of sequence of octets with fixed size, attr is the sequence's size, size is array's length	 
	CUBE_TYPE_DEFNAMELIST,   // an array of sequence of octets with fixed size, attr is the sequence's size, size is array's length	 
	CUBE_TYPE_BOOL,  
	CUBE_TYPE_ESTRING,  // a variable length string ended with '\0'
	CUBE_TYPE_JSONSTRING,  // a variable length string encluded in "{}", "[]" or "\"\"" or "\'\'", it is only special in struct_json, other times,
       			        // it is same as ESTRING	
	CUBE_TYPE_NODATA,   // this element has no data
	CUBE_TYPE_ARRAY,   // this element use an pointer point to a fixed number element
	CUBE_TYPE_DEFARRAY,   // this element has no data
	CUBE_TYPE_DEFINE,	//an octets sequence whose length defined by a forhead element (an uchar, an ushort or a int element), the attr parameter 
				//show the element's name, 
	CUBE_TYPE_DEFSTR,	//a string whose length defined by a forhead element (an uchar, an ushort or a int element), the attr parameter 
				//show the element's name, 
	CUBE_TYPE_DEFSTRARRAY,	//a fixed string' s array whose elem number defined by a forhead element (an uchar, an ushort,a int element or 
				//a string like "72", the attr parameter show the forhead element's name, the elem_attr->size show how
			 	// the string's fixed length.
				// NOTE: there should not be any ' ' in the string.
				//
	CUBE_TYPE_SUBSTRUCT,    // this element describes a new struct in this site, attr points to the description of the new struct
        CUBE_TYPE_CHOICE,
		
	TPM_TYPE_UINT64,
	TPM_TYPE_UINT32,
	TPM_TYPE_UINT16,
	CUBE_TYPE_USERDEF=0x80,
	CUBE_TYPE_ENDDATA=0x100,
};

enum cube_struct_elem_attr
{
	CUBE_ELEM_FLAG_INDEX=0x01,
	CUBE_ELEM_FLAG_KEY=0x02,
	CUBE_ELEM_FLAG_INPUT=0x04,
	CUBE_ELEM_FLAG_DESC=0x10,
	CUBE_ELEM_FLAG_TEMP=0x1000,
};

// struct define end
// memdb define start
enum base_cube_db
{
	DB_INDEX=0x01,
	DB_NAMELIST,
	DB_STRUCT_DESC,
	DB_TYPELIST,
	DB_SUBTYPELIST,
	DB_CONVERTLIST,
	DB_RECORDTYPE,
	DB_BASEEND=0x10,
	DB_DTYPE_START=0x100,
};
// memdb define end
//
// message define start
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

typedef struct tagMessage_Head  //Ç¿ÖÆ·ÃÎÊ¿ØÖÆ±ê¼Ç
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
// message define end
// sys_func define start
extern struct timeval time_val;
// sys_func define end

#endif
