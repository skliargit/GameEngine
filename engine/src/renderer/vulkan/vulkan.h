#pragma once

#include <core/defines.h>

bool vulkan_backend_initialize(const char* title, u32 width, u32 height);
void vulkan_backend_shutdown();
bool vulkan_backend_is_supported();
void vulkan_backend_on_resize(const u32 width, const u32 height);
