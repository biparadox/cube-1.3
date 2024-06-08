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

#include "modbus_master.h"
#include "modbus_tcp.h"
#include "modbus_state.h"
#include "modbus_cmd.h"

#define MAX_LINE_LEN 1024

static BYTE Buf[DIGEST_SIZE*64];
struct tcloud_connector * client_conn;

void * proc_masterindex_init(void * sub_proc,void * recv_msg);

int modbus_master_init(void * sub_proc,void * para)
{
    
    int ret;
    struct init_para * init_para=(struct init_para *)para;
    RECORD(MODBUS_STATE,MASTER) * master_index;

    master_index = Dalloc0(sizeof(*master_index),sub_proc);
    if(master_index==NULL)
        return -ENOMEM;
    Strncpy(master_index->master_name,init_para->master_name,DIGEST_SIZE);
    master_index->unit_num=init_para->unit_num;
    ex_module_setpointer(sub_proc,master_index);
    return 0;
}

int modbus_master_start(void * sub_proc,void * para)
{
    int ret = 0;
    int rc = 0;
    RECORD(MODBUS_STATE,MASTER) * master_index;
    void * recv_msg;
    void * send_msg;
    int type;
    int subtype;

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
        if((type==TYPE(MODBUS_STATE))&&(subtype==SUBTYPE(MODBUS_STATE,SLAVE)))
        {
                ret = proc_masterstate_set(sub_proc,recv_msg);
                if(ret<0)
                    return ret;
            
        }
        else if((type==TYPE(MODBUS_CMD)))
        {
            master_index =  ex_module_getpointer(sub_proc);

           ret=ex_module_sendmsg(sub_proc,recv_msg);
        }
        else if((type==TYPE(MODBUS_DATA)))
        {
            master_index =  ex_module_getpointer(sub_proc);
            ret=ex_module_sendmsg(sub_proc,recv_msg);
        }
    }
    return 0;
}

int proc_masterstate_set(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(MODBUS_STATE,SLAVE) * slave_index;
    RECORD(MODBUS_STATE,MASTER) * master_index;

    master_index = (RECORD(MODBUS_STATE,MASTER)*) ex_module_getpointer(sub_proc);
    if(master_index == NULL)
        return -EINVAL;

    ret = message_get_record(recv_msg,&slave_index,0);
    if(ret<0)
        return ret;
    if(slave_index ==NULL)
        return -EINVAL;
    slave_index->state_machine=MODBUS_CONNECT;
    
    DB_RECORD * db_record; 
    db_record = memdb_store(slave_index,TYPE_PAIR(MODBUS_STATE,SLAVE),slave_index->slave_name);
    if(db_record == NULL)
        return -EINVAL;

    master_index->slave_uuid= Dalloc0(DIGEST_SIZE*master_index->unit_num,sub_proc);
    if(master_index->slave_uuid==NULL)
        return -ENOMEM;
    Memcpy(master_index->slave_uuid,db_record->head.uuid,DIGEST_SIZE);    

    ret = ex_module_setpointer(sub_proc,master_index);
    return ret;
}
