#pragma once

#include <core/defines.h>
#include <platform/window.h>

bool wayland_backend_initialize(void* internal_data);
void wayland_backend_shutdown(void* internal_data);
bool wayland_backend_poll_events(void* internal_data);
bool wayland_backend_is_supported();
u64  wayland_backend_memory_requirement();

platform_window* wayland_backend_window_create(const platform_window_config* config, void* internal_data);
void wayland_backend_window_destroy(platform_window* window, void* internal_data);
