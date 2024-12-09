enum dtype_http_server {
	TYPE(HTTP_SERVER)=0x401
};
enum subtype_http_server {
	SUBTYPE(HTTP_SERVER,SERVER)=0x1,
	SUBTYPE(HTTP_SERVER,FILE),
	SUBTYPE(HTTP_SERVER,ACTION),
	SUBTYPE(HTTP_SERVER,RESULT)
};
enum enum_http_server_fileattr
{
	ENUM(HTTP_SERVER_ATTR,NORMAL)=0x01,	
	ENUM(HTTP_SERVER_ATTR,CRYPT),	
	ENUM(HTTP_SERVER_ATTR,SIGN),	
	ENUM(HTTP_SERVER_ATTR,AUTH)	
};

enum enum_http_server_action
{
	ENUM(HTTP_SERVER_ACTION,START)=0x01,	
	ENUM(HTTP_SERVER_ACTION,DOWNLOAD),	
	ENUM(HTTP_SERVER_ACTION,UPLOAD),	
	ENUM(HTTP_SERVER_ACTION,CHECK),	
	ENUM(HTTP_SERVER_ACTION,DELETE)	
};

enum enum_http_server_result
{
	ENUM(HTTP_SERVER_RESULT,EMPTY)=0x01,	
	ENUM(HTTP_SERVER_RESULT,EXIST),	
	ENUM(HTTP_SERVER_RESULT,OCCUPY),	
	ENUM(HTTP_SERVER_RESULT,ALIAS),	
	ENUM(HTTP_SERVER_RESULT,ACCORD),	
	ENUM(HTTP_SERVER_RESULT,FINISH=0x10),	
	ENUM(HTTP_SERVER_RESULT,SUCCEED),	
	ENUM(HTTP_SERVER_RESULT,ERR_NOSERVER=0x101),	
	ENUM(HTTP_SERVER_RESULT,ERR_TIMEOUT=0x102),	
	ENUM(HTTP_SERVER_RESULT,ERR_NOTCONN=0x103)	
};

typedef struct http_server_server{
	char * server_name;
	char * ip_addr;
	int port;
	BYTE uuid[32];
	char * server_dir;
	char * server_desc;
}__attribute__((packed)) RECORD(HTTP_SERVER,SERVER);

typedef struct http_server_file{
	char * file_name;
	BYTE uuid[32];
	char * owner;
	UINT32 attr_flag;
	BYTE meta_uuid[32];
}__attribute__((packed)) RECORD(HTTP_SERVER,FILE);

typedef struct http_server_action{
	char * server_name;
	char * file_name;
	BYTE uuid[32];
	enum enum_http_server_action action;
	char * user;
}__attribute__((packed)) RECORD(HTTP_SERVER,ACTION);

typedef struct http_server_result{
	char * server_name;
	char * file_name;
	BYTE uuid[32];
	enum enum_http_server_result result;
	char * user;
	char * result_desc;
}__attribute__((packed)) RECORD(HTTP_SERVER,RESULT);

