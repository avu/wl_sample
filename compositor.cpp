#include <stdint.h>
#include <stdio.h>
#include <wayland-client.h>
#include <cstring>

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

static void randname(char *buf) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i) {
        buf[i] = 'A' + (r & 15) + (r & 16) * 2;
        r >>= 5;
    }
}

static int create_shm_file(void) {
    int retries = 100;
    do {
        char name[] = "/wl_shm-XXXXXX";
        randname(name + sizeof(name) - 7);
        --retries;
        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            return fd;
        }
    } while (retries > 0 && errno == EEXIST);
    return -1;
}

int allocate_shm_file(size_t size) {
    int fd = create_shm_file();
    if (fd < 0)
        return -1;
    int ret;
    do {
        ret = ftruncate(fd, size);
    } while (ret < 0 && errno == EINTR);
    if (ret < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

struct our_state {
    // ...
    struct wl_compositor *compositor;
    struct wl_shm *shm;
    // ...
};

static void
registry_handle_global(void *data, struct wl_registry *registry,
                       uint32_t name, const char *interface, uint32_t version) {
    printf("interface: '%s', version: %d, name: %d\n",
           interface, version, name);
    struct our_state *state = static_cast<our_state *>(data);
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = static_cast<wl_compositor *>(wl_registry_bind(registry, name, &wl_compositor_interface, 4));
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->shm = static_cast<wl_shm *>(wl_registry_bind(
                registry, name, &wl_shm_interface, 1));
    }

}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
                              uint32_t name) {
    // This space deliberately left blank
}

static const struct wl_registry_listener
        registry_listener = {
        .global = registry_handle_global,
        .global_remove = registry_handle_global_remove,
};

int
main(int argc, char *argv[])
{
    struct our_state state = { 0 };
    struct wl_display *display = wl_display_connect(NULL);
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, &state);
    wl_display_roundtrip(display);
    struct wl_surface *surface = wl_compositor_create_surface(state.compositor);
    printf("Surface %p\n", surface);
    printf("Shmem %p\n", state.shm);

    const int width = 1920, height = 1080;
    const int stride = width * 4;
    const int shm_pool_size = height * stride * 2;

    int fd = allocate_shm_file(shm_pool_size);
    uint8_t *pool_data = static_cast<uint8_t *>(mmap(NULL, shm_pool_size,
                                                     PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

    struct wl_shm *shm = state.shm; // Bound from registry
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, shm_pool_size);

    int index = 0;
    int offset = height * stride * index;
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, offset,
            width, height, stride, WL_SHM_FORMAT_XRGB8888);
    uint32_t *pixels = (uint32_t *)&pool_data[offset];
    memset(pixels, 0, width * height * 4);

    pixels = (uint32_t *)&pool_data[offset];
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if ((x + y / 8 * 8) % 16 < 8) {
                pixels[y * width + x] = 0xFF666666;
            } else {
                pixels[y * width + x] = 0xFFEEEEEE;
            }
        }
    }
    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage(surface, 0, 0, UINT32_MAX, UINT32_MAX);
    wl_surface_commit(surface);

    return 0;
}