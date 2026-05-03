#pragma once

#include <core/defines.h>
#include <renderer/vulkan/types.h>

/**
*/
bool vulkan_command_buffer_allocate(vulkan_context* context, VkCommandPool pool, u32 buffer_count, VkCommandBuffer* out_buffers);

/**
*/
void vulkan_command_buffer_free(vulkan_context* context, VkCommandPool pool, u32 buffer_count, VkCommandBuffer* buffers);

/**
*/
bool vulkan_command_buffer_begin(VkCommandBuffer buffer, bool single_use, bool rendering_continue, bool simultaneous_use);

/**
*/
bool vulkan_command_buffer_end(VkCommandBuffer buffer);

/**
*/
bool vulkan_command_buffer_reset(VkCommandBuffer buffer);

/**
*/
bool vulkan_command_buffer_submit(
    u32 buffer_count, VkCommandBuffer* buffers, VkQueue queue, u32 wait_semaphores_count, VkSemaphore* wait_semaphores,
    VkPipelineStageFlags* wait_stages, u32 signal_semaphore_count, VkSemaphore* signal_semaphores, VkFence signal_fence
);
