#pragma once

#include <core/defines.h>
#include <renderer/vulkan/types.h>

bool vulkan_command_manager_create(vulkan_context* context, VkQueue queue, u32 queue_family_index, vulkan_command_manager_t* out_manager);

void vulkan_command_manager_destroy(vulkan_context* context, vulkan_command_manager_t* manager);

void vulkan_command_manager_wait_idle(vulkan_command_manager_t* manager);

void vulkan_command_buffer_create(vulkan_command_manager_t* manager, u32 buffer_count, VkCommandBuffer* buffers);

void vulkan_command_buffer_destroy(vulkan_command_manager_t* manager, u32 buffer_count, VkCommandBuffer* buffers);

void vulkan_command_buffer_submit(
    vulkan_command_manager_t* manager, u32 buffer_count, VkCommandBuffer* buffers, u32 wait_semaphores_count, VkSemaphore* wait_semaphores,
    VkPipelineStageFlags* wait_stages, u32 signal_semaphore_count, VkSemaphore* signal_semaphores, VkFence signal_fence
);

void vulkan_command_buffer_begin(VkCommandBuffer buffer, VkCommandBufferUsageFlags usage);

void vulkan_command_buffer_end(VkCommandBuffer buffer);

void vulkan_command_buffer_reset(VkCommandBuffer buffer);

void vulkan_command_buffer_begin_single_use(vulkan_command_manager_t* manager, VkCommandBuffer* buffer);

void vulkan_command_buffer_end_single_use(vulkan_command_manager_t* manager, VkCommandBuffer buffer);
