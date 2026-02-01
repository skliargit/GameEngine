#include "renderer/vulkan/vulkan_buffer.h"
#include "renderer/vulkan/vulkan_result.h"
#include "renderer/vulkan/vulkan_utils.h"

#include "core/logger.h"
#include "core/memory.h"
#include "debug/assert.h"

// Указывает использует ли буфер видеопамять (VRAM, доступная только GPU).
static bool buffer_is_device_local(vulkan_buffer* buffer)
{
    return (buffer->memory_property_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
}

// Указывает использует ли буфер оперативную память (RAM, доступна CPU и может быть доступна GPU).
static bool buffer_is_host_visible(vulkan_buffer* buffer)
{
    return (buffer->memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
}

// Указывает использует ли буфер согласованную память (RAM <-> VRAM, автоматически синхронизируемая).
// static bool buffer_is_host_coherent(vulkan_buffer* buffer)
// {
//     return (buffer->memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
// }

// Указывает использует ли буфер кэширумую память (RAM <- VRAM, ручная синхронизация).
// static bool buffer_is_host_cached(vulkan_buffer* buffer)
// {
//     return (buffer->memory_property_flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
// }

// Внутренняя функция копирования данных.
static bool buffer_copy_range(vulkan_context* context, VkBuffer src, u64 src_offset, VkBuffer dst, u64 dst_offset, u64 size)
{
    VkQueue queue = context->device.graphics_queue.handle;

    // TODO: Использование семафора.
    // Ожидание завершения операций в очереди.
    VkResult result = vkQueueWaitIdle(queue);
    if(!vulkan_result_is_success(result))
    {
        LOG_FATAL("Failed to wait queue operation: %s.", vulkan_result_get_string(result));
    }

    // Выделение временного командного буфера.
    VkCommandBufferAllocateInfo allocate_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = context->device.graphics_queue.command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    // TODO: Интерфейс командного буфера!
    // Создание временного буфера команд.
    VkCommandBuffer cmdbuf;
    result = vkAllocateCommandBuffers(context->device.logical, &allocate_info, &cmdbuf);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to allocate command buffers: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Начало записи команд во временный буфер команд.
    VkCommandBufferBeginInfo cmdbuf_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // Указывает на однократное использование.
    };

    result = vkBeginCommandBuffer(cmdbuf, &cmdbuf_begin_info);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to start recording to the command buffer: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Запись команды для копирования региона памяти.
    VkBufferCopy copy_region;
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    copy_region.size = size;
    vkCmdCopyBuffer(cmdbuf, src, dst, 1, &copy_region);

    // Завершение записи команд во временный буфер команд.
    result = vkEndCommandBuffer(cmdbuf);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to end recording to the command buffer: %s.", vulkan_result_get_string(result));
        return false;
    }

    // TODO: Встроить семафоры.
    // Отправка на выполнение временного буфера команд.
    VkSubmitInfo cmdbuf_submit_info = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        // .waitSemaphoreCount   = ,
        // .pWaitSemaphores      = ,
        // .pWaitDstStageMask    = ,
        // .signalSemaphoreCount = ,
        // .pSignalSemaphores    = ,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &cmdbuf,
    };

    result = vkQueueSubmit(queue, 1, &cmdbuf_submit_info, nullptr);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to submit queue: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Ожидание окончания операции.
    result = vkQueueWaitIdle(queue);
    if(!vulkan_result_is_success(result))
    {
        LOG_FATAL("Failed to wait idle of queue: %s", vulkan_result_get_string(result));
    }

    // Освобождение буфера команд.
    vkFreeCommandBuffers(context->device.logical, context->device.graphics_queue.command_pool, 1, &cmdbuf);

    return true;
}

