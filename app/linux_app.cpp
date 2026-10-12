#include "../verdande/verdande.h"

#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include "xdg-shell/xdg-shell-client-protocol.h"

#include <sys/mman.h>
#include <dlfcn.h>
#include <stdio.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>


//8.3 & 8.4 Damaging and opaque surface

/* Wayland code */
struct client_state {

    struct wl_display       *wl_display;
    struct wl_registry      *wl_registry;
    struct wl_shm           *wl_shm;
    struct wl_compositor    *wl_compositor;
    struct wl_shm_pool      *wl_shm_pool;
    struct xdg_wm_base      *xdg_wm_base;

    struct wl_surface       *wl_surface;
    struct xdg_surface      *xdg_surface;
    struct xdg_toplevel     *xdg_toplevel;
    
    int width, height;

    int32_t shm_fd;
    size_t pool_size;
    uint8_t *pool_data;
    
    uint32_t frame_index;
    bool frame_ready;
    bool closed;
    draw_fn dr;
    bool configured = false;

    void *handle;

};

//TODO: Remove shm_file, buffers, pool, frame_ready, frame_index when gpu rendering

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

static wl_buffer *draw_frame(client_state *state, size_t frame_size_MAX) {

    size_t offset = state->frame_index * frame_size_MAX;
    uint32_t *BitmapMem = reinterpret_cast<uint32_t*>(state->pool_data + offset);


    state->dr(BitmapMem,state->width,state->height);

    wl_buffer *buffer = wl_shm_pool_create_buffer(state->wl_shm_pool, static_cast<int32_t>(offset),
            state->width, state->height, state->width * 4, WL_SHM_FORMAT_XRGB8888);

    wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);

    state->frame_index = !state->frame_index;
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

static void wl_surface_frame_done(void *data, wl_callback *cb, uint32_t time);

static const wl_callback_listener wl_surface_frame_listener = {
	.done = wl_surface_frame_done,
};

static void wl_surface_frame_done(void *data, wl_callback *cb, uint32_t time){
    (void)time;
    wl_callback_destroy(cb);
    
    client_state *state = static_cast<client_state*>(data);
    state->frame_ready = true;
}


static void xdg_surface_configure(void *data, xdg_surface *xdg_surface, uint32_t serial) {
    client_state *state = static_cast<client_state*>(data);
    xdg_surface_ack_configure(xdg_surface, serial);
    state->configured = true;

}

static const xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void xdg_toplevel_configure(void *data, xdg_toplevel *xdg_toplevel,
        int32_t width, int32_t height, wl_array *states) {
    (void)xdg_toplevel;
    (void)states;

    client_state *state = static_cast<client_state*>(data);  
    if (width > 0) {
        state->width = width;
    }
    if (height > 0) {
        state->height = height;
    }
}

static void xdg_toplevel_close(void *data, xdg_toplevel *xdg_toplevel) {
    (void)xdg_toplevel;
    client_state *state = static_cast<client_state*>(data);
    state->closed = true;
}

static const xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
    .configure_bounds = NULL,
    .wm_capabilities = NULL,
};

struct Static_Arena {
    uint8_t *base;
    size_t size;
    size_t offset;
};

bool arena_init(Static_Arena *arena, size_t size) {
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE| MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        printf("Failed mmap init");
        return false;
    }
    arena->base = static_cast<uint8_t*>(ptr);
    arena->size = size;
    arena->offset = 0;
    return true;
}

void* arena_alloc(Static_Arena *arena, size_t size) {
    if (arena->offset + size > arena->size) {
        return nullptr;
    }
    void *ptr = arena->base + arena->offset;
    arena->offset += size;
    return ptr;
}

void arena_destroy(Static_Arena *arena) {
    if (arena->base && arena->base != MAP_FAILED) {
        munmap(arena->base, arena->size);
    }
}


