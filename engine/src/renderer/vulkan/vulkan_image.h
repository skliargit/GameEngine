#pragma once

#include <core/defines.h>
#include <renderer/vulkan/vulkan_types.h>

/**/
bool vulkan_image_create(
    vulkan_context* context, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
    VkMemoryPropertyFlags memory_flags, vulkan_image* out_image
);

/**/
bool vulkan_image_view_cretae(
    vulkan_context* context, VkFormat format, VkImageAspectFlags aspect_flags, vulkan_image* image
);

/**/
void vulkan_image_destroy(vulkan_context* context, vulkan_image* image);
