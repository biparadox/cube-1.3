#include <stdio.h> 
#include <stdlib.h> 
#include <dlfcn.h>

char * plugin_file ="./libecho_plugin.so";

int main()
{
    void *handle;
    int (*func)(void *, void *);
    char *error;
    int a = 30;
    int b = 5;

    handle = dlopen(plugin_file, RTLD_NOW);
    if (handle == NULL)
    {
    	fprintf(stderr, "Failed to open libaray %s error:%s\n", plugin_file, dlerror());
    	return -1;
    }
    func = dlsym(handle,"echo_plugin_init");

/*
    func = dlsym(handle, "add");
    printf("%d + %d = %d\n", a, b, func(a, b));

    func = dlsym(handle, "sub");
    printf("%d + %d = %d\n", a, b, func(a, b));

    func = dlsym(handle, "div");
    printf("%d + %d = %d\n", a, b, func(a, b));
    
    func = dlsym(handle, "mul");
    printf("%d + %d = %d\n", a, b, func(a, b));
*/
    dlclose(handle);
    return 0;
}
