#include "renderer/vulkan/image.h"
#include "renderer/vulkan/result.h"
#include "renderer/vulkan/utils.h"

#include "core/logger.h"
#include "core/memory.h"

bool vulkan_image_create(
    vulkan_context* context, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
    VkMemoryPropertyFlags memory_property_flags, vulkan_image* out_image
)
{
    // Обнуление памяти.
    mzero(out_image, sizeof(vulkan_image));

    out_image->width = width;
    out_image->height = height;
    out_image->memory_property_flags = memory_property_flags;

    // TODO: Указать индексы очередей для совместного использования изображения.
    // NOTE: Изображение будет привязано к первому семейству очередей, которое его использует.
    VkImageCreateInfo image_info = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags         = 0,                         // TODO: Добавить конфигурацию флагов (например, для массивов изображений).
        .imageType     = VK_IMAGE_TYPE_2D,          // TODO: Добавить поддержку других типов изображений (3D, 1D).
        .format        = format,
        .extent.width  = width,
        .extent.height = height,
        .extent.depth  = 1,                         // TODO: Добавить конфигурацию глубины для 3D изображений.
        .mipLevels     = 1,                         // TODO: Добавить поддержку мипмаппинга.
        .arrayLayers   = 1,                         // TODO: Добавить поддержку массивов изображений.
        .samples       = VK_SAMPLE_COUNT_1_BIT,     // TODO: Добавить поддержку мультисэмплинга.
        .tiling        = tiling,
        .usage         = usage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE, // TODO: Добавить поддержку совместного использования между семействами очередей.
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkResult result = vkCreateImage(context->device.logical, &image_info, context->allocator, &out_image->handle);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create vulkan image: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Получение требований к памяти.
    vkGetImageMemoryRequirements(context->device.logical, out_image->handle, &out_image->memory_requirements);

    u32 memory_index = vulkan_util_find_memory_index(&context->device, out_image->memory_requirements.memoryTypeBits, memory_property_flags);
    if(memory_index == INVALID_ID32)
    {
        LOG_ERROR("Failed to find memory index for image.");
        return false;
    }

    // Выделение памяти.
    VkMemoryAllocateInfo memory_allocate_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = out_image->memory_requirements.size,
        .memoryTypeIndex = memory_index
    };

    result = vkAllocateMemory(context->device.logical, &memory_allocate_info, context->allocator, &out_image->memory);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to allocate memory for image: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Привязывание памяти.
    // TODO: Настраиваемое смещение.
    result = vkBindImageMemory(context->device.logical, out_image->handle, out_image->memory, 0);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to bind allocated memory to image: %s.", vulkan_result_get_string(result));
        return false;
    }

    return true;
}

bool vulkan_image_create_view(
    vulkan_context* context, VkFormat format, VkImageAspectFlags aspect_flags, vulkan_image* image
)
{
    if(image->handle == nullptr)
    {
        LOG_ERROR("Failed to create image view because image handle is missing.");
        return false;
    }

    VkImageViewCreateInfo image_view_info = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = image->handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = format,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask = aspect_flags,
        .subresourceRange.baseMipLevel = 0,            // TODO: Настраиваемый.
        .subresourceRange.levelCount = 1,              // TODO: Настраиваемый.
        .subresourceRange.baseArrayLayer = 0,          // TODO: Настраиваемый.
        .subresourceRange.layerCount = 1               // TODO: Настраиваемый.
    };

    VkResult result = vkCreateImageView(context->device.logical, &image_view_info, context->allocator, &image->view);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create image views: %s.", vulkan_result_get_string(result));
        return false;
    }

    return true;
}

// TODO: перенести в шейдер.
bool vulkan_image_create_sampler(vulkan_context* context, VkSampler* out_sampler)
{
    // TODO: Сделать полностью настраиваемой.
    VkSamplerCreateInfo sampler_info = {
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter               = VK_FILTER_LINEAR,                      // TODO: Заменить
        .minFilter               = VK_FILTER_LINEAR,                      // TODO: Заменить
        .addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // TODO: Заменить
        .addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // TODO: Заменить
        .addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // TODO: Заменить
        .anisotropyEnable        = VK_TRUE,                               // TODO: Должно быть в соответсвии с настроками устройства.
        .maxAnisotropy           = 16,
        .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable           = VK_FALSE,
        .compareOp               = VK_COMPARE_OP_ALWAYS,
        .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias              = 0.0f,
        .minLod                  = 0.0f,
        .maxLod                  = 0.0f,
    };

    VkResult result = vkCreateSampler(context->device.logical, &sampler_info, context->allocator, out_sampler);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create sampler: %s.", vulkan_result_get_string(result));
        return false;
    }

    return true;
}

