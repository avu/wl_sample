#include <wayland-client-core.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h> // Wayland EGL MUST be included before EGL headers

#include "xdg-shell-client-protocol.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <GLES2/gl2.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <sys/time.h>

/* Wayland code */
struct client_state {
    /* Globals */
    struct wl_display *wl_display;
    struct wl_registry *wl_registry;
    struct wl_shm *wl_shm;
    struct wl_compositor *wl_compositor;
    struct xdg_wm_base *xdg_wm_base;
    /* Objects */
    struct wl_surface *wl_surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct wl_egl_window *egl_window;
    struct wl_region *region;

    EGLNativeDisplayType native_display;
    EGLNativeWindowType native_window;
    uint16_t window_width, window_height;
    EGLDisplay  display;
    EGLContext  context;
    EGLSurface  surface;
    bool program_alive;
    int32_t old_w, old_h;
};

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720


static void xdg_toplevel_handle_configure(void *data,
		struct xdg_toplevel *xdg_toplevel, int32_t w, int32_t h,
		struct wl_array *states) {
    struct client_state *state = (struct client_state *)data;
	// no window geometry event, ignore
	if(w == 0 && h == 0) return;

	// window resized
	if(state->old_w != w && state->old_h != h) {
		state->old_w = w;
		state->old_h = h;

		wl_egl_window_resize(state->native_window, w, h, 0, 0);
		wl_surface_commit(state->wl_surface);
	}
}

static void xdg_toplevel_handle_close(void *data, struct xdg_toplevel *xdg_toplevel) {
    struct client_state *state = (struct client_state *)data;
    state->program_alive = false;
}

struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = xdg_toplevel_handle_configure,
	.close = xdg_toplevel_handle_close,
};


static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
	// confirm that you exist to the compositor
	xdg_surface_ack_configure(xdg_surface, serial);
}

const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
	xdg_wm_base_pong(xdg_wm_base, serial);
}

const struct xdg_wm_base_listener xdg_wm_base_listener = {
	.ping = xdg_wm_base_ping,
};

void create_native_window(struct client_state* state, char *title, int width, int height) {
	state->old_w = width;
	state->old_h = height;

	state->region = wl_compositor_create_region(state->wl_compositor);

	wl_region_add(state->region, 0, 0, width, height);
	wl_surface_set_opaque_region(state->wl_surface, state->region);

	struct wl_egl_window *egl_window = 
		wl_egl_window_create(state->wl_surface, width, height);

	if (egl_window == EGL_NO_SURFACE) {
		fprintf(stderr, "No window... \n");
		exit(1);
	}

	state->window_width = width;
    state->window_height = height;
    state->native_window = egl_window;
}

EGLBoolean create_egl_context (struct client_state* state) {
	EGLint numConfigs;
	EGLint majorVersion;
	EGLint minorVersion;
	EGLContext context;
	EGLSurface surface;
	EGLConfig config;
	EGLint fbAttribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE,        8,
		EGL_GREEN_SIZE,      8,
		EGL_BLUE_SIZE,       8,
		EGL_NONE
	};
	EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
	EGLDisplay display = eglGetDisplay( state->native_display );
	if ( display == EGL_NO_DISPLAY )
	{
		fprintf(stderr, "No EGL Display...\n");
		return EGL_FALSE;
	}

	// Initialize EGL
	if ( !eglInitialize(display, &majorVersion, &minorVersion) )
	{
		fprintf(stderr, "No Initialisation...\n");
		return EGL_FALSE;
	}

	// Get configs
	if ( (eglGetConfigs(display, NULL, 0, &numConfigs) != EGL_TRUE) || (numConfigs == 0))
	{
		fprintf(stderr, "No configuration...\n");
		return EGL_FALSE;
	}

	// Choose config
	if ( (eglChooseConfig(display, fbAttribs, &config, 1, &numConfigs) != EGL_TRUE) || (numConfigs != 1))
	{
		fprintf(stderr,"No configuration...\n");
		return EGL_FALSE;
	}

	// Create a surface
	surface = eglCreateWindowSurface(display, config, state->native_window, NULL);
	if ( surface == EGL_NO_SURFACE )
	{
		fprintf(stderr, "No surface...\n");
		return EGL_FALSE;
	}

	// Create a GL context
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs );
	if ( context == EGL_NO_CONTEXT ) {
		fprintf(stderr, "No context...\n");
		return EGL_FALSE;
	}

	// Make the context current
	if (!eglMakeCurrent(display, surface, surface, context) )
	{
		fprintf(stderr, "Could not make the current window current !\n");
		return EGL_FALSE;
	}

	state->display = display;
	state->surface = surface;
	state->context = context;
	return EGL_TRUE;
}

