#include "renderer/renderer.h"
#include "renderer/vulkan/backend.h"

#include "core/logger.h"
#include "core/memory.h"
#include "debug/assert.h"

typedef struct renderer_backend_info {
    const char* name;
    bool is_supported;
} renderer_backend_info;

typedef struct renderer_system_context {
    bool (*backend_initialize)(platform_window* window);
    void (*backend_shutdown)();

    void (*backend_wait_idle_device)();

    void (*frame_resize)(const u32 width, const u32 height);
    bool (*frame_begin)();
    bool (*frame_end)();
    void (*frame_bind_shader)(shader_t* shader);
    void (*frame_bind_buffer)(buffer_t* buffer, const usize buffer_offset);
    void (*frame_draw)(const u32 vertex_count);
    void (*frame_draw_indexed)(const u32 index_count);

    bool (*buffer_create)(buffer_t* buffer);
    void (*buffer_destroy)(buffer_t* buffer);
    bool (*buffer_resize)(buffer_t* buffer, usize new_size);
    bool (*buffer_map_memory)(buffer_t* buffer, usize offset, usize size, void** data); 
    void (*buffer_unmap_memory)(buffer_t* buffer);
    bool (*buffer_load_range)(buffer_t* buffer, usize offset, usize size, const void* data);
    void (*buffer_copy_range)(buffer_t* src, usize src_offset, buffer_t* dst, usize dst_offset, usize size);

    bool (*shader_create)(shader_t* shader, u32 stage_count, shader_stage_file_t* stage_files);
    void (*shader_destroy)(shader_t* shader);
    bool (*shader_acquire_resource)(shader_t* shader, u32 set_index, u32* out_resource_id);
    void (*shader_release_resource)(shader_t* shader, u32 resource_id);
    void (*shader_update_resource_binding)(shader_t* shader, u32 resource_id, u32 binding_index, const void* data);
    void (*shader_apply_resource)(shader_t* shader, u32 resource_id);

    // TODO: Временно, убрать!
    void (*shader_update_model)(shader_t* shader, renderer_model_t* model);
} renderer_system_context;

static renderer_system_context* context = nullptr;

bool renderer_initialize(renderer_config* config)
{
    ASSERT(context == nullptr, "Renderer system is already initialized.");
    ASSERT(config != nullptr, "Configuration must be non-null.");

    // Заполнение информации по бэкендам.
    // TODO: Разные ОС поддерживают определенные движки.
    renderer_backend_info backend_supports[RENDERER_BACKEND_TYPE_COUNT] = {
        [RENDERER_BACKEND_TYPE_VULKAN]  = {"Vulkan" , vulkan_backend_is_supported()},
        [RENDERER_BACKEND_TYPE_OPENGL]  = {"OpenGL" , false},
        [RENDERER_BACKEND_TYPE_DIRECTX] = {"DirectX", false}
    };

    if(!backend_supports[config->backend_type].is_supported)
    {
        LOG_ERROR("%s backend not supported.", backend_supports[config->backend_type].name);
        return false;
    }

    context = mallocate(sizeof(renderer_system_context), MEMORY_TAG_RENDERER);
    if(!context)
    {
        LOG_ERROR("Failed to allocate memory for renderer context.");
        renderer_shutdown();
        return false;
    }
    mzero(context, sizeof(renderer_system_context));

    switch(config->backend_type)
    {
        case RENDERER_BACKEND_TYPE_VULKAN:
            context->backend_initialize             = vulkan_backend_initialize;
            context->backend_shutdown               = vulkan_backend_shutdown;

            context->backend_wait_idle_device       = vulkan_wait_idle_device;

            context->frame_resize                   = vulkan_frame_resize;
            context->frame_begin                    = vulkan_frame_begin;
            context->frame_end                      = vulkan_frame_end;
            context->frame_bind_shader              = vulkan_frame_bind_shader;
            context->frame_bind_buffer              = vulkan_frame_bind_buffer;
            context->frame_draw                     = vulkan_frame_draw;
            context->frame_draw_indexed             = vulkan_frame_draw_indexed;

            context->buffer_create                  = vulkan_buffer_create;
            context->buffer_destroy                 = vulkan_buffer_destroy;
            context->buffer_resize                  = vulkan_buffer_resize;
            context->buffer_map_memory              = vulkan_buffer_map_memory;
            context->buffer_unmap_memory            = vulkan_buffer_unmap_memory;
            context->buffer_load_range              = vulkan_buffer_load_range;
            context->buffer_copy_range              = vulkan_buffer_copy_range;

            context->shader_create                  = vulkan_shader_create;
            context->shader_destroy                 = vulkan_shader_destroy;
            context->shader_acquire_resource        = vulkan_shader_acquire_resource;
            context->shader_release_resource        = vulkan_shader_release_resource;
            context->shader_update_resource_binding = vulkan_shader_update_resource_binding;
            context->shader_apply_resource          = vulkan_shader_apply_resource;

            context->shader_update_model      = vulkan_shader_update_model;
            break;
        case RENDERER_BACKEND_TYPE_OPENGL:
        case RENDERER_BACKEND_TYPE_DIRECTX:
        default:
            LOG_ERROR("Selected backend not supported.");
            renderer_shutdown();
            return false;
    }

    if(!context->backend_initialize(config->window))
    {
        LOG_ERROR("Failed to initialize %s backend.", backend_supports[config->backend_type].name);
        renderer_shutdown();
        return false;
    }

    return true;
}

