#pragma once

#include <renderer/vulkan/vulkan_types.h>

/**/
u32 vulkan_util_find_memory_index(vulkan_device* device, u32 type_bits, VkMemoryPropertyFlags property_flags);
