#pragma once

#include <core/defines.h>
#include <renderer/vulkan/vulkan_types.h>

/**/
bool vulkan_swapchain_create(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* out_swapchain);

/**/
void vulkan_swapchain_destroy(vulkan_context* context, vulkan_swapchain* swapchain);

/**/
bool vulkan_swapchain_recreate(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* swapchain);

/**/
void vulkan_swapchain_acquire_next_image_index();

/**/
void vulkan_swapchain_present();