void renderer_shutdown()
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->backend_shutdown();
    mfree(context, sizeof(renderer_system_context), MEMORY_TAG_RENDERER);
    context = nullptr;
}

bool renderer_is_initialized()
{
    return context != nullptr;
}

void renderer_wait_idle_device()
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->backend_wait_idle_device();
}

void renderer_frame_resize(u32 width, u32 height)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->frame_resize(width, height);
}

bool renderer_frame_begin()
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    return context->frame_begin();
}

bool renderer_frame_end()
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    return context->frame_end();
}

void renderer_frame_bind_shader(shader_t* shader)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->frame_bind_shader(shader);
}

void renderer_frame_bind_buffer(buffer_t* buffer, const usize buffer_offset)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->frame_bind_buffer(buffer, buffer_offset);
}

void renderer_frame_draw(const u32 vertex_count)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->frame_draw(vertex_count);
}

void renderer_frame_draw_indexed(const u32 index_count)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->frame_draw_indexed(index_count);
}

bool renderer_buffer_create(buffer_type_t type, usize size, buffer_t* out_buffer)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    out_buffer->type = type;
    out_buffer->size = size;
    return context->buffer_create(out_buffer);
}

void renderer_buffer_destroy(buffer_t* buffer)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->buffer_destroy(buffer);
    mzero(buffer, sizeof(buffer_t));
}

bool renderer_buffer_resize(buffer_t* buffer, usize new_size)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");
    ASSERT(new_size > 0, "New size must be greater than zero.");

    if(new_size < buffer->size)
    {
        LOG_WARN("New buffer size is less than current size.");
        return true;
    }

    return context->buffer_resize(buffer, new_size);
}

bool renderer_buffer_map_memory(buffer_t* buffer, usize offset, usize size, void** data)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    return context->buffer_map_memory(buffer, offset, size, data);
}

void renderer_buffer_unmap_memory(buffer_t* buffer)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->buffer_unmap_memory(buffer);
}

bool renderer_buffer_load_range(buffer_t* buffer, usize offset, usize size, const void* data)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    if(offset + size > buffer->size)
    {
        LOG_ERROR("Data exceeds buffer bounds.");
        return false;
    }

    return context->buffer_load_range(buffer, offset, size, data);
}

void renderer_buffer_copy_range(buffer_t* src, usize src_offset, buffer_t* dst, usize dst_offset, usize size)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->buffer_copy_range(src, src_offset, dst, dst_offset, size);
}

CORE_API bool renderer_shader_create(u32 stage_count, shader_stage_file_t* stage_files, shader_t* out_shader)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    return context->shader_create(out_shader, stage_count, stage_files);
}

void renderer_shader_destroy(shader_t* shader)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->shader_destroy(shader);
}

bool renderer_shader_acquire_resource(shader_t* shader, u32 set_index, u32* out_resource_id)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    return context->shader_acquire_resource(shader, set_index, out_resource_id);
}

void renderer_shader_release_resource(shader_t* shader, u32 resource_id)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->shader_release_resource(shader, resource_id);
}

void renderer_shader_update_resource_binding(shader_t* shader, u32 resource_id, u32 binding_index, const void* data)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->shader_update_resource_binding(shader, resource_id, binding_index, data);
}

void renderer_shader_apply_resource(shader_t* shader, u32 resource_id)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->shader_apply_resource(shader, resource_id);
}

void renderer_shader_update_model(shader_t* shader, renderer_model_t* model)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    context->shader_update_model(shader, model);
}
