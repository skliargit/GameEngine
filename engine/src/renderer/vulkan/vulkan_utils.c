#include "renderer/vulkan/vulkan_utils.h"

u32 vulkan_util_find_memory_index(vulkan_device* device, u32 type_bits, VkMemoryPropertyFlags property_flags)
{
    for(u32 i = 0; i < device->memory_properties.memoryTypeCount; ++i)
    {
        // Проверка каждого типа памяти на наличие запрашиваемых свойств.
        VkMemoryPropertyFlags current_property_flags = device->memory_properties.memoryTypes[i].propertyFlags & property_flags;
        if(type_bits & (1 << i) && current_property_flags == property_flags)
        {
            return i;
        }
    }

    return INVALID_ID32;
}
