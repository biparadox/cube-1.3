#ifndef CUBE_FUNC_HEADER
#define CUBE_FUNC_HEADER

// function from alloc.h start
void * Talloc(int size);
void * Talloc0(int size);
void * Dalloc(int size,void * base);
void * Dalloc0(int size,void * base);

int Free(void * pointer);
int Free0(void * pointer);
// function from alloc.h end

// function from basefunc.h
typedef struct tagnameofvalue
{
	char * name;
	int value;
}__attribute__((packed)) NAME2VALUE;

typedef struct uuid_head
{
	BYTE uuid[DIGEST_SIZE];
	char name[DIGEST_SIZE];
	int type;
	int subtype;
}__attribute__((packed)) UUID_HEAD;

int   Isvaliduuid(char * uuid);
int   Isstrinuuid(BYTE * uuid);
int   Isemptyuuid(BYTE * uuid);

void * Memcpy(void * dest,void * src, unsigned int count);
void * Memset(void * s,int c, int n);
int    Memcmp(const void *s1,const void *s2,int n);

char * Strcpy(char *dest,const char *src);
char * Strncpy(char *dest,const char *src,int n);
int    Strcmp(const char *s1,const char *s2);
int    Strncmp(const char *s1,const char *s2,int n);
char * Strcat(char * dest,const char *src);
char * Strncat(char * dest,const char *src,int n);
int    Strlen(char * str);
int    Strnlen(char * str,int n);

int    Getfiledfromstr(char * name,char * str,char IFS,int maxsize);
int    Itoa(int n,char * str);
int    Atoi(char * str,int maxlen);
int    Getlowestbit(BYTE * addr,int size,int bit); 
// crypto_func start
int digest_to_uuid(BYTE *digest,char *uuid);
int uuid_to_digest(char * uuid,BYTE *digest);
int comp_proc_uuid(BYTE * dev_uuid,char * proc_name,BYTE * conn_uuid);
int calculate_by_context(BYTE **buffer,int countline,UINT32 *SM3_hash);
int calculate_context_sm3(BYTE* context, int context_size, UINT32 *SM3_hash);
int calculate_context_sha1(BYTE* context,int context_size,UINT32 *SM3_hash);
int calculate_sm3(char* filename, unsigned char *SM3_hash);


int extend_pcr_sm3digest(BYTE * pcr_value,BYTE * sm3digest);
int is_valid_uuidstr(char * uuidstr);
int sm4_context_crypt( BYTE * input, BYTE ** output, int size,char * passwd);
int sm4_context_decrypt( BYTE * input, BYTE ** output, int size,char * passwd);
int get_random_uuid(BYTE * uuid);

int sm4_text_crypt( BYTE * input, BYTE ** output, BYTE * passwd);
int sm4_text_decrypt( BYTE * input, BYTE ** output, BYTE * passwd);
void sm4_data_prepare(int input_len,BYTE * input_data,int * output_len,BYTE * output_data);
int sm4_data_recover(int input_len,BYTE * input_data,int * output_len,BYTE * output_data);

// crypto_func end

// ex_module func start
enum module_type
{
	MOD_TYPE_MAIN=0x01,
	MOD_TYPE_CONN,
	MOD_TYPE_ROUTER,
	MOD_TYPE_PORT,
	MOD_TYPE_MONITOR,
	MOD_TYPE_DECIDE,
	MOD_TYPE_CONTROL,
	MOD_TYPE_START,
	MOD_TYPE_TRANSLATER,
};

int ex_module_gettype(void * ex_mod);
char * ex_module_getname(void * ex_mod);
int ex_module_setpointer(void * ex_mod,void * pointer);
void * ex_module_getpointer(void * ex_mod);
int ex_module_sendmsg(void * ex_mod,void *msg);
int ex_module_recvmsg(void * ex_mod,void **msg);

int proc_share_data_getstate();
int proc_share_data_setstate(int state);
void * proc_share_data_getpointer();
int proc_share_data_setpointer(void * pointer);
int proc_share_data_getvalue(char * valuename,void * value);
int proc_share_data_setvalue(char * valuename,void * value);
int proc_get_uuid(BYTE * uuid);

// ex_module func end
// struct func start

int struct_get_elem_no(void * struct_template);
void * struct_get_elem(void * struct_template,int elem_no);

int struct_size(void * struct_template);
int struct_2_blob(void * addr, void * blob, void * struct_template);
int struct_2_part_blob(void * addr, void * blob, void * struct_template, int flag);
int part_blob_2_struct(void * blob, void * addr, void * struct_template, int flag);
int blob_2_struct(void * blob, void * addr, void * struct_template);

int struct_clone(void * src, void * destr, void * struct_template);
int struct_part_clone(void * src, void * destr, void * struct_template,int flag);
int struct_comp_elem(char * name,void * src_addr, void * dest_addr,void * template);
int struct_comp_elem_value(char * name,void * src, void * value, void * struct_template);
int struct_compare(void * src, void * destr, void * struct_template);
int struct_part_compare(void * src, void * destr, void * struct_template,int flag);
int struct_read_elem(char * name, void * addr, void * elem_data, void * struct_template);
int struct_write_elem(char * name, void * addr, void * elem_data, void * struct_template);
int struct_read_elem_byno(int elem_no, void * addr, void * elem_data, void * struct_template);
int struct_write_elem_byno(int elem_no, void * addr, void * elem_data, void * struct_template);
int struct_read_elem_text(char * name, void * addr, char * text, void * struct_template);
int struct_write_elem_text(char * name, void * addr, char * string, void * struct_template);
char * dup_str(char * src, int size);
int struct_2_json(void * addr, char * json_str, void * template);
int struct_2_part_json(void * addr, char * json_str, void * struct_template, int flag);
int json_2_struct(void * root, void * addr, void * struct_template);
int json_2_part_struct(void * root, void * addr, void * struct_template,int flag);

