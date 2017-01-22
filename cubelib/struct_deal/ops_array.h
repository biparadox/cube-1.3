



int string_struct_2_blob(void * addr, void * elem_data, void * elem_template);
int int_enum_flag_struct_2_blob(void * addr, void * elem_data,  void *	elem_template);

int uint_struct_2_blob(void * addr, void * elem_data,  void *	elem_template);
int  time_struct_2_blob(void * addr, void * elem_data,  void * elem_template);

int uchar_struct_2_blob(void * addr, void * elem_data,  void * elem_template);

int ushort_struct_2_blob(void * addr, void * elem_data,  void * elem_template);
int uint16_struct_2_blob(void * addr, void * elem_data,  void * elem_template);

int longlong_struct_2_blob(void * addr, void * elem_data,  void *elem_template);
int uint64_struct_2_blob(void * addr, void * elem_data,  void *	elem_template);

int bindata_bimap_hexdata_struct_2_blob(void * addr,void * elem_data,  void * elem_template);
int binarray_struct_2_blob(void * addr, void * elem_data,  void *elem_template);

int  vstring_struct_2_blob(void * addr, void * elem_data,  void *elem_template);
int estring_jsonstring_struct_2_blob(void * addr, void * elem_data,  void *elem_template);

int  define_defstr_struct_2_blob(void *addr, void * elem_data,void * elem_template);
int defstrarray_struct_2_blob(void * addr, void * elem_data,  void *elem_template);

int orgchain_struct_2_blob(void * addr, void * elem_data,  void * elem_template);
int nodata_struct_2_blob(void * addr, void * elem_data,  void * elem_template);


int string_blob_2_struct(void * addr, void * elem_data, void * elem_template);
int int_enum_flag_blob_2_struct(void * addr, void * elem_data, void * elem_template);

int uint32_blob_2_struct(void * addr, void * elem_data, void * elem_template);
int time_blob_2_struct(void * addr, void * elem_data, void * elem_template);

int uchar_blob_2_struct(void * addr, void * elem_data, void * elem_template);
int ushort_blob_2_struct(void * addr, void * elem_data, void * elem_template);

int  uint16_blob_2_struct(void * addr, void * elem_data, void * elem_template);
int uint64_blob_2_struct(void * addr, void * elem_data, void * elem_template);

int bindata_bimap_hexdata_blob_2_struct(void * addr, void * elem_data, void * elem_template);
int vstring_blob_2_struct(void * addr, void * elem_data, void * elem_template);

int estring_jsonstring_blob_2_struct(void * addr, void * elem_data, void * elem_template);
int longlong_blob_2_struct(void * addr, void * elem_data, void * elem_template);

int uint64_blob_2_struct(void * addr, void * elem_data, void * elem_template);
	{ CUBE_TYPE_ESTRING,&estring_convert_ops},
int bindata_bimap_hexdata_blob_2_struct(void * addr, void * elem_data, void * elem_template);

int binarray_blob_2_struct(void * addr, void * elem_data, void * elem_template);
int vstring_blob_2_struct(void * addr, void * elem_data, void * elem_template);

int estring_jsonstring_blob_2_struct(void * addr, void * elem_data, void * elem_template);
int define_defstr_blob_2_struct(void * addr, void * elem_data, void * elem_template);

int defstrarray_blob_2_struct(void * addr, void * elem_data, void * elem_template);
int orgchain__blob_2_struct(void * addr, void * elem_data, void * elem_template);
int nodata__blob_2_struct(void * addr, void *  elem_data, void * elem_template);
struct convert_ops
{
	int(*struct_2_blob)(void * struct_elem, void * blob_addr, void * struct_elem_attr);
	int(*blob_2_struct)(void * blob_addr, void * struct_elem, void * struct_elem_attr);
};
struct namevalue
{
	int  keyvalue;
	void * ops;
};

