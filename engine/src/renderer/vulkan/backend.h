#pragma once

#include <core/defines.h>
#include <renderer/types.h>
#include <platform/window.h>

bool vulkan_backend_initialize(platform_window* window);
void vulkan_backend_shutdown();
bool vulkan_backend_is_supported();
void vulkan_backend_resize(const u32 width, const u32 height);

bool vulkan_backend_frame_begin();
bool vulkan_backend_frame_end();

void vulkan_backend_update_camera(const mat4* proj, const mat4* view);
void vulkan_backend_update_model(const mat4* model);
