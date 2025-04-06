#ifndef VIRTUAL_NODE_H
#define VIRTUAL_NODE_H
 
int virtual_node_init (void * sub_proc, void * para);
int virtual_node_start (void * sub_proc, void * para);

struct virtual_node_para
{
	int virt_type;
	char * node_name;	
    char * domain;
}__attribute__((packed));
#endif