void vulkan_image_destroy(vulkan_context* context, vulkan_image* image)
{
    if(image->view)
    {
        vkDestroyImageView(context->device.logical, image->view, context->allocator);
    }

    if(image->memory)
    {
        vkFreeMemory(context->device.logical, image->memory, context->allocator);
    }

    if(image->handle)
    {
        vkDestroyImage(context->device.logical, image->handle, context->allocator);
    }

    mzero(image, sizeof(vulkan_image));
}

void vulkan_image_transition_layout(VkCommandBuffer cmdbuf, vulkan_image_transition_t transition_op, VkImage* images)
{
    VkImageMemoryBarrier image_barriers[2] =
    {
        // Барьер буфера цвета
        {
            .sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,
            .image                           = images[0],
            .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel   = 0,
            .subresourceRange.levelCount     = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount     = 1
        },
        // Барьер буфера глубины
        {
            .sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,
            .image                           = images[1],
            .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
            .subresourceRange.baseMipLevel   = 0,
            .subresourceRange.levelCount     = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount     = 1
        }
    };

    u32 image_count = 1; // Количество изображений по умолчанию.
    VkPipelineStageFlags src_stage, dst_stage;

    switch(transition_op)
    {
        // Только для перевода макета изображения для записи в него данных.
        case VULKAN_IMAGE_TRANSITION_UNDEFINED_TO_TRANSFER_DST:
            // Для буфера цвета.
            image_barriers[0].srcAccessMask = VK_ACCESS_NONE;
            image_barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            image_barriers[0].oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            image_barriers[0].newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            // Стадии конвейера.
            src_stage                 = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage                 = VK_PIPELINE_STAGE_TRANSFER_BIT;

            // NOTE: Используется значение image_count по умолчанию.
            break;

        // Это гарантирует, что команды до барьера завершат копирование данных до того,
        // как команды после начнут исполнять стадии фрагментного шейдера.
        case VULKAN_IMAGE_TRANSITION_TRANSFER_DST_TO_SHADER_READ:
            // Для буфера цвета.
            image_barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            image_barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            image_barriers[0].oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            image_barriers[0].newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            // Стадии конвейера.
            src_stage                 = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dst_stage                 = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            // NOTE: Используется значение image_count по умолчанию.
            break;

        // Переводит буферы цвета и глубины для записи финально изображения и теста глубины соответственно.
        case VULKAN_IMAGE_TRANSITION_ATTACHMENTS_TO_RENDERING:
            // Для буфера цвета.
            image_barriers[0].srcAccessMask = VK_ACCESS_NONE;
            image_barriers[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            image_barriers[0].oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            image_barriers[0].newLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // Для буфера глубины.
            image_barriers[1].srcAccessMask = VK_ACCESS_NONE;
            image_barriers[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            image_barriers[1].oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            image_barriers[1].newLayout     = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

            // Стадии конвейера.
            src_stage                 = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage                 = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

            image_count = 2;
            break;

        // Переводит только буфер цвета для чтения и показа изображения на экране.
        case VULKAN_IMAGE_TRANSITION_ATTACHMENT_TO_PRESENT:
            // Для буфера цвета.
            image_barriers[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            image_barriers[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            image_barriers[0].oldLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            image_barriers[0].newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            // Стадии конвейера.
            src_stage                 = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dst_stage                 = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

            // NOTE: Используется значение image_count по умолчанию.
            break;

        // TODO: Перевод макета изображения в заданный пользователем.
        case VULKAN_IMAGE_TRANSITION_CUSTOM:
            LOG_ERROR("Not support custom transition layout yet.");
            return;
    }

    vkCmdPipelineBarrier(cmdbuf, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, image_count, image_barriers);
}

void vulkan_image_copy_from_buffer(VkCommandBuffer cmdbuf, vulkan_image* image, VkBuffer buffer)
{
    VkBufferImageCopy region = {
        .bufferOffset                    = 0,
        .bufferRowLength                 = 0,
        .bufferImageHeight               = 0,
        .imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel       = 0,
        .imageSubresource.baseArrayLayer = 0,
        .imageSubresource.layerCount     = 1,
        .imageOffset.x                   = 0,
        .imageOffset.y                   = 0,
        .imageOffset.z                   = 0,
        .imageExtent.width               = image->width,
        .imageExtent.height              = image->height,
        .imageExtent.depth               = 1
    };

    vkCmdCopyBufferToImage(cmdbuf, buffer, image->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}
