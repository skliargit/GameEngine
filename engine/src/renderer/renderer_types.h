#pragma once

#include <core/defines.h>

typedef enum renderer_backend_type {
    RENDERER_BACKEND_TYPE_VULKAN,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX,
    RENDERER_BACKEND_TYPE_COUNT,
} renderer_backend_type;

typedef struct renderer_config {
    renderer_backend_type backend_type;
    const char* title;
    u32 width;
    u32 height;
} renderer_config;