EGLBoolean create_window_with_egl_context(struct client_state* state, char *title, int width, int height) {
    create_native_window(state, title, width, height);
	return create_egl_context(state);
}

void draw() {
	struct timeval tv;
    gettimeofday(&tv, NULL);
    double time = double (tv.tv_sec % 10) / 10.0;
    glClearColor((float)time, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void refresh_window(struct client_state *state) {
    eglSwapBuffers(state->display, state->surface);
}

static void global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
                                    const char *interface, uint32_t version)
{
    struct client_state *state = (struct client_state *)data;
	fprintf(stderr, "Got a registry event for %s id %d\n", interface, id);
	if (strcmp(interface, "wl_compositor") == 0) {
        state->wl_compositor = static_cast<wl_compositor *>(wl_registry_bind(registry, id, &wl_compositor_interface, 1));
    } else if(strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->xdg_wm_base = static_cast<xdg_wm_base *>(wl_registry_bind(registry, id,
                                                                         &xdg_wm_base_interface, 1));
		xdg_wm_base_add_listener(state->xdg_wm_base, &xdg_wm_base_listener, data);
	} 
}

static void global_registry_remover (void *data, struct wl_registry *registry, uint32_t id) {
	fprintf(stderr, "Got a registry losing event for %d\n", id);
}

const struct wl_registry_listener listener = {
	global_registry_handler,
	global_registry_remover
};

static void get_server_references(struct client_state* client_state) {
	struct wl_display * display = wl_display_connect(NULL);
	if (display == NULL) {
		fprintf(stderr, "Can't connect to wayland display.\n");
		exit(1);
	}

	struct wl_registry *wl_registry =
		wl_display_get_registry(display);
	wl_registry_add_listener(wl_registry, &listener, client_state);

	// This call the attached listener global_registry_handler
	wl_display_dispatch(display);
	wl_display_roundtrip(display);

	// If at this point, global_registry_handler didn't set the 
	// compositor, nor the shell, bailout !
	if (client_state->wl_compositor == NULL || client_state->xdg_wm_base == NULL) {
		fprintf(stderr, "No compositor or XDG\n");
		exit(1);
	} else {
		client_state->native_display = display;
	}
}

void destroy_window(struct client_state* client_state) {
	eglDestroySurface(client_state->display, client_state->surface);
	wl_egl_window_destroy(client_state->native_window);
	xdg_toplevel_destroy(client_state->xdg_toplevel);
	xdg_surface_destroy(client_state->xdg_surface);
	wl_surface_destroy(client_state->wl_surface);
	eglDestroyContext(client_state->display, client_state->context);
}

int main() {
    struct client_state state = {0};
	get_server_references(&state);

	state.wl_surface = wl_compositor_create_surface(state.wl_compositor);
	if (state.wl_surface == NULL) {
		fprintf(stderr, "No compositor surface.\n");
		exit(1);
	}

	state.xdg_surface = xdg_wm_base_get_xdg_surface(state.xdg_wm_base, state.wl_surface);

	xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);

	state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
	xdg_toplevel_set_title(state.xdg_toplevel, "Wayland EGL example");
	xdg_toplevel_add_listener(state.xdg_toplevel, &xdg_toplevel_listener, &state);

	wl_surface_commit(state.wl_surface);

    create_window_with_egl_context(&state, "EGL", WINDOW_WIDTH, WINDOW_HEIGHT);

	state.program_alive = true;

	while (state.program_alive) {
		wl_display_dispatch_pending(state.native_display);
		draw();
        refresh_window(&state);
	}

	destroy_window(&state);
	wl_display_disconnect(state.native_display);
}

