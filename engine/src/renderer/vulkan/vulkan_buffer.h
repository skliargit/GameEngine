#pragma once

#include <core/defines.h>
#include <renderer/vulkan/vulkan_types.h>

/**/
bool vulkan_buffer_create(vulkan_context* context, vulkan_buffer_type type, u64 size, vulkan_buffer* out_buffer);

/**/
void vulkan_buffer_destroy(vulkan_context* context, vulkan_buffer* buffer);

/**/
bool vulkan_buffer_resize(vulkan_context* context, vulkan_buffer* buffer, u64 new_size);

/**/
bool vulkan_buffer_map_memory(vulkan_context* context, vulkan_buffer* buffer, u64 offset, u64 size, void** data);

/**/
void vulkan_buffer_unmap_memory(vulkan_context* context, vulkan_buffer* buffer);

/**/
bool vulkan_buffer_load_data(vulkan_context* context, vulkan_buffer* buffer, u64 offset, u64 size, const void* data);

/**/
bool vulkan_buffer_copy_range(vulkan_context* context, vulkan_buffer* src, u64 src_offset, vulkan_buffer* dst, u64 dst_offset, u64 size);