bool vulkan_buffer_create(vulkan_context* context, vulkan_buffer_type type, u64 size, vulkan_buffer* out_buffer)
{
    // Очистка перед использованием.
    mzero(out_buffer, sizeof(vulkan_buffer));
    out_buffer->type = type;
    out_buffer->size = size;

    // Установка флагов использования буфера и его флаги свойств памяти.
    switch(type)
    {
        case VULKAN_BUFFER_TYPE_VERTEX:
            out_buffer->usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            out_buffer->memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        case VULKAN_BUFFER_TYPE_INDEX:
            out_buffer->usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            out_buffer->memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        case VULKAN_BUFFER_TYPE_STAGING:
            out_buffer->usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            out_buffer->memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;
        case VULKAN_BUFFER_TYPE_UNIFORM:
            out_buffer->usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            out_buffer->memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            out_buffer->memory_property_flags |= context->device.supports_host_local_memory ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
            break;
        case VULKAN_BUFFER_TYPE_READ:
        case VULKAN_BUFFER_TYPE_STORAGE:
        default:
            LOG_ERROR("Buffer type %u is not yet supported.", type);
            return false;
    }

    VkBufferCreateInfo buffer_info = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = out_buffer->usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE // TODO: Добавить поддержку совместного использования между семействами очередей.
    };

    VkResult result = vkCreateBuffer(context->device.logical, &buffer_info, context->allocator, &out_buffer->handle);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create buffer type %u: %s.", type, vulkan_result_get_string(result));
        return false;
    }

    // Получение требований памяти.
    vkGetBufferMemoryRequirements(context->device.logical, out_buffer->handle, &out_buffer->memory_requirements);

    // Получение индекса типа памяти.
    out_buffer->memory_index = vulkan_util_find_memory_index(&context->device, out_buffer->memory_requirements.memoryTypeBits, out_buffer->memory_property_flags);
    if(out_buffer->memory_index == INVALID_ID32)
    {
        LOG_ERROR("Failed to find memory index for buffer type %u.", type);
        return false;
    }

    // Выделение памяти.
    VkMemoryAllocateInfo memory_allocate_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = out_buffer->memory_requirements.size,
        .memoryTypeIndex = out_buffer->memory_index
    };

    result = vkAllocateMemory(context->device.logical, &memory_allocate_info, context->allocator, &out_buffer->memory);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to allocate memory for buffer type %u: %s.", type, vulkan_result_get_string(result));
        return false;
    }

    // TODO: Выделять большой кусок и выполнять связывание с ним буферов.
    result = vkBindBufferMemory(context->device.logical, out_buffer->handle, out_buffer->memory, 0);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to bind memory buffer of type %u: %s.", type, vulkan_result_get_string(result));
        return false;
    }

    return true;
}

void vulkan_buffer_destroy(vulkan_context* context, vulkan_buffer* buffer)
{
    if(buffer->memory != nullptr)
    {
        vkFreeMemory(context->device.logical, buffer->memory, context->allocator);
    }

    if(buffer->handle != nullptr)
    {
        vkDestroyBuffer(context->device.logical, buffer->handle, context->allocator);
    }

    // Обнуление всех указателей и данных.
    mzero(buffer, sizeof(vulkan_buffer));
}

