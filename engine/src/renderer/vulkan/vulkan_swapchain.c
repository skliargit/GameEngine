#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_result.h"

#include "core/logger.h"
#include "core/containers/darray.h"

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

    // Выбор формата пикселей поверхности.
    swapchain->image_format = surface_formats[0];
    for(u32 i = 0; i < surface_format_count; ++i)
    {
        VkSurfaceFormatKHR surface_format = surface_formats[i];
        if(surface_format.format == VK_FORMAT_B8G8R8A8_UNORM && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            swapchain->image_format = surface_format;
            LOG_TRACE("Vulkan swapchain selected preferred format and color space.");
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
    swapchain->present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for(u32 i = 0; i < present_mode_count; ++i)
    {
        VkPresentModeKHR present_mode = present_modes[i];
        if(present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchain->present_mode = present_mode;
            LOG_TRACE("Vulkan swapchain selected preferred present mode.");
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

    LOG_TRACE("Vulkan swapchain image width  : %u (%u..%u).", swapchain->image_extent.width, image_extent_min.width, image_extent_max.width);
    LOG_TRACE("Vulkan swapchain image height : %u (%u..%u).", swapchain->image_extent.height, image_extent_min.height, image_extent_max.height);

    // Получение количества кадров.
    u32 image_count_min    = surface_capabilities.minImageCount;
    u32 image_count_max    = surface_capabilities.maxImageCount;
    swapchain->image_count = image_count_min + 1;

    // Усечение до поддерживаемых границ.
    if(image_count_max > 0 && swapchain->image_count > image_count_max)
    {
        swapchain->image_count = image_count_max;
        LOG_TRACE("Vulkan swapchain image count  : %u (range: %u..%u).", swapchain->image_count, image_count_min, image_count_max);
    }
    else
    {
        LOG_TRACE("Vulkan swapchain image count  : %u (min: %u).", swapchain->image_count, image_count_min);
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
        swapchain_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices   = nullptr;
        LOG_TRACE("Vulkan swapchain image sharing mode is VK_SHARING_MODE_EXCLUSIVE (preferred).");
    }
    else
    {
        swapchain_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices   = queue_families;
        LOG_TRACE("Vulkan swapchain image sharing mode is VK_SHARING_MODE_CONCURRENT.");
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

    // TODO: Резервировать.
    VkImage* images = darray_create_custom(VkImage, swapchain->image_count);
    swapchain->image_views = darray_create_custom(VkImageView, swapchain->image_count);
    result = vkGetSwapchainImagesKHR(logical, swapchain->handle, &swapchain->image_count, images);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to get vulkan swapchain images: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Создание видов (указывает на то как сипользовать изображение).
    for(u32 i = 0; i < swapchain->image_count; ++i)
    {
        VkImageViewCreateInfo image_view_info = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = images[i],
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

    darray_destroy(images);

    return true;
}

static void swapchain_destroy(vulkan_context* context, vulkan_swapchain* swapchain)
{
    if(swapchain->image_views)
    {
        for(u32 i = 0; i < swapchain->image_count; ++i)
        {
            vkDestroyImageView(context->device.logical, swapchain->image_views[i], context->allocator);
        }
        darray_destroy(swapchain->image_views);
    }

    if(swapchain->handle)
    {
        vkDestroySwapchainKHR(context->device.logical, swapchain->handle, context->allocator);
    }
}

bool vulkan_swapchain_create(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* out_swapchain)
{
    if(!swapchain_create(context, width, height, out_swapchain))
    {
        return false;
    }

    return true;
}

void vulkan_swapchain_destroy(vulkan_context* context, vulkan_swapchain* swapchain)
{
    swapchain_destroy(context, swapchain);
}

bool vulkan_swapchain_recreate(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* swapchain)
{
    swapchain_destroy(context, swapchain);
    if(!swapchain_create(context, width, height, swapchain))
    {
        return false;
    }

    return true;
}

void vulkan_swapchain_acquire_next_image_index()
{
    // vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex);
}

void vulkan_swapchain_present()
{
    // vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo);
}
