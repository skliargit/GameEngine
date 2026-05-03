#include "renderer/vulkan/command_buffer.h"
#include "renderer/vulkan/result.h"

#include "core/logger.h"

bool vulkan_command_buffer_allocate(vulkan_context* context, VkCommandPool pool, u32 buffer_count, VkCommandBuffer* out_buffers)
{
    VkCommandBufferAllocateInfo buffer_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,     // TODO: Сделать настраиваемым.
        .commandBufferCount = buffer_count
    };

    VkResult result = vkAllocateCommandBuffers(context->device.logical, &buffer_info, out_buffers);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to allocate command buffers: %s.", vulkan_result_get_string(result));
        return false;
    }

    return true;
}

void vulkan_command_buffer_free(vulkan_context* context, VkCommandPool pool, u32 buffer_count, VkCommandBuffer* buffers)
{
    vkFreeCommandBuffers(context->device.logical, pool, buffer_count, buffers);
}

bool vulkan_command_buffer_begin(VkCommandBuffer buffer, bool single_use, bool rendering_continue, bool simultaneous_use)
{
    VkCommandBufferBeginInfo buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0 // Указывает на использование командного буфера более одного раза.
    };

    if(single_use)
    {
        buffer_begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }

    if(rendering_continue)
    {
        buffer_begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    }

    if(simultaneous_use)
    {
        buffer_begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }

    VkResult result = vkBeginCommandBuffer(buffer, &buffer_begin_info);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to begin recording command buffer: %s.", vulkan_result_get_string(result));
        return false;
    }

    return true;
}

bool vulkan_command_buffer_end(VkCommandBuffer buffer)
{
    VkResult result = vkEndCommandBuffer(buffer);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to end recording command buffer: %s.", vulkan_result_get_string(result));
        return false;
    }

    return true;
}

bool vulkan_command_buffer_reset(VkCommandBuffer buffer)
{
    VkResult result = vkResetCommandBuffer(buffer, 0);   // TODO: Сделать настраиваемым.
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to reset command buffer: %s.", vulkan_result_get_string(result));
        return false;
    }

    return true;
}

bool vulkan_command_buffer_submit(
    u32 buffer_count, VkCommandBuffer* buffers, VkQueue queue, u32 wait_semaphores_count, VkSemaphore* wait_semaphores,
    VkPipelineStageFlags* wait_stages, u32 signal_semaphore_count, VkSemaphore* signal_semaphores, VkFence signal_fence
)
{
    VkSubmitInfo submit_info = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = wait_semaphores_count,
        .pWaitSemaphores      = wait_semaphores,
        .pWaitDstStageMask    = wait_stages,
        .signalSemaphoreCount = signal_semaphore_count,
        .pSignalSemaphores    = signal_semaphores,
        .commandBufferCount   = buffer_count,
        .pCommandBuffers      = buffers,
    };

    VkResult result = vkQueueSubmit(queue, 1, &submit_info, signal_fence);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to submit command buffer: %s.", vulkan_result_get_string(result));
        return false;
    }

    return true;
}
