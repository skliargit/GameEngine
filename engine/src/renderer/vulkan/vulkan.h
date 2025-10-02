#pragma once

#include <core/defines.h>
#include <platform/window.h>

bool vulkan_backend_initialize(platform_window* window);
void vulkan_backend_shutdown();
bool vulkan_backend_is_supported();
void vulkan_backend_on_resize(const u32 width, const u32 height);
