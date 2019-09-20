#ifndef MAIN_PROC_FUNC_H
#define MAIN_PROC_FUNC_H

struct main_config
{
	char proc_name[DIGEST_SIZE];
	char * init_dlib;
	char * init_func;
	void * init_para;	
}__attribute__((packed));

struct plugin_config
{
	char name[DIGEST_SIZE];
//	enum proc_type type;
	char * plugin_dlib;
	char * init;
	char * start;	
	void * init_para;
}__attribute__((packed));

struct lib_para_struct
{
	char * libname;	
	enum module_type type;
	char * dynamic_lib;
	char * init_func;
	char * start_func;
	void * para_template;
}__attribute__((packed));


char *  get_temp_filename(char * tag );
int get_local_uuid(BYTE * uuid);
int read_json_file(char * file_name);
void * main_read_func(char * libname,char * sym);
int read_sys_cfg(void ** lib_para_struct,void * root_node,char * plugin_dir);

int  cube_path_add(char * name,char * path);
char * cube_path_get_first(char * name); 
char * cube_path_get_next(char * name);
char * cube_path_remove(char * name); 

#endif
