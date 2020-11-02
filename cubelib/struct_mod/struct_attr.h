/*************************************************
*       Hige security Linux Operating System Project
*
*	File description: 	Definition of data describe struct header file
*	File name:		struct_deal.h
*	date:    	2008-05-09
*	Author:    	Hu jun
*************************************************/
#ifndef  _CUBE_STRUCT_ATTR_H
#define  _CUBE_STRUCT_ATTR_H
#include "../include/struct_deal.h"

extern ELEM_OPS string_convert_ops;
extern ELEM_OPS estring_convert_ops;
extern ELEM_OPS bindata_convert_ops;
extern ELEM_OPS hexdata_convert_ops;
extern ELEM_OPS define_convert_ops;
extern ELEM_OPS int_convert_ops;
extern ELEM_OPS uuid_convert_ops;
extern ELEM_OPS uuidarray_convert_ops;
extern ELEM_OPS defuuidarray_convert_ops;
extern ELEM_OPS defnamelist_convert_ops;
extern ELEM_OPS enum_convert_ops;
extern ELEM_OPS flag_convert_ops;
extern ELEM_OPS tpm_uint64_convert_ops;
extern ELEM_OPS tpm_uint32_convert_ops;
extern ELEM_OPS tpm_uint16_convert_ops;

#endif
