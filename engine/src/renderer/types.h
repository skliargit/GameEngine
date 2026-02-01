#pragma once

#include <core/defines.h>
#include <math/types.h>

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

// TODO: Временно.
// NOTE: Обязательно выравнивание 256 байт.
typedef struct renderer_camera {
    // @brief Матрица проекции камеры.
    mat4 proj;
    // @brief Матрица вида камеры.
    mat4 view;
    // @brief Выравнивание объекта.
    mat4 padding[2]; // TODO: Добавить проверку и следование требованию minUniformBufferOffsetAlignment.
} renderer_camera;

// TODO: Временно.
// NOTE: Обязательно выравнивание 256 байт.
typedef struct renderer_model {
    // @brief Матрица трансформации объекта.
    mat4 transform;
    // @brief Выравнивание объекта.
    mat4 padding[3]; // TODO: Добавить проверку и следование требованию minUniformBufferOffsetAlignment.
} renderer_model;

typedef struct renderer_config {
    // @brief Указывает тип используемого бэкенда.
    renderer_backend_type backend_type;
    // @brief Указывает какие использовать типы устройств.
    renderer_backend_device_type_flags device_types;
    // @brief Указатель на связаное с рендером окно.
    platform_window* window;
} renderer_config;
