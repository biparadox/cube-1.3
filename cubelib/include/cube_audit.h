enum dtype_cube_audit {
	TYPE(CUBE_AUDIT)=0x251
};
enum subtype_cube_audit {
	SUBTYPE(CUBE_AUDIT,ROUTE_SELECT)=0x1,
	SUBTYPE(CUBE_AUDIT,MODULE_SELECT),
	SUBTYPE(CUBE_AUDIT,ROUTE_FILTER)=0x20,
	SUBTYPE(CUBE_AUDIT,MODULE_FILTER)
};
enum audit_filter_op {
	FILTER_SELALL=0x01,
	FILTER_EXCEPT,
	FILTER_SELSEQ
};
typedef struct cube_audit_route_select{
	char route_name[32];
	UINT32 filter_op;
	int seq_num;
}__attribute__((packed)) RECORD(CUBE_AUDIT,ROUTE_SELECT);

typedef struct cube_audit_module_select{
	char sender_name[32];
	char receiver_name[32];
	UINT32 filter_op;
}__attribute__((packed)) RECORD(CUBE_AUDIT,MODULE_SELECT);

typedef struct cube_audit_route_filter{
	char route_name[32];
	UINT32 filter_op;
	int seq_num;
	int seq_counter;
	BYTE match_uuid[32];
}__attribute__((packed)) RECORD(CUBE_AUDIT,ROUTE_FILTER);

