#ifndef IEC104_CHANNEL_FUNC_H
#define IEC104_CHANNEL_FUNC_H

int modbus_slave_init(void * sub_proc,void * para);
int modbus_slave_start(void * sub_proc,void * para);

struct init_para
{
	char slave_name[32];
	UINT16 unit_addr;	
	BYTE slave_key[32];
}__attribute__((packed));

#endif