int main(){
    
    Static_Arena arena = {};
    if (!arena_init(&arena, 64ULL*1024ULL*1024ULL)) {
        return 1;
    }

    void* handle = dlopen(SO_PATH, RTLD_NOW);

    if (handle == nullptr){
        printf("Failed: %s\n", dlerror());
        return 1;
    }
    
    draw_fn draw = reinterpret_cast<draw_fn>(dlsym(handle, "draw"));
    if (draw == nullptr){
        printf("Failed to load api %s\n", dlerror());
        dlclose(handle);
        return 1;
    }
    

    client_state state = {};
    state.width = 2560;
    state.height = 1440;
    state.dr = draw;
    state.handle=handle;

    size_t single_frame_MAX_mem = 2560 * 1440 * 4;
    state.pool_size = single_frame_MAX_mem * 2;

    state.shm_fd = allocate_shm_file(state.pool_size);
    state.pool_data = static_cast<uint8_t*>(mmap(NULL, state.pool_size , PROT_READ | PROT_WRITE, MAP_SHARED, state.shm_fd, 0));
    
    if (!(state.wl_display = wl_display_connect(nullptr))) {
        printf("Connection not established\n");
        return 1;
    }

    state.wl_registry = wl_display_get_registry(state.wl_display);
    wl_registry_add_listener(state.wl_registry, &registry_listener, &state);
    
    wl_display_roundtrip(state.wl_display);

    if (!state.wl_compositor || !state.wl_shm || !state.xdg_wm_base) {
         printf("Some required globals unavailable\n");
         return 1;
    }
    
    //TODO: change to inotify and use with poll
    struct stat so_file_info;
    time_t prev_so_time = 0;

    state.wl_shm_pool = wl_shm_create_pool(state.wl_shm, state.shm_fd, static_cast<int32_t>(state.pool_size));
    
    state.wl_surface = wl_compositor_create_surface(state.wl_compositor);
    state.xdg_surface = xdg_wm_base_get_xdg_surface(state.xdg_wm_base, state.wl_surface);
    xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);

    state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
    xdg_toplevel_set_title(state.xdg_toplevel, "Window");
    xdg_toplevel_add_listener(state.xdg_toplevel, &xdg_toplevel_listener, &state);

    wl_surface_commit(state.wl_surface);

    wl_display_roundtrip(state.wl_display);

    wl_callback *cb = wl_surface_frame(state.wl_surface);
    wl_callback_add_listener(cb, &wl_surface_frame_listener, &state);

    if (state.configured) {
        wl_buffer *buffer = draw_frame(&state, single_frame_MAX_mem);
        wl_surface_attach(state.wl_surface, buffer, 0, 0);
        wl_surface_damage_buffer(state.wl_surface, 0, 0, INT32_MAX, INT32_MAX);
        wl_surface_commit(state.wl_surface);
    }

    pollfd fds[1];
    fds[0].fd = wl_display_get_fd(state.wl_display);
    fds[0].events = POLLIN;


    while (!state.closed) {
        while (wl_display_prepare_read(state.wl_display) != 0) {
            wl_display_dispatch_pending(state.wl_display);
        }

        wl_display_flush(state.wl_display);
        
        int timeout = state.frame_ready ? 0 : -1;
        poll(fds,1,timeout);

        if (fds[0].revents & POLLIN) {
            wl_display_read_events(state.wl_display);
        } else {
            wl_display_cancel_read(state.wl_display);
        }

        wl_display_dispatch_pending(state.wl_display);

        if (state.frame_ready) {
            state.frame_ready = false;

            wl_buffer *buffer = draw_frame(&state, single_frame_MAX_mem);
            wl_surface_attach(state.wl_surface, buffer, 0, 0);
            wl_surface_damage_buffer(state.wl_surface, 0, 0, INT32_MAX, INT32_MAX);

            cb = wl_surface_frame(state.wl_surface);
            wl_callback_add_listener(cb, &wl_surface_frame_listener, &state);

            wl_surface_commit(state.wl_surface);
        }

        //TODO: change to inotify?
        if (stat("build/libverdande.so", &so_file_info) == 0) {
            if (time_t time = so_file_info.st_mtime; prev_so_time != time) {
                prev_so_time = time;
                dlclose(state.handle);
                state.handle = dlopen(SO_PATH, RTLD_NOW);
            }
        }
    }


    int err = wl_display_get_error(state.wl_display);
    if (err != 0) {
        fprintf(stderr, "Wayland protocol/display error during runtime: %d\n", err);
    }

    wl_display_disconnect(state.wl_display);
    
    munmap(state.pool_data, state.pool_size);
    close(state.shm_fd);
    dlclose(state.handle);
    arena_destroy(&arena);

    return 0;
        
}
