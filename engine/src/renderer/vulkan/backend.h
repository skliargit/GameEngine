#pragma once

#include <core/defines.h>
#include <renderer/types.h>
#include <resource/types.h>
#include <platform/window.h>
#include <renderer/vulkan/types.h>

bool vulkan_backend_initialize(platform_window* window);
void vulkan_backend_shutdown();
bool vulkan_backend_is_supported();

void vulkan_wait_idle_device();

void vulkan_frame_resize(const u32 width, const u32 height);
bool vulkan_frame_begin();
bool vulkan_frame_end();
void vulkan_frame_bind_shader(shader_t* shader);
void vulkan_frame_bind_buffer(buffer_t* buffer, const usize buffer_offset);
void vulkan_frame_draw(const u32 vertex_count);
void vulkan_frame_draw_indexed(const u32 index_count);

bool vulkan_buffer_create(buffer_t* buffer);
void vulkan_buffer_destroy(buffer_t* buffer);
bool vulkan_buffer_resize(buffer_t* buffer, usize new_size);
bool vulkan_buffer_map_memory(buffer_t* buffer, usize offset, usize size, void** data); 
void vulkan_buffer_unmap_memory(buffer_t* buffer);
bool vulkan_buffer_load_range(buffer_t* buffer, usize offset, usize size, const void* data);
void vulkan_buffer_copy_range(buffer_t* src, usize src_offset, buffer_t* dst, usize dst_offset, usize size);

bool vulkan_shader_create(shader_t* shader, u32 stage_count, shader_stage_file_t* stage_files);
void vulkan_shader_destroy(shader_t* shader);
bool vulkan_shader_acquire_resource(shader_t* shader, u32 set_index, u32* out_resource_id);
void vulkan_shader_release_resource(shader_t* shader, u32 resource_id);
void vulkan_shader_update_resource_binding(shader_t* shader, u32 resource_id, u32 binding_index, const void* data);
void vulkan_shader_apply_resource(shader_t* shader, u32 resource_id);

// TODO: Временно, убрать!
void vulkan_shader_update_model(shader_t* shader, renderer_model_t* model);

void vulkan_texture_create(texture_t* t, const void* data);
void vulkan_texture_destroy(texture_t* t);
