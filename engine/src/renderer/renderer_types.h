#pragma once

#include <core/defines.h>

typedef struct platform_window platform_window;

typedef enum renderer_backend_type {
    RENDERER_BACKEND_TYPE_VULKAN,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX,
    RENDERER_BACKEND_TYPE_COUNT,
} renderer_backend_type;

typedef enum renderer_backend_device_type_flags {
    RENDERER_BACKEND_DEVICE_TYPE_SOFTWARE   = 0x01,
    RENDERER_BACKEND_DEVICE_TYPE_INTEGRATED = 0x02,
    RENDERER_BACKEND_DEVICE_TYPE_DISCRETE   = 0x04,
    RENDERER_BACKEND_DEVICE_TYPE_COUNT
} renderer_backend_device_type_flags;

typedef struct renderer_config {
    // @brief Указывает тип используемого бэкенда.
    renderer_backend_type backend_type;
    // @brief Указывает какие использовать типы устройств.
    renderer_backend_device_type_flags device_types;
    platform_window* window;
} renderer_config;
