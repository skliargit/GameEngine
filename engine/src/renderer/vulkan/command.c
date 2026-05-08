#include "renderer/vulkan/command.h"
#include "renderer/vulkan/result.h"

#include "core/logger.h"
#include "core/memory.h"

bool vulkan_command_manager_create(vulkan_context* context, VkQueue queue, u32 queue_family_index, vulkan_command_manager_t* out_manager)
{
    out_manager->device = context->device.logical;
    out_manager->queue  = queue;

    VkCommandPoolCreateInfo command_pool_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // TODO: Настраиваемый. Сейчас только для кадров!
        .queueFamilyIndex = queue_family_index,
    };

    VkResult result = vkCreateCommandPool(context->device.logical, &command_pool_info, context->allocator, &out_manager->pool);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create command pool for command manager: %s.", vulkan_result_get_string(result));
        return false;
    }

    return true;
}

void vulkan_command_manager_destroy(vulkan_context* context, vulkan_command_manager_t* manager)
{
    vkDestroyCommandPool(context->device.logical, manager->pool, context->allocator);
    mzero(manager, sizeof(vulkan_command_manager_t));
}

void vulkan_command_manager_wait_idle(vulkan_command_manager_t* manager)
{
    VkResult result = vkQueueWaitIdle(manager->queue);
    if(!vulkan_result_is_success(result))
    {
        LOG_FATAL("Failed to wait idle of queue for command manager: %s.", vulkan_result_get_string(result));
    }
}

void vulkan_command_buffer_create(vulkan_command_manager_t* manager, u32 buffer_count, VkCommandBuffer* buffers)
{
    VkCommandBufferAllocateInfo buffer_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = manager->pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,     // TODO: Сделать настраиваемым.
        .commandBufferCount = buffer_count
    };

    VkResult result = vkAllocateCommandBuffers(manager->device, &buffer_info, buffers);
    if(!vulkan_result_is_success(result))
    {
        LOG_FATAL("Failed to create command buffers: %s.", vulkan_result_get_string(result));
    }
}

void vulkan_command_buffer_destroy(vulkan_command_manager_t* manager, u32 buffer_count, VkCommandBuffer* buffers)
{
    vkFreeCommandBuffers(manager->device, manager->pool, buffer_count, buffers);
    mzero(buffers, sizeof(VkCommandBuffer) * buffer_count);
}

void vulkan_command_buffer_submit(
    vulkan_command_manager_t* manager, u32 buffer_count, VkCommandBuffer* buffers, u32 wait_semaphores_count, VkSemaphore* wait_semaphores,
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

    VkResult result = vkQueueSubmit(manager->queue, 1, &submit_info, signal_fence);
    if(!vulkan_result_is_success(result))
    {
        LOG_FATAL("Failed to submit command buffers: %s.", vulkan_result_get_string(result));
    }
}

void vulkan_command_buffer_begin(VkCommandBuffer buffer, VkCommandBufferUsageFlags usage)
{
    VkCommandBufferBeginInfo buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = usage // NOTE: Ноль указывает на использование командного буфера более одного раза.
    };

    VkResult result = vkBeginCommandBuffer(buffer, &buffer_begin_info);
    if(!vulkan_result_is_success(result))
    {
        LOG_FATAL("Failed to begin recording command buffer: %s.", vulkan_result_get_string(result));
    }
}

void vulkan_command_buffer_end(VkCommandBuffer buffer)
{
    VkResult result = vkEndCommandBuffer(buffer);
    if(!vulkan_result_is_success(result))
    {
        LOG_FATAL("Failed to end recording command buffer: %s.", vulkan_result_get_string(result));
    }
}

void vulkan_command_buffer_reset(VkCommandBuffer buffer)
{
    VkResult result = vkResetCommandBuffer(buffer, 0);   // TODO: Сделать настраиваемым.
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to reset command buffer: %s.", vulkan_result_get_string(result));
    }
}

void vulkan_command_buffer_begin_single_use(vulkan_command_manager_t* manager, VkCommandBuffer* buffer)
{
    vulkan_command_manager_wait_idle(manager);
    vulkan_command_buffer_create(manager, 1, buffer);
    vulkan_command_buffer_begin(*buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

void vulkan_command_buffer_end_single_use(vulkan_command_manager_t* manager, VkCommandBuffer buffer)
{
    vulkan_command_buffer_end(buffer);
    vulkan_command_buffer_submit(manager, 1, &buffer, 0, nullptr, nullptr, 0, nullptr, nullptr);
    vulkan_command_manager_wait_idle(manager);
    vulkan_command_buffer_destroy(manager, 1, &buffer);
}
