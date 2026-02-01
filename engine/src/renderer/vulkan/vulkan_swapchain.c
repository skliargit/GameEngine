#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_result.h"
#include "renderer/vulkan/vulkan_image.h"

#include "core/logger.h"
#include "core/memory.h"
#include "core/containers/darray.h"

// Получение форрмата буфер глубины.
typedef struct depth_format {
    VkFormat format;
    u8 channel_count;
} depth_format;

static bool swapchain_create(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* swapchain)
{
    VkPhysicalDevice physical = context->device.physical;
    VkSurfaceKHR surface = context->surface;
    VkDevice logical = context->device.logical;

    // Запрос количество поддерживаемых форматов пикселей для заданной поверхности.
    u32 surface_format_count = 0;
    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &surface_format_count, nullptr);
    if(!vulkan_result_is_success(result) || surface_format_count == 0)
    {
        LOG_ERROR("Failed to get surface format count: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Получение форматов пикселей для заданной поверхности.
    VkSurfaceFormatKHR* surface_formats = darray_create_custom(VkSurfaceFormatKHR, surface_format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &surface_format_count, surface_formats);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to get surface formats: %s.", vulkan_result_get_string(result));
        darray_destroy(surface_formats);
        return false;
    }

    LOG_TRACE("----------------------------------------------------------");
    LOG_TRACE("Vulkan swapchain configuration:");
    LOG_TRACE("----------------------------------------------------------");

    // Выбор формата пикселей поверхности.
    swapchain->image_format = surface_formats[0];
    for(u32 i = 0; i < surface_format_count; ++i)
    {
        // TODO: ?
        // VkFormatProperties format_props;
        // vkGetPhysicalDeviceFormatProperties(context->device.physical_device, color_format, &format_props);
        // if(!(format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)) {
        //     LOG_ERROR("Color format not supported for rendering");
        //     return false;
        // }

        VkSurfaceFormatKHR surface_format = surface_formats[i];
        if(surface_format.format == VK_FORMAT_B8G8R8A8_UNORM && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            swapchain->image_format = surface_format;
            LOG_TRACE("  Surface format      : VK_FORMAT_B8G8R8A8_UNORM");
            LOG_TRACE("  Surface color space : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR");
            break;
        }
    }

    // Освобождение памяти форматов пикселей поверхности.
    darray_destroy(surface_formats);

    // Запрос количество поддерживаемых режимов представления для заданной поверхности.
    u32 present_mode_count = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &present_mode_count, nullptr);
    if(!vulkan_result_is_success(result) || present_mode_count == 0)
    {
        LOG_ERROR("Failed to get surface present mode count: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Получение режимов представления для заданной поверхности.
    VkPresentModeKHR* present_modes = darray_create_custom(VkPresentModeKHR, present_mode_count);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &present_mode_count, present_modes);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to get surface present modes: %s.", vulkan_result_get_string(result));
        darray_destroy(present_modes);
        return false;
    }

    // Выбор режима представления поверхности.
    // NOTE: Выбран гарантированный спецификацией режим презентации.
    swapchain->present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for(u32 i = 0; i < present_mode_count; ++i)
    {
        VkPresentModeKHR present_mode = present_modes[i];
        if(present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchain->present_mode = present_mode;
            LOG_TRACE("  Present mode        : VK_PRESENT_MODE_MAILBOX_KHR");
            break;
        }
    }

    // Освобождение памяти режимов представления поверхности.
    darray_destroy(present_modes);

    // Запрос основных возможностей поверхности, необходимых для создания цепочки обмена.
    VkSurfaceCapabilitiesKHR surface_capabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &surface_capabilities);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to get surface capabilities: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Получение размеров кадра.
    VkExtent2D image_extent_min = surface_capabilities.minImageExtent;
    VkExtent2D image_extent_max = surface_capabilities.maxImageExtent;

    // Усечение до поддерживаемых границ.
    // TODO: Что для linux и window добавить принудительную обработку сообщений окна.
    // NOTE: В windows минимальные и максимальные границы окна равны текущему размеру окна.
    swapchain->image_extent.width  = CLAMP(width, image_extent_min.width, image_extent_max.width);
    swapchain->image_extent.height = CLAMP(height, image_extent_min.height, image_extent_max.height);

    LOG_TRACE("  Image width         : %u (%u..%u)", swapchain->image_extent.width, image_extent_min.width, image_extent_max.width);
    LOG_TRACE("  Image height        : %u (%u..%u)", swapchain->image_extent.height, image_extent_min.height, image_extent_max.height);

    // Получение количества кадров.
    u32 image_count_min    = surface_capabilities.minImageCount;
    u32 image_count_max    = surface_capabilities.maxImageCount;
    swapchain->image_count = image_count_min + 1;

    // Усечение до поддерживаемых границ.
    if(image_count_max > 0 && swapchain->image_count > image_count_max)
    {
        swapchain->image_count = image_count_max;
        LOG_TRACE("  Image count         : %u (%u..%u)", swapchain->image_count, image_count_min, image_count_max);
    }
    else
    {
        LOG_TRACE("  Image count         : %u (%u..inf)", swapchain->image_count, image_count_min);
    }

    // Преобразования к изображениям по умолчанию.
    swapchain->image_transform = surface_capabilities.currentTransform;

    // Создание цепочки обмена.
    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = context->surface,
        .minImageCount         = swapchain->image_count,
        .imageFormat           = swapchain->image_format.format,
        .imageColorSpace       = swapchain->image_format.colorSpace,
        .imageExtent           = swapchain->image_extent,
        .imageArrayLayers      = 1,                                      // Для нестереоскопических изображений всегда 1.
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform          = swapchain->image_transform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,      // Игнорирование альфа-канала.
        .presentMode           = swapchain->present_mode,
        .clipped               = VK_TRUE,                                // Предотвращает рендер перекрытых участков окна.
        .oldSwapchain          = nullptr,
    };

    // Семейства очередей использующих цепочку обмена.
    const u32 queue_families[2] = { context->device.graphics_queue.family_index, context->device.present_queue.family_index };

    if(context->device.graphics_queue.family_index == context->device.present_queue.family_index)
    {
        // NOTE: Предпочитаемый, не требует дополнительной синхронизации.
        swapchain_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices   = nullptr;
        LOG_TRACE("  Image sharing mode  : VK_SHARING_MODE_EXCLUSIVE");
    }
    else
    {
        swapchain_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices   = queue_families;
        LOG_TRACE("  Image sharing mode  : VK_SHARING_MODE_CONCURRENT");
    }

    result = vkCreateSwapchainKHR(logical, &swapchain_info, context->allocator, &swapchain->handle);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create vulkan swapchain: %s.", vulkan_result_get_string(result));
        return false;
    }

    result = vkGetSwapchainImagesKHR(logical, swapchain->handle, &swapchain->image_count, nullptr);
    if(!vulkan_result_is_success(result) || swapchain->image_count == 0)
    {
        LOG_ERROR("Failed to get vulkan swapchain image count: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Создание массива для хранения изобрадений.
    swapchain->images = darray_create_custom(VkImage, swapchain->image_count);

    result = vkGetSwapchainImagesKHR(logical, swapchain->handle, &swapchain->image_count, swapchain->images);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to get vulkan swapchain images: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Создание видов (указывает на то как ипользовать изображение).
    swapchain->image_views = darray_create_custom(VkImageView, swapchain->image_count);

    for(u32 i = 0; i < swapchain->image_count; ++i)
    {
        VkImageViewCreateInfo image_view_info = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = swapchain->images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = swapchain->image_format.format,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1
        };

        result = vkCreateImageView(logical, &image_view_info, context->allocator, &swapchain->image_views[i]);
        if(!vulkan_result_is_success(result))
        {
            LOG_ERROR("Failed to create swapchain image views: %s.", vulkan_result_get_string(result));
            return false;
        }
    }

    // Количество изображений цепочки обмена которые могут находиться в обработке, так же известны как кадры (обычно 2-3).
    // TODO: Должно учитывать количество кадров исходя из количества изображений цепочки обмена.
    swapchain->max_frames_in_flight = 2;

    // Установка текущего кадр.
    swapchain->current_frame = 0;

    // TODO: Неправильное использование буфера глубины!!! Должно быть по одному на кард в обработке!!!!
    static const char* depth_format_strings[] = {
        "VK_FORMAT_D32_SFLOAT",
        "VK_FORMAT_D32_SFLOAT_S8_UINT",
        "VK_FORMAT_D24_UNORM_S8_UINT"
    };

    static const depth_format depth_formats[] = {
        { VK_FORMAT_D32_SFLOAT        , 4 },
        // TODO: Stencil!
        // { VK_FORMAT_D32_SFLOAT_S8_UINT, 4 },
        // { VK_FORMAT_D24_UNORM_S8_UINT,  3 },
    };

    static const u32 depth_format_count = ARRAY_SIZE(depth_formats);
    static const VkFormatFeatureFlags flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    u32 depth_format_index = INVALID_ID32;
    for(u32 i = 0; i < depth_format_count; ++i)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physical, depth_formats[i].format, &properties);

        if((properties.linearTilingFeatures & flags) == flags || (properties.optimalTilingFeatures & flags) == flags)
        {
            depth_format_index = i;
            break;
        }
    }

    if(depth_format_index == INVALID_ID32)
    {
        LOG_ERROR("Failed to find a supported depth format.");
        return false;
    }

    swapchain->depth_format = depth_formats[depth_format_index].format;
    swapchain->depth_channel_count = depth_formats[depth_format_index].channel_count;
    LOG_TRACE("  Depth format        : %s", depth_format_strings[depth_format_index]);
    LOG_TRACE("  Depth channel count : %hhu", swapchain->depth_channel_count);

    if(!vulkan_image_create(
        context, width, height, swapchain->depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &swapchain->depth_image
    ))
    {
        LOG_ERROR("Failed to create swapchain depth image.");
        return false;
    }

    if(!vulkan_image_view_cretae(context, swapchain->depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, &swapchain->depth_image))
    {
        LOG_ERROR("Failed to create swapchain depth image views.");
        return false;
    }

    LOG_TRACE("  Max frame in flight : %u", swapchain->max_frames_in_flight);
    LOG_TRACE("----------------------------------------------------------");

    return true;
}

