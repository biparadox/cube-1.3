#ifndef USER_INFO_H
#define USER_INFO_H
#include <time.h>

enum visual_sort_type
{
	DTYPE_VISUAL_SORT=0x1101
};

enum visual_sort_subtype
{
	SUBTYPE_VISUAL_DATA=0x01,
	SUBTYPE_VISUAL_REQ
};

enum data_type
{
	DATA_INIT=0x01,
	DATA_ADD=0x02,
	DATA_DEL=0x03,
	DATA_INSERT=0x04,
	DATA_SWAP=0x05,
	DATA_RESULT=0x06,
	DATA_ERROR=0xFF,
};

struct visual_data
{
	enum data_type type;
	int  index;
	char * name;
	int  value;
}__attribute__((packed));

struct visual_req
{
	int  size;
	char * name;
	int  interval;
}__attribute__((packed));

#endif
