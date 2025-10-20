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
bool vulkan_swapchain_acquire_next_image_index(
    vulkan_context* context, vulkan_swapchain* swapchain, VkSemaphore image_available_semaphore, VkFence wait_fence,
    u64 timeout_ns, u32* out_image_index
);

/**/
void vulkan_swapchain_present(
    vulkan_context* context, vulkan_swapchain* swapchain, VkQueue present_queue, VkSemaphore render_complete_semaphore,
    u32 present_image_index
);
