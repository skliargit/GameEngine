#pragma once

#include <core/defines.h>
#include <renderer/vulkan/types.h>

/**
*/
bool vulkan_image_create(
    vulkan_context* context, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
    VkMemoryPropertyFlags memory_property_flags, vulkan_image* out_image
);

/**
*/
bool vulkan_image_create_view(vulkan_context* context, VkFormat format, VkImageAspectFlags aspect_flags, vulkan_image* image);

/**
*/
// TODO: перенести в шейдер.
bool vulkan_image_create_sampler(vulkan_context* context, VkSampler* out_sampler);

/**
*/
void vulkan_image_destroy(vulkan_context* context, vulkan_image* image);

/**
*/
void vulkan_image_transition_layout(VkCommandBuffer cmdbuf, vulkan_image_transition_t transition_op, VkImage* images);

/**
*/
void vulkan_image_copy_from_buffer(VkCommandBuffer cmdbuf, vulkan_image* image, VkBuffer buffer);
