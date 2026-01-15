#include "renderer/vulkan/vulkan_image.h"
#include "renderer/vulkan/vulkan_result.h"
#include "renderer/vulkan/vulkan_utils.h"

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

bool vulkan_image_view_cretae(
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
