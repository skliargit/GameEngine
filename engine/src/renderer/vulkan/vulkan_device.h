#pragma once

#include <core/defines.h>
#include <renderer/vulkan/vulkan_types.h>

/**/
bool vulkan_device_create(vulkan_context* context, vulkan_physical_device* physical, vulkan_device_config* config, vulkan_device* out_device);

/**/
void vulkan_device_destroy(vulkan_context* context, vulkan_device* device);

/**/
bool vulkan_device_enumerate_physical_devices(vulkan_context* context, u32* physical_count, vulkan_physical_device* physicals);

/**/
const char* vulkan_device_get_physical_device_type_str(VkPhysicalDeviceType type);
