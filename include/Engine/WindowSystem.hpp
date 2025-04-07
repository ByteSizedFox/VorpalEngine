#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "volk.h"
#include <wayland-client.h>
#include "xdg-shell-protocol.h"

static struct wl_compositor* compositor = NULL;
static struct xdg_wm_base* shell = NULL;
static int resize = 0;
static int readyToResize = 0;
static int quit = 0;
static int newWidth = 0;
static int newHeight = 0;

static void handleShellPing(void* data, struct xdg_wm_base* shell, uint32_t serial) {
    xdg_wm_base_pong(shell, serial);
}
const static struct xdg_wm_base_listener shellListener = {
    .ping = handleShellPing
};
static void handleRegistry(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        compositor = (wl_compositor *) wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        shell = (xdg_wm_base *) wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(shell, &shellListener, NULL);
    }
}
const struct wl_registry_listener registryListener = {
    .global = handleRegistry
};
static void handleShellSurfaceConfigure(void* data, struct xdg_surface* shellSurface, uint32_t serial) {
    xdg_surface_ack_configure(shellSurface, serial);
    if (resize) {
        readyToResize = 1;
    }
}
const struct xdg_surface_listener shellSurfaceListener = {
    .configure = handleShellSurfaceConfigure
};

static void handleToplevelConfigure(void* data, struct xdg_toplevel* toplevel, int32_t width, int32_t height, struct wl_array* states) {
    if (width != 0 && height != 0) {
        resize = 1;
        newWidth = width;
        newHeight = height;
    }
}
static void handleToplevelClose(void* data, struct xdg_toplevel* toplevel) {
    quit = 1;
}
const static struct xdg_toplevel_listener toplevelListener = {
    .configure = handleToplevelConfigure,
    .close = handleToplevelClose
};

class WaylandWindow {
public:
    struct wl_display* display = NULL;
    struct wl_registry* registry = NULL;
    
    struct wl_surface* surface = NULL;
    struct xdg_surface* shellSurface = NULL;
    struct xdg_toplevel* toplevel = NULL;
    const char* const appName = "Wayland Vulkan Example";

    //void (*)(void *data, xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, wl_array *states)
    //void (*)(void *data, xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, wl_array *states)
    

    void initSurface() {
        display = wl_display_connect(NULL);

        registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registryListener, NULL);
        wl_display_roundtrip(display);

        surface = wl_compositor_create_surface(compositor);

        shellSurface = xdg_wm_base_get_xdg_surface(shell, surface);
        xdg_surface_add_listener(shellSurface, &shellSurfaceListener, NULL);

        toplevel = xdg_surface_get_toplevel(shellSurface);
        xdg_toplevel_add_listener(toplevel, &toplevelListener, NULL);

        xdg_toplevel_set_title(toplevel, appName);
        xdg_toplevel_set_app_id(toplevel, appName);

        wl_surface_commit(surface);
        wl_display_roundtrip(display);
        wl_surface_commit(surface);

        wl_display_roundtrip(display);
    }
    VkSurfaceKHR getSurface(VkInstance instance) {
        VkWaylandSurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        createInfo.display = display;
        createInfo.surface = surface;

        VkSurfaceKHR vulkanSurface;
        
        vkCreateWaylandSurfaceKHR(instance, &createInfo, NULL, &vulkanSurface);
        return vulkanSurface;
    }
    void update() {
        wl_display_roundtrip(display);
    }
    void commit() {
        wl_surface_commit(surface);
    }
};