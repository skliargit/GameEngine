#pragma once

#include <core/defines.h>
#include <platform/window.h>

bool xcb_backend_initialize(void* internal_data);
void xcb_backend_shutdown(void* internal_data);
bool xcb_backend_poll_events(void* internal_data);
bool xcb_backend_is_supported();
u64  xcb_backend_memory_requirement();

platform_window* xcb_backend_window_create(const platform_window_config* config, void* internal_data);
void xcb_backend_window_destroy(platform_window* window, void* internal_data);
