#pragma once

#include <core/defines.h>
#include <platform/window.h>

bool xcb_backend_create(const platform_window_config* config, void* internal_data);
void xcb_backend_destroy(void* internal_data);
bool xcb_backend_poll_events(void* internal_data);

bool xcb_backend_is_supported();
u64  xcb_backend_instance_memory_requirement();
