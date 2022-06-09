#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
 
#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "message.h"
#include "ex_module.h"
#include "sys_func.h"

#include "virtual_node.h"
// add para lib_include
int virtual_node_init(void * sub_proc, void * para)
{
	int ret;

    // Init virtuan node Func
    struct virtual_node_para * virt_para=para;
    if((virt_para==NULL) || (virt_para->virt_type==0))
    // normal node
    {
        return 0;
    }
    else if(virt_para->virt_type == 1)
    // virtual node, need to change node_uuid to a virtual uuid
    {
        // build virtual_node struct and change uuid
        RECORD(MESSAGE,VIRTUAL_NODE) * virtual_node;
        virtual_node =Talloc0(sizeof(*virtual_node));
        if(virtual_node ==NULL)
            return -ENOMEM;
        Strncpy(virtual_node->node_name,virt_para->node_name,DIGEST_SIZE);
        Strncpy(virtual_node->domain,virt_para->domain,DIGEST_SIZE);
        calculate_context_sm3(virtual_node->node_name,DIGEST_SIZE,virtual_node->node_uuid);
        proc_share_data_setvalue("uuid",virtual_node->node_uuid);

        // store virtual_node struct
        DB_RECORD * db_record;
        db_record = memdb_find_byname("default",TYPE_PAIR(MESSAGE,VIRTUAL_NODE));
        if(db_record != NULL)
        {
            print_cubeaudit("change virtual node node_name to %s!",virtual_node->node_name);
            memdb_remove(db_record->head.uuid,TYPE_PAIR(MESSAGE,VIRTUAL_NODE));
        }
        memdb_store(virtual_node,TYPE_PAIR(MESSAGE,VIRTUAL_NODE),"default");
    }
	return 0;
}
int virtual_node_start(void * sub_proc, void * para)
{
	int ret;
	void * recv_msg;
	int type;
	int subtype;
	// add yorself's module exec func here

	while(1)
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
		ret=proc_attach_virtnodeinfo(sub_proc,recv_msg);
	}

	return 0;
}

int proc_attach_virtnodeinfo(void * sub_proc,void * recv_msg)
{
	int ret=0;
	int fd;

	RECORD(MESSAGE,VIRTUAL_NODE)  virtual_node;

	printf("begin virtual node info  attach!\n");
    DB_RECORD * db_record;

    db_record = memdb_find_byname("default",TYPE_PAIR(MESSAGE,VIRTUAL_NODE));
    if(db_record != NULL)
    {
         ret=message_add_expand_data(recv_msg,TYPE_PAIR(MESSAGE,VIRTUAL_NODE),db_record->record);   
    }

	ret=ex_module_sendmsg(sub_proc,recv_msg);

	return ret;
}