static void swapchain_destroy(vulkan_context* context, vulkan_swapchain* swapchain)
{
    // NOTE: См. https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
    //       vkDeviceWaitIdle или vkQueueWaitIdle не может гарантировать безопасность удаления ресурсов swapchain.
    // TODO: Рассмотреть использование расширения VK_EXT_swapchain_maintenance1 для решения проблемы vkQueuePresentKHR
    //       и vkDeviceWaitIdle (vkQueueWaitIdle) при уничтожении swapchain.

    vulkan_image_destroy(context, &swapchain->depth_image);

    if(swapchain->image_views)
    {
        for(u32 i = 0; i < swapchain->image_count; ++i)
        {
            vkDestroyImageView(context->device.logical, swapchain->image_views[i], context->allocator);
        }

        darray_destroy(swapchain->image_views);
    }

    if(swapchain->images)
    {
        darray_destroy(swapchain->images);
    }

    if(swapchain->handle)
    {
        vkDestroySwapchainKHR(context->device.logical, swapchain->handle, context->allocator);
    }

    mzero(swapchain, sizeof(vulkan_swapchain));
}

bool vulkan_swapchain_create(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* out_swapchain)
{
    return swapchain_create(context, width, height, out_swapchain);
}

void vulkan_swapchain_destroy(vulkan_context* context, vulkan_swapchain* swapchain)
{
    if(swapchain == nullptr || swapchain->handle == nullptr)
    {
        LOG_WARN("Some resources were not allocated, skipping...");
        return;
    }

    swapchain_destroy(context, swapchain);
}

