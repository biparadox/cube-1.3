#ifndef FILE_STRUCT_H
#define FILE_STRUCT_H

enum dtype_file_process
{
	TYPE(FILE_TRANS)=0x301,
};

enum subtype_file_trans
{
	SUBTYPE(FILE_TRANS,FILE_INFO)=0x01,
	SUBTYPE(FILE_TRANS,FILE_DATA),
	SUBTYPE(FILE_TRANS,REQUEST),
	SUBTYPE(FILE_TRANS,FILE_STORE),
	SUBTYPE(FILE_TRANS,FILE_NOTICE)
};

typedef struct policy_file
{
	BYTE uuid[DIGEST_SIZE];
	int  type;
	int subtype;  // this policy's type and subtype, used when it is a record store file。otherwise type is 0
	char * creater;
	BYTE creater_uuid[DIGEST_SIZE];
	char * file_path;
	BYTE file_uuid[DIGEST_SIZE];
	char * file_describe;
}__attribute__((packed)) RECORD(FILE_TRANS,FILE_INFO); 

typedef struct policyfile_data
{
	BYTE uuid[DIGEST_SIZE];
	char * filename;
	int total_size;
	int record_no;
	int offset;
	int data_size;
	BYTE * file_data; //the file data
}__attribute__((packed)) RECORD(FILE_TRANS,FILE_DATA);

typedef struct policyfile_req
{
	char uuid[DIGEST_SIZE];
	char * filename;
	char * requestor;
}__attribute__((packed)) RECORD(FILE_TRANS,REQUEST);

//the struct of policy file data request: use type FILS
typedef struct policyfile_store
{
	char uuid[DIGEST_SIZE];
	char * filename;
	int file_size;
	int block_size;
	int block_num;
	int mark_len;
	BYTE * marks;
}__attribute__((packed)) RECORD(FILE_TRANS,FILE_STORE);


//the struct of policy file data notice message: use type FILN
typedef struct policyfile_notice
{
	char uuid[DIGEST_SIZE];
	char * filename;
	int result;
}__attribute__((packed)) RECORD(FILE_TRANS,FILE_NOTICE);
#endif
