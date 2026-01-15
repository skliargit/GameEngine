#include "renderer/renderer.h"
#include "renderer/vulkan/vulkan_backend.h"

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
    void (*backend_resize)(const u32 width, const u32 height);
    bool (*backend_frame_begin)();
    bool (*backend_frame_end)();
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
            context->backend_initialize  = vulkan_backend_initialize;
            context->backend_shutdown    = vulkan_backend_shutdown;
            context->backend_resize      = vulkan_backend_resize;
            context->backend_frame_begin = vulkan_backend_frame_begin;
            context->backend_frame_end   = vulkan_backend_frame_end;
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

bool renderer_system_is_initialized()
{
    return context != nullptr;
}

void renderer_on_resize(u32 width, u32 height)
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");
    context->backend_resize(width, height);
}

bool renderer_draw()
{
    ASSERT(context != nullptr, "Renderer system should be initialized.");

    if(!context->backend_frame_begin())
    {
        LOG_DEBUG("Skipping begin frame.");
        return false;
    }

    if(!context->backend_frame_end())
    {
        LOG_ERROR("Failed to end frame.");
        return false;
    }

    return true;
}
