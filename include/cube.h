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

// crypto_func start
int digest_to_uuid(BYTE *digest,char *uuid);
int uuid_to_digest(char * uuid,BYTE *digest);
int comp_proc_uuid(BYTE * dev_uuid,char * proc_name,BYTE * conn_uuid);
int calculate_by_context(BYTE **buffer,int countline,UINT32 *SM3_hash);
int calculate_context_sm3(BYTE* context, int context_size, UINT32 *SM3_hash);
int calculate_context_sha1(BYTE* context,int context_size,UINT32 *SM3_hash);
int extend_pcr_sm3digest(BYTE * pcr_value,BYTE * sm3digest);
int is_valid_uuidstr(char * uuidstr);
int sm4_context_crypt( BYTE * input, BYTE ** output, int size,char * passwd);
int sm4_context_decrypt( BYTE * input, BYTE ** output, int size,char * passwd);
int calculate_sm3(char* filename, UINT32 *SM3_hash);
int get_random_uuid(BYTE * uuid);

int sm4_text_crypt( BYTE * input, BYTE ** output, BYTE * passwd);
int sm4_text_decrypt( BYTE * input, BYTE ** output, BYTE * passwd);
void sm4_data_prepare(int input_len,BYTE * input_data,int * output_len,BYTE * output_data);
int sm4_data_recover(int input_len,BYTE * input_data,int * output_len,BYTE * output_data);

// crypto_func end

// ex_module func start
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

// ex_module func end
// struct func start
int struct_size(void * struct_template);
int struct_2_blob(void * addr, void * blob, void * struct_template);
int struct_2_part_blob(void * addr, void * blob, void * struct_template, int flag);
int part_blob_2_struct(void * blob, void * addr, void * struct_template, int flag);
int blob_2_struct(void * blob, void * addr, void * struct_template);

int struct_clone(void * src, void * destr, void * struct_template);
int struct_part_clone(void * src, void * destr, void * struct_template,int flag);
int struct_comp_elem_value(char * name,void * src, void * value, void * struct_template);
int struct_compare(void * src, void * destr, void * struct_template);
int struct_part_compare(void * src, void * destr, void * struct_template,int flag);
int struct_read_elem(char * name, void * addr, void * elem_data, void * struct_template);
int struct_write_elem(char * name, void * addr, void * elem_data, void * struct_template);
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
// message func end

// memdb func start
#define TYPE(type) DTYPE_##type
#define SUBTYPE(type,subtype) SUBTYPE_##type##_##subtype
#define TYPE_PAIR(type,subtype) DTYPE_##type,SUBTYPE_##type##_##subtype
#define RECORD(type,subtype) record_##type##_##subtype
#define tagRECORD(type,subtype) tagrecord_##type##_##subtype

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

void * memdb_get_template(int type, int subtype);

// memdb func end
// system func start
int print_cubeerr(char * format,...);
int print_cubeaudit(char * format,...);
int convert_uuidname(char * name,int len,BYTE * digest,char * newfilename);
int RAND_bytes(unsigned char *buffer, size_t len);
void print_bin_data(BYTE * data,int len,int width);

// system func end
#endif
