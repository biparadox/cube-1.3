enum dtype_general_return {
	TYPE(GENERAL_RETURN)=0x240
};
enum subtype_general_return {
	SUBTYPE(GENERAL_RETURN,INT)=0x1,
	SUBTYPE(GENERAL_RETURN,UUID),
	SUBTYPE(GENERAL_RETURN,BINDATA),
	SUBTYPE(GENERAL_RETURN,STRING),
	SUBTYPE(GENERAL_RETURN,STRING_ARRAY),
	SUBTYPE(GENERAL_RETURN,RECORD)
};
typedef struct genera_return_int{
	char * name;
	int return_value;
}__attribute__((packed)) RECORD(GENERAL_RETURN,INT);

typedef struct genera_return_uuid{
	char * name;
	BYTE return_value[32];
}__attribute__((packed)) RECORD(GENERAL_RETURN,UUID);

typedef struct genera_return_bindata{
	char * name;
	int size;
	BYTE * bindata;
}__attribute__((packed)) RECORD(GENERAL_RETURN,BINDATA);

typedef struct genera_return_string{
	char * name;
	char * return_value;
}__attribute__((packed)) RECORD(GENERAL_RETURN,STRING);

typedef struct genera_return_stringarray{
	char * name;
	int num;
	char * array;
}__attribute__((packed)) RECORD(GENERAL_RETURN,STRING_ARRAY);

typedef struct genera_return_record{
	char * name;
	int type;
	int subtype;
	int size;
	BYTE * blob;
}__attribute__((packed)) RECORD(GENERAL_RETURN,RECORD);

