#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "json.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "basefunc.h"
#include "memdb.h"
#include "message.h"
#include "channel.h"
#include "connector.h"
#include "ex_module.h"
#include "sys_func.h"
#include "sm3_ext.h"

#include "modbus_slave.h"
#include "modbus_tcp.h"
#include "modbus_state.h"
#include "modbus_cmd.h"

#define MAX_LINE_LEN 1024

static BYTE Buf[DIGEST_SIZE*64];
struct tcloud_connector * client_conn;
int NUM = 0B00000000;

RECORD(MODBUS_STATE,SLAVE) * slave_index;

int proc_slavestate_set(void * sub_proc,void * recv_msg);

int modbus_slave_init(void * sub_proc,void * para)
{
    
    int ret;
    struct init_para * init_para=(struct init_para *)para;


    slave_index = Dalloc0(sizeof(*slave_index),sub_proc);
    if(slave_index==NULL)
        return -ENOMEM;
    Strncpy(slave_index->slave_name,init_para->slave_name,DIGEST_SIZE);

    slave_index->unit_addr=init_para->unit_addr;
    Memcpy(slave_index->slave_key,init_para->slave_key,DIGEST_SIZE);
    ex_module_setpointer(sub_proc,slave_index);
    return 0;
}

int modbus_slave_start(void * sub_proc,void * para)
{
    int ret = 0;
    int rc = 0;
    RECORD(MODBUS_STATE,SLAVE) * slave_index;
    void * recv_msg;
    void * send_msg;
    int type;
    int subtype;

    ret = proc_slavestate_send(sub_proc);
    if(ret<0)
        return ret;
    for (;;)
    {
        usleep(time_val.tv_usec);
        ret=ex_module_recvmsg(sub_proc,&recv_msg); 
        if(ret<0)
            continue;
        if(recv_msg==NULL)
            continue;
        type=message_get_type(recv_msg);
        subtype=message_get_subtype(recv_msg);
        if(!memdb_find_recordtype(type,subtype))
        {
            printf("message format (%d %d) is not registered!\n",
                message_get_type(recv_msg),message_get_subtype(recv_msg));
            continue;
        }
        if((type==TYPE(MODBUS_STATE))&&(subtype==SUBTYPE(MODBUS_STATE,SERVER)))
        {
            //printf("client message %p\n",sub_proc);
            ret=proc_slavestate_set(sub_proc,recv_msg); 
            slave_index =  ex_module_getpointer(sub_proc);
        }
        else if((type==TYPE(MODBUS_CMD)))
        {
            slave_index =  ex_module_getpointer(sub_proc);
            if(slave_index->state_machine == MODBUS_CONNECT)
            {
                slave_index->state_machine = MODBUS_RESPONSE;

                ret=ex_module_sendmsg(sub_proc,recv_msg);
            }
            else
            {		 
                slave_index->state_machine=MODBUS_CONNECT;
	     }	
             ret=ex_module_sendmsg(sub_proc,recv_msg);
        }
        else if((type==TYPE(MODBUS_DATA)))
        {
            slave_index =  ex_module_getpointer(sub_proc);
            if(slave_index->state_machine == MODBUS_RESPONSE)
            {
                slave_index->state_machine = MODBUS_CONNECT;
                ret=ex_module_sendmsg(sub_proc,recv_msg);
            }
            else
                slave_index->state_machine=MODBUS_CONNECT;
        }
    }
    return 0;
}

int proc_slavestate_send(void * sub_proc)
{
    int ret = 0;
    RECORD(MODBUS_STATE,SLAVE) * slave_index;
    void * send_msg;
    slave_index = (RECORD(MODBUS_STATE,SLAVE)*) ex_module_getpointer(sub_proc);

    if(slave_index == NULL)
        return -EINVAL;
    send_msg = message_create(TYPE_PAIR(MODBUS_STATE,SLAVE),NULL);
    if(send_msg==NULL)
        return -EINVAL;
    ret=message_add_record(send_msg,slave_index);
    if(ret<0)
        return -EINVAL;
    ret=ex_module_sendmsg(sub_proc,send_msg);
    return ret;
}
int proc_slavestate_set(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(MODBUS_STATE,SLAVE) * slave_index;
    RECORD(MODBUS_STATE,SERVER) * server_index;

    slave_index = (RECORD(MODBUS_STATE,SLAVE)*) ex_module_getpointer(sub_proc);
    if(slave_index == NULL)
        return -EINVAL;

    ret = message_get_record(recv_msg,&server_index,0);
    if(ret<0)
        return ret;
    if(server_index ==NULL)
        return -EINVAL;
    slave_index->state_machine=MODBUS_CONNECT;
    ret = ex_module_setpointer(sub_proc,slave_index);
    return ret;
}