extern int deep_debug;

// struct func end
// message func start
typedef struct tagMessage_Expand_Data_Head //general expand 's head struct
{
   int  data_size;   //this expand data's size
   int  type;
   int  subtype;      //expand data's type
} __attribute__((packed)) MSG_EXPAND_HEAD;

typedef struct tagMessage_Expand_Data  //general expand data struct
{
   int  data_size;   //this expand data's size
   int  type;
   int  subtype;      //expand data's type
   void * expand;
} __attribute__((packed)) MSG_EXPAND;

struct start_para
{
	int argc;
	char **argv;
};


void * message_get_head(void * message);
int message_get_type(void * message);
int message_get_subtype(void * message);
int message_get_uuid(void * message,BYTE * uuid);
const char * message_get_sender(void * message);
const char * message_get_receiver(void * message);

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
int message_get_define_expand(void * message,void ** addr,int type,int subtype);

int message_set_flag(void * message, int flag);
int message_get_flag(void * message);
int message_get_blob(void * message, void ** blob);
int message_set_blob(void * message,void * blob, int size);

int message_2_json(void * message,char * json_str);
int json_2_message(char * json_str,void ** message);
void * message_gen_typesmsg(int type,int subtype,void * active_msg);
// message func end

// memdb func start
#define TYPE(type) DTYPE_##type
#define SUBTYPE(type,subtype) SUBTYPE_##type##_##subtype
#define TYPE_PAIR(type,subtype) DTYPE_##type,SUBTYPE_##type##_##subtype
#define RECORD(type,subtype) record_##type##_##subtype
#define tagRECORD(type,subtype) tagrecord_##type##_##subtype
#define ENUM(class) ENUM_##class
#define ENUM(class,item) ENUM_##class_##item

typedef struct db_record
{
	UUID_HEAD head;
	int name_no;
	char ** names;
	void * record;
	void * tail;
}DB_RECORD;

void * memdb_store(void * data,int type,int subtype,char * name);
int  memdb_store_record(void * record);
void * memdb_get_first(int type,int subtype);
void * memdb_get_next(int type,int subtype);
void * memdb_get_first_record(int type,int subtype);
void * memdb_get_next_record(int type,int subtype);
void * memdb_remove(void * uuid,int type,int subtype);
int memdb_remove_byname(char * name,int type,int subtype);
void * memdb_remove_record(void * record);
int memdb_free_record(void * record);

int memdb_output_blob(void * record,BYTE * blob,int type,int subtype);
int memdb_read_blob(BYTE * blob,void * record,int type,int subtype);

void * memdb_find(void * data,int type,int subtype);
void * memdb_find_first(int type,int subtype, char * name,void * value);
void * memdb_find_byname(char * name,int type,int subtype);

int memdb_find_recordtype(int type,int subtype);
int memdb_set_template(int type, int subtype, void * struct_template);

int  memdb_get_typeno(char * typestr);
int  memdb_get_subtypeno(int typeno,char * typestr);
char * memdb_get_typestr(int typeno);
char * memdb_get_subtypestr(int typeno,int subtypeno);
void * memdb_get_template(int type, int subtype);
int memdb_comp_uuid(void * record);
int memdb_comp_record_uuid(void * record,int type,int subtype, BYTE * uuid);

// memdb func end
// system func start
int print_cubeerr(char * format,...);
int print_cubeaudit(char * format,...);
int convert_uuidname(char * name,int len,BYTE * digest,char * newfilename);
int RAND_bytes(unsigned char *buffer, int len);
void print_bin_data(BYTE * data,int len,int width);
void * create_type_message(int type,int subtype,void * active_msg);

int ex_module_addslot(void * ex_mod,void * slot_port);
void * ex_module_findport(void * ex_mod,char * name);
int ex_module_addsock(void * ex_mod,void * sock);
void * ex_module_removesock(void * ex_mod,BYTE * uuid);
void * ex_module_findsock(void * ex_mod,BYTE * uuid);

void * slot_port_init(char * name, int port_num);
void * slot_port_addrecordpin(void * port,int type,int subtype);
void * slot_port_addmessagepin(void * port,int type,int subtype);
void * slot_create_sock(void * slot_port,BYTE * uuid);
int slot_sock_addrecord(void * sock,void * record);
int slot_sock_addrecorddata(void * sock,int type,int subtype, void * record);
int slot_sock_addmsg(void * sock, void * message);
int slot_sock_isactive(void * sock);
int slot_sock_isempty(void * sock);
void * slot_sock_removerecord(void * sock,int type,int subtype);
void * slot_sock_removemessage(void * sock,int type,int subtype);


// system func end
#endif