bool vulkan_buffer_resize(vulkan_context* context, vulkan_buffer* buffer, u64 new_size)
{
    // TODO: Размер больше размера текущего буфера и тем более нуля.
    ASSERT(new_size > 0, "New size must be greater than zero.");

    if(new_size == buffer->size)
    {
        return true;
    }

    // Создание нового буфера.
    VkBufferCreateInfo buffer_info = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = new_size,
        .usage       = buffer->usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE // TODO: Добавить поддержку совместного использования между семействами очередей.
    };

    VkBuffer new_buffer = nullptr;
    VkResult result = vkCreateBuffer(context->device.logical, &buffer_info, context->allocator, &new_buffer);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to resize buffer type %u: %s.", buffer->type, vulkan_result_get_string(result));
        return false;
    }

    // Получение новых требований памяти.
    VkMemoryRequirements new_memory_requirements;
    vkGetBufferMemoryRequirements(context->device.logical, new_buffer, &new_memory_requirements);

    // Выделение новой памяти.
    VkMemoryAllocateInfo memory_allocate_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = new_memory_requirements.size,
        .memoryTypeIndex = buffer->memory_index
    };

    VkDeviceMemory new_memory = nullptr;
    result = vkAllocateMemory(context->device.logical, &memory_allocate_info, context->allocator, &new_memory);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to allocate memory to resize buffer of type %u: %s.", buffer->type, vulkan_result_get_string(result));
        return false;
    }

    // Привязывание новой памяти для выполнения операций с ней.
    result = vkBindBufferMemory(context->device.logical, new_buffer, new_memory, 0);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to bind memory to resize buffer of type %u: %s.", buffer->type, vulkan_result_get_string(result));
        return false;
    }

    // Копирование данных из старой памяти в новую.
    if(!buffer_copy_range(context, buffer->handle, 0, new_buffer, 0, buffer->size))
    {
        LOG_ERROR("Failed to copy data during buffer resize.");
        return false;
    }

    // Освобождение памяти старого буфера.
    if(buffer->memory != nullptr)
    {
        vkFreeMemory(context->device.logical, buffer->memory, context->allocator);
    }

    // Уничтожение старого буфера.
    if(buffer->handle != nullptr)
    {
        vkDestroyBuffer(context->device.logical, buffer->handle, context->allocator);
    }

    // Обновление указателей и данных буфера.
    buffer->size = new_size;
    buffer->handle = new_buffer;
    buffer->memory = new_memory;
    buffer->memory_requirements = new_memory_requirements;

    return true;
}

bool vulkan_buffer_map_memory(vulkan_context* context, vulkan_buffer* buffer, u64 offset, u64 size, void** data)
{
    VkResult result = vkMapMemory(context->device.logical, buffer->memory, offset, size, 0, data);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to map memory buffer of type %u: %s.", buffer->type, vulkan_result_get_string(result));
        return false;
    }

    return true;
}

void vulkan_buffer_unmap_memory(vulkan_context* context, vulkan_buffer* buffer)
{
    vkUnmapMemory(context->device.logical, buffer->memory);
}

bool vulkan_buffer_load_data(vulkan_context* context, vulkan_buffer* buffer, u64 offset, u64 size, const void* data)
{
    if(offset + size > buffer->size)
    {
        LOG_ERROR("Data exceeds buffer bounds.");
        return false;
    }

    // Копирование с промежуточным буфером.
    if(buffer_is_device_local(buffer) && !buffer_is_host_visible(buffer))
    {
        // Создание промежуточного буфера для загрузки в видеопамять.
        vulkan_buffer staging;
        if(!vulkan_buffer_create(context, VULKAN_BUFFER_TYPE_STAGING, size, &staging))
        {
            LOG_ERROR("Failed to create staging buffer.");
            return false;
        }

        // Загрузка данных в промежуточный буфер.
        vulkan_buffer_load_data(context, &staging, 0, size, data);

        // Копирование из промежуточного буфер в видеопамять.
        vulkan_buffer_copy_range(context, &staging, 0, buffer, offset, size);

        // Уничтожение промежуточного буфера.
        vulkan_buffer_destroy(context, &staging);
    }
    // Копирование без промежуточного буфера.
    else
    {
        void* mapped_data = nullptr;
        if(!vulkan_buffer_map_memory(context, buffer, offset, size, &mapped_data))
        {
            return false;
        }

        mcopy(mapped_data, data, size);
        vulkan_buffer_unmap_memory(context, buffer);
    }

    return true;
}

bool vulkan_buffer_copy_range(vulkan_context* context, vulkan_buffer* src, u64 src_offset, vulkan_buffer* dst, u64 dst_offset, u64 size)
{
    return buffer_copy_range(context, src->handle, src_offset, dst->handle, dst_offset, size);
}
