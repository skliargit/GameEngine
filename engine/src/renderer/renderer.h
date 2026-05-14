#pragma once

#include <core/defines.h>
#include <renderer/types.h>

bool renderer_initialize(renderer_config* config);
void renderer_shutdown();
CORE_API bool renderer_is_initialized();

CORE_API void renderer_wait_idle_device();

CORE_API void renderer_frame_resize(u32 width, u32 height);
CORE_API bool renderer_frame_begin();
CORE_API void renderer_frame_bind_shader(shader_t* shader);
CORE_API void renderer_frame_bind_buffer(buffer_t* buffer, const usize buffer_offset);
CORE_API void renderer_frame_draw(const u32 vertex_count);
CORE_API void renderer_frame_draw_indexed(const u32 index_count);
CORE_API bool renderer_frame_end();

CORE_API bool renderer_buffer_create(buffer_type_t type, usize size, buffer_t* out_buffer);
CORE_API void renderer_buffer_destroy(buffer_t* buffer);
CORE_API bool renderer_buffer_resize(buffer_t* buffer, usize new_size);
CORE_API bool renderer_buffer_map_memory(buffer_t* buffer, usize offset, usize size, void** data); 
CORE_API void renderer_buffer_unmap_memory(buffer_t* buffer);
CORE_API bool renderer_buffer_load_range(buffer_t* buffer, usize offset, usize size, const void* data);
CORE_API void renderer_buffer_copy_range(buffer_t* src, usize src_offset, buffer_t* dst, usize dst_offset, usize size);

CORE_API bool renderer_shader_create(u32 stage_count, shader_stage_file_t* stage_files, shader_t* out_shader);
CORE_API void renderer_shader_destroy(shader_t* shader);
CORE_API bool renderer_shader_acquire_resource(shader_t* shader, u32 set_index, u32* out_resource_id);
CORE_API void renderer_shader_release_resource(shader_t* shader, u32 resource_id);
CORE_API void renderer_shader_update_resource_binding(shader_t* shader, u32 resource_id, u32 binding_index, const void* data);
CORE_API void renderer_shader_apply_resource(shader_t* shader, u32 resource_id);

// TODO: Временно, убрать!
CORE_API void renderer_shader_update_model(shader_t* shader, renderer_model_t* model);