struct namevalue  ops_array[]=
{
	{ CUBE_TYPE_STRING,&string_convert_ops},
	{ CUBE_TYPE_INT,&int_enum_flag_convert_ops},
        { CUBE_TYPE_ENUM,&int_enum_flag_convert_ops }, 
        { CUBE_TYPE_FLAG,&int_enum_flag_convert_ops},  
        { CUBE_TYPE_TIME,&time_convert_ops},  
        { CUBE_TYPE_UCHAR,&uchar_convert_ops}  
        { CUBE_TYPE_USHORT,&ushort_convert_ops},  
        { CUBE_TYPE_LONGLONG,&longlong_convert_ops},  
        { CUBE_TYPE_BINDATA,&bindata_bitmap_hexdata_convert_ops},  
        { CUBE_TYPE_BITMAP,&bindata_bitmap_hexdata_convert_ops},  
        { CUBE_TYPE_HEXDATA,&bindata_bimap_hexdata_convert_ops},  
        { CUBE_TYPE_BINARRAY,&binarray_convert_ops},  
        { CUBE_TYPE_VSTRING,&vstring_convert_ops},  
        { CUBE_TYPE_ESTRING,&estring_jsonstring_convert_ops},  
        { CUBE_TYPE_JSONSTRING,&jsonstring_estring_convert_ops},  
	{ CUBE_TYPE_NODATA,&nodata_convert_ops},
	{ CUBE_TYPE_DEFINE,&define_defstr_convert_ops},
	{ CUBE_TYPE_DEFSTR,&define_defstr_convert_ops},
	{CUBE_TYPE_DEFSTRARRAY,&defstrarray_convert_ops}
}
struct convert_ops string_convert_ops =
{
	.struct_2_blob = string_struct_2_blob,
	.blob_2_struct = string_blob_2_struct,
};

struct convert_ops int_enum_flag_convert_ops =
{     
	.struct_2_blob = int_enum_flag_struct_2_blob,
	.blob_2_struct = int_enum_flag_blob_2_struct,
	
};
struct convert_ops uint_convert_ops =
{
	.struct_2_blob = uint_struct_2_blob,
	.blob_2_struct = uint_blob_2_struct,

};struct convert_ops time_convert_ops =
{
	.struct_2_blob = time_struct_2_blob,
	.blob_2_struct = time_blob_2_struct,

};
struct convert_ops estring_convert_ops =
{
	.struct_2_blob = estring_struct_2_blob,
	.blob_2_struct = estring_blob_2_struct,

};
struct convert_ops uchar_convert_ops =
{
	.struct_2_blob = uchar_struct_2_blob,
	.blob_2_struct = uchar_blob_2_struct,
}
struct convert_ops ushort_convert_ops =
{
	.struct_2_blob = ushort_struct_2_blob,
	.blob_2_struct = ushort_blob_2_struct,

};
struct convert_ops uint16_convert_ops =
{
	.struct_2_blob = uint16_struct_2_blob,
	.blob_2_struct = uint16_blob_2_struct,

};
struct convert_ops longlong_convert_ops =
{
	.struct_2_blob = longlong_struct_2_blob,
	.blob_2_struct = longlong_blob_2_struct,

};
struct convert_ops uint64_convert_ops =
{
	.struct_2_blob = uint64_struct_2_blob,
	.blob_2_struct = uint64_blob_2_struct,

};
struct convert_ops bindata_bimap_hexdata_convert_ops =
{
	.struct_2_blob = bindata_bimap_hexdata_struct_2_blob,
	.blob_2_struct = bindata_bimap_hexdata_blob_2_struct,

};
struct convert_ops binarray_convert_ops =
{
	.struct_2_blob = binarray_struct_2_blob,
	.blob_2_struct = binarray_blob_2_struct,

};
struct convert_ops vstring_convert_ops =
{
	.struct_2_blob = vstring_struct_2_blob,
	.blob_2_struct = vstring_blob_2_struct,

};
struct convert_ops jsonstring_convert_ops =
{
};
	.struct_2_blob =jsonstring_struct_2_blob,
	.blob_2_struct =jsonstring_blob_2_struct,

};
struct convert_ops define_defstr_convert_ops =
{
	.struct_2_blob = define_defstr_struct_2_blob,
	.blob_2_struct = define_defstr_blob_2_struct,

};
struct convert_ops defstrarray_convert_ops =
{
	.struct_2_blob = defstrarry_struct_2_blob,
	.blob_2_struct = defstrarry_blob_2_struct,

};
struct convert_ops orgchain_convert_ops =
{
	.struct_2_blob = orgchain_struct_2_blob,
	.blob_2_struct = orgchain_blob_2_struct,

};
struct convert_ops nodata_convert_ops =
{
	.struct_2_blob = nodata_struct_2_blob,
	.blob_2_struct = nodata_blob_2_struct,

};






