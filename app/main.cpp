#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <wayland-client.h>
#include "xdg-shell/xdg-shell-client-protocol.h"

/* Wayland code */
struct client_state {
    /* Globals */
   struct wl_display      *wl_display;
   struct wl_registry     *wl_registry;
   struct wl_shm          *wl_shm;
   struct wl_compositor   *wl_compositor;
   struct xdg_wm_base     *xdg_wm_base;
    /* Objects */
   struct wl_surface      *wl_surface;
   struct xdg_surface     *xdg_surface;
   struct xdg_toplevel    *xdg_toplevel;
    
    //Persistent memmory block
    int width, height;
    uint32_t *data;
    int32_t shm_fd;
    size_t size;
};

//TODO: Remove when mem arena is setup. 
static int allocate_shm_file(size_t size) {
    int fd = memfd_create("wayland-shm", MFD_CLOEXEC);
    if (fd < 0) return -1;
    if (ftruncate(fd, size) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void wl_buffer_release(void *data, wl_buffer *wl_buffer) {
    (void)data;
    wl_buffer_destroy(wl_buffer);
}

static const wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

static wl_buffer * draw_frame(client_state *state) {
    wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, state->shm_fd, static_cast<int32_t>(state->size));
    wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, state->width, state->height, state->width*4, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
 
    for (int y = 0; y < state->height; ++y) {
        for (int x = 0; x < state->width; ++x) {
                state->data[y * state->width + x] = 0xFFEEEEEE;
        }
    }

    wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
    return buffer;
}

static void xdg_wm_base_ping(void *data, xdg_wm_base *base, uint32_t serial) {
    (void)data;
    xdg_wm_base_pong(base, serial);
}

static const xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void registry_handle_global(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    (void)version;
    client_state *state = static_cast<client_state*>(data);
    
    if (strcmp(interface, "wl_compositor") == 0) {
        state->wl_compositor = static_cast<wl_compositor*>(wl_registry_bind(
            registry, name, &wl_compositor_interface, 4));
    } else if (strcmp(interface, "wl_shm") == 0) {
        state->wl_shm = static_cast<wl_shm*>(wl_registry_bind(
            registry, name, &wl_shm_interface, 1));
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        state->xdg_wm_base = static_cast<xdg_wm_base*>(wl_registry_bind(
            registry, name, &xdg_wm_base_interface, 1));
        xdg_wm_base_add_listener(state->xdg_wm_base, &xdg_wm_base_listener, state);
    }
}

static void registry_handle_global_remove(void *data, wl_registry *registry, uint32_t name)
{
	(void)data;
    (void)registry;
    (void)name;
}

static const wl_registry_listener registry_listener = {
	.global = registry_handle_global,
	.global_remove = registry_handle_global_remove
};

static void xdg_surface_configure(void *data, xdg_surface *xdg_surface, uint32_t serial) {
    client_state *state = static_cast<client_state*>(data);
    xdg_surface_ack_configure(xdg_surface, serial);

    wl_buffer *buffer = draw_frame(state);
    wl_surface_attach(state->wl_surface, buffer, 0, 0);
    wl_surface_commit(state->wl_surface);
}

static const xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

int main(){
    //printf("Test\n");
    //
    //void* handle = dlopen(SO_PATH,RTLD_NOW);

    //if (handle == nullptr){
    //    printf("Failed: %s\n", dlerror());
    //    return 1;
    //}
    //
    //
    ////API_func_ptr get_api = static_cast<API_func_ptr>(dlsym(handle, "get_api"));
    //if (get_api == nullptr){
    //    printf("Failed to load api %s\n", dlerror());
    //    dlclose(handle);
    //    return 1;
    //}

    //Verda_API* api = get_api();
    //api->hello();

    //struct stat so_file_info;
    //time_t prev_so_time = 0;
    //
    //int x = 0;
    //while (x != 10) {
    //    if (stat(SO_PATH, &so_file_info) == 0){
    //        if (prev_so_time != so_file_info.st_mtime){
    //            printf("SO Recompilation detected!\n");
    //            prev_so_time = so_file_info.st_mtime;
    //            
    //            dlclose(handle);
    //            handle = nullptr;
    //            handle = dlopen(SO_PATH,RTLD_NOW);
    //            if (handle == nullptr){
    //                printf("Failed: %s\n", dlerror());
    //                return 1;
    //            }


    //            }

    //        }

    //    }
    //    
    //    sleep(1);
    //}
    //dlclose(handle);

    client_state state = {};
    state.width = 640;
    state.height = 480;
    state.size = state.width * state.height * 4;

    state.shm_fd = allocate_shm_file(state.size);
    state.data = static_cast<uint32_t*>(mmap(NULL, state.size, PROT_READ | PROT_WRITE, MAP_SHARED, state.shm_fd, 0));
    
    
    if (!(state.wl_display = wl_display_connect(nullptr))) {
        printf("Connection not established\n");
        return 1;
    }
    printf("Connection\n");

    state.wl_registry = wl_display_get_registry(state.wl_display);
    wl_registry_add_listener(state.wl_registry, &registry_listener, &state);
    
    wl_display_roundtrip(state.wl_display);

    if (!state.wl_compositor || !state.wl_shm || !state.xdg_wm_base) {
         printf("Some required globals unavailable\n");
         return 1;
    }
    
    state.wl_surface = wl_compositor_create_surface(state.wl_compositor);
    state.xdg_surface = xdg_wm_base_get_xdg_surface(state.xdg_wm_base, state.wl_surface);
    xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);

    state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
    xdg_toplevel_set_title(state.xdg_toplevel, "Window");

    wl_surface_commit(state.wl_surface);
    

    while (wl_display_dispatch(state.wl_display) != -1) {
    
    }
    
    //Cleanup
    wl_registry_destroy(state.wl_registry);
    wl_display_disconnect(state.wl_display);
    munmap(state.data, state.size);
    close(state.shm_fd);
    return 0;
        
}
