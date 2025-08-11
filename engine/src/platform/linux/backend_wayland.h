#pragma once

#include <core/defines.h>
#include <platform/window.h>

bool wayland_backend_create(const platform_window_config* config, void* internal_data);
void wayland_backend_destroy(void* internal_data);
bool wayland_backend_poll_events(void* internal_data);

bool wayland_backend_is_supported();
u64  wayland_backend_instance_memory_requirement();
