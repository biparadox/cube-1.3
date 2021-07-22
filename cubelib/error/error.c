/**
 * File: basefunc.c
 *
 * Created on: Jun 5, 2015
 * Author: Hu Jun (algorist@bjut.edu.cn)
 */

#include <stdio.h>
#include <stdlib.h>

#include "../include/data_type.h"
#include "../include/alloc.h"
#include "../include/memfunc.h"
#include "../include/errno.h"

enum errpage_state
{
    ERRPAGE_INIT=0x1,
    ERRPAGE_OVERFLOW,
};

BYTE Buf[DIGEST_SIZE*16];

struct cube_error_page
{
    int  offset;
    enum errpage_state state;
    int  errinfo_num;
    int  throw_num;
    int  start_site;
    int  end_site;
    int  buf_size;
};

static struct cube_error_page * error_page;

int cubeerr_init()
{
    int ret;
    void * buf = malloc(ERR_PAGE_SIZE);
    if(buf == NULL)
        return -CUBEERR_NOMEM;
    error_page = (struct cube_error_page *)buf;    
    error_page->offset = sizeof(struct cube_error_page);
    error_page->state = ERRPAGE_INIT;
    error_page->errinfo_num=0;
    error_page->throw_num=0;
    error_page->start_site = error_page->offset;
    error_page->end_site = error_page->offset;
    error_page->buf_size =  ERR_PAGE_SIZE-error_page->offset;

    return 0;
}

void _cubeerr_get_head(ERR_INFO * head)
{
       int left_size=error_page->buf_size-error_page->start_site;
       if(left_size >sizeof(ERR_INFO))
       {
            Memcpy(head,((void *)error_page)+error_page->start_site,sizeof(ERR_INFO));
       }
       else
       {
            Memcpy(head,((void *)error_page)+error_page->start_site,left_size);
            Memcpy(((void *)head)+left_size,((void *)error_page)+error_page->offset,sizeof(ERR_INFO)-left_size);
       }
}

int _cubeerr_write_buf( void * write_buf,int size)
{
     int left_size;
     if(size<=0)
        return 0;
     left_size=error_page->buf_size-error_page->end_site;
     if(left_size>size)
     {
        Memcpy(((void *)error_page)+error_page->end_site,write_buf,size);
        error_page->end_site+=size;
     }
     else
     {
        Memcpy(((void *)error_page)+error_page->end_site,write_buf,left_size);
        Memcpy(((void *)error_page)+error_page->offset,write_buf+left_size,size-left_size);
        error_page->end_site = error_page->offset+size-left_size;
     }  

     return 0;    
}

int _cubeerr_write_info(ERR_INFO * err_info)
{
      // write error info head
      int ret;
      ret = _cubeerr_write_buf( err_info,sizeof(ERR_INFO));
      ret = _cubeerr_write_buf(err_info->errinfo,err_info->errinfo_size);
      error_page->errinfo_num++;
      return ret; 
}

// 0 means throw it, 1 means

int _cubeerr_read_buf( void * read_buf,int size)
{
     int left_size;
     if(size<=0)
        return 0;
     left_size=error_page->buf_size-error_page->start_site;
     if(left_size>size)
     {
        if(read_buf!=NULL)
            Memcpy(read_buf,((void *)error_page)+error_page->start_site,size);
        error_page->start_site+=size;
     }
     else
     {
        if(read_buf!=NULL)
        {
            Memcpy(read_buf,((void *)error_page)+error_page->start_site,left_size);
            Memcpy(read_buf+left_size,((void *)error_page)+error_page->offset,size-left_size);
        }
        error_page->start_site = error_page->offset+size-left_size;
     }  
     return 0;    
}

int _cubeerr_empty_size()
{
    int empty_size;
    if(error_page->end_site >= error_page->start_site)
    {
        empty_size=error_page->buf_size-error_page->end_site;
        empty_size+= error_page->start_site-error_page->offset;
    }
    else
    {
        empty_size=error_page->start_site-error_page->end_site;
    }
    return empty_size;
}

int cubeerr_write(enum cube_error_code err_code,int para, char * err_string)
{
    ERR_INFO err_info;
    ERR_INFO throw_info;
    int info_size;
    int left_size;
    int ret;

    err_info.error_code = err_code;
    err_info.error_para=para;
    err_info.errinfo_size=Strlen(err_string);

    info_size=sizeof(ERR_INFO)+err_info.errinfo_size;
    if(info_size>DIGEST_SIZE*16)
    {
        error_page->throw_num++;
        return -CUBEERR_OVERFLOW;
    }

    while (info_size > _cubeerr_empty_size())
    {
        _cubeerr_get_head(&throw_info);
        _cubeerr_read_buf(NULL,sizeof(ERR_INFO));
        _cubeerr_read_buf(NULL,throw_info.errinfo_size);
        error_page->throw_num++;
        error_page->errinfo_num--;
    }

    ret=_cubeerr_write_info(&err_info);
    return ret;
}

ERR_INFO * cubeerr_get()
{
    ERR_INFO * return_info;
    ERR_INFO info_head; 

}