bool vulkan_swapchain_recreate(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* swapchain)
{
    // NOTE: Т.к. в момент показа может измениться размер окна, перед изменением цепочки обмена нужно дождаться
    //       завершения операций на GPU и только после этого изменять цепочку обмена.
    VkResult result = vkDeviceWaitIdle(context->device.logical);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed wait device idle: %s.", vulkan_result_get_string(result));
        return false;
    }

    swapchain_destroy(context, swapchain);
    return swapchain_create(context, width, height, swapchain);
}

bool vulkan_swapchain_acquire_next_image_index(
    vulkan_context* context, vulkan_swapchain* swapchain, VkSemaphore image_available_semaphore, VkFence wait_fence,
    u64 timeout_ns, u32* out_image_index
)
{
    VkResult result = vkAcquireNextImageKHR(
        context->device.logical, swapchain->handle, timeout_ns, image_available_semaphore, wait_fence, out_image_index
    );

    if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        LOG_DEBUG("Recreate swapchain: %s.", vulkan_result_get_string(result));
        vulkan_swapchain_recreate(context, context->frame_width, context->frame_height, swapchain);
        return false;
    }
    else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        LOG_ERROR("Failed to acquire swapchain next image index: %s.", vulkan_result_get_string(result));
        return false;
    }

    return true;
}

void vulkan_swapchain_present(
    vulkan_context* context, vulkan_swapchain* swapchain, VkQueue present_queue, VkSemaphore image_complete_semaphore,
    u32 present_image_index
)
{
    VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount     = 1,
        .pSwapchains        = &swapchain->handle,
        .pImageIndices      = &present_image_index,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &image_complete_semaphore,
        .pResults           = nullptr
    };

    VkResult result = vkQueuePresentKHR(present_queue, &present_info);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        LOG_DEBUG("Recreate swapchain: %s.", vulkan_result_get_string(result));
        vulkan_swapchain_recreate(context, context->frame_width, context->frame_height, swapchain);
    }
    else if(result != VK_SUCCESS)
    {
        LOG_FATAL("Failed to present swapchain image!", __FUNCTION__);
    }

    // NOTE: Оптимальная версия, т.к. операция взятия остатка очень тяжелая!
    swapchain->current_frame++;
    swapchain->current_frame = swapchain->current_frame >= swapchain->max_frames_in_flight ? 0 : swapchain->current_frame;
}
