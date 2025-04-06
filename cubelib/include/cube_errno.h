#ifndef _CUBE_ERRNO_H
#define _CUBE_ERRNO_H

//#include "../include/struct_deal.h"
//#include "../include/valuelist.h"

#define ERR_PAGE_SIZE 4096

enum cube_error_code
{
    CUBEERR_INVAL=0x01,
    CUBEERR_NOMEM,
    CUBEERR_OVERFLOW,
    CUBEERR_NOSPC,
    CUBEERR_NOACC,
    CUBEERR_ALGERR,
    CUBEERR_SYSERR,
};

typedef struct cube_error_info
{
    UINT16 error_code;
    int    error_para;
    int    errinfo_size;
    char   errinfo[0]
}ERR_INFO;

int cubeerr_init(void);
int cubeerr_write(enum cube_error_code err_code,int para,char * err_string);
ERR_INFO * cubeerr_get();
int cubeerr_sprint(ERR_INFO * err_info,char * content);

int cubeerr_info_num();
int cubeerr_throw_num();

#endif
