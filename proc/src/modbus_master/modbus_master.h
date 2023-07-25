#ifndef MODBUS_MASTER_FUNC_H
#define MODBUS_MASTER_FUNC_H

int modbus_master_init(void * sub_proc,void * para);
int modbus_master_start(void * sub_proc,void * para);

struct init_para
{
	char master_name[32];
    int unit_num;
}__attribute__((packed));

#endif
