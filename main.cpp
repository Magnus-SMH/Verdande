#include <stdio.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wayland-client.h>

int main()
{
    printf("Test\n");
    
    void* handle = dlopen(SO_PATH,RTLD_NOW);
    
    if (handle == nullptr){
        printf("Feil: %s\n", dlerror());
        return 1;
    }
        
    void (*hello)(void) = reinterpret_cast<void (*)()>(dlsym(handle, "hello"));
    
    hello();
    struct stat so_file_info;
    time_t prev_so_time = 0;
    
    while (true)
    {
        if (stat("build/libVerdande.so", &so_file_info) == 0){

            if (prev_so_time != so_file_info.st_mtime){
                printf("\nSO Recompilation detected!");
                prev_so_time = so_file_info.st_mtime;
                //dlclose, then dlopen and dlsym again
            }
            //printf("\ntime: %lld", (long long)so_file_info.st_mtime);


        }
        
        sleep(1);
    }
    dlclose(handle);
    
    //struct wl_display *display = nullptr;
    //display = wl_display_connect(nullptr);

    //if (display == nullptr) {
    //    printf("Connection not established\n");
    //    return 3;
    //}
    //printf("Connection\n");

    //wl_display_disconnect(display);
    //return 0;
        
}
