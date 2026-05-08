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

typedef struct renderer_config {
    // @brief Указывает тип используемого бэкенда.
    renderer_backend_type backend_type;
    // @brief Указывает какие использовать типы устройств.
    renderer_backend_device_type_flags device_types;
    // @brief Указатель на связаное с рендером окно.
    platform_window* window;
} renderer_config;

/**
    @brief Тип буфера данных.
*/
typedef enum buffer_type {
    BUFFER_TYPE_VERTEX,                         /**< Использование буфера для вершинных данных.           */
    BUFFER_TYPE_INDEX,                          /**< Использование буфера для индексных данных.           */
    BUFFER_TYPE_UNIFORM,                        /**< Использование буфера для универсальных данных.       */
    BUFFER_TYPE_STAGING,                        /**< Использование буфера для операций копирования.       */
    BUFFER_TYPE_READ,                           /**< Использование буфера для операций чтения.            */
    BUFFER_TYPE_STORAGE,                        /**< Использование буфера для хранения данных.            */
} buffer_type_t;

/**
    @brief Контекст буфера данных.
*/
typedef struct buffer {
    buffer_type_t type;                         /**< Тип буфера данных.         */
    usize size;                                 /**< Размера буфера в байтах.   */
    void* internal_data;                        /**< Внутренние данные бэкенда. */
} buffer_t;

#define SHADER_MAX_STAGES 2

/**
    @brief Стадии шейдеров (можно комбинировать).
*/
typedef enum shader_stage {
    SHADER_STAGE_VERTEX   = 0x01,               /**< Стадия обработки вершин геометрии.     */
    SHADER_STAGE_FRAGMENT = 0x02,               /**< Стадия обработки фрагментов(пикселей). */
} shader_stage_t;

/**
    @brief Контекст шейдера.
*/
typedef struct shader {
    buffer_t uniform_buffer;                    /**< Буфер данных (размер данных * на количество кадров). */
    void* internal_data;                        /**< Внутренние данные бэкенда.                           */
} shader_t;

// TODO: Временно.
// NOTE: Обязательно выравнивание 256 байт (следование требованию minUniformBufferOffsetAlignment).
typedef struct renderer_camera {
    mat4 proj;                                  /**< Матрица проекции камеры.   */
    mat4 view;                                  /**< Матрица вида камеры.       */
    mat4 padding[2];                            /**< Выравнивание объекта.      */
} renderer_camera_t;

// TODO: Временно.
// NOTE: Обязательно выравнивание 256 байт (следование требованию minUniformBufferOffsetAlignment).
typedef struct renderer_model {
    mat4 transform;                             /**< Матрица трансформации объекта. */
    mat4 padding[3];                            /**< Выравнивание объекта.          */
} renderer_model_t;

// TODO: Временно! Сделать shader_config и другие.
typedef struct shader_stage_file {
    char* path;
    shader_stage_t stage;
} shader_stage_file_t;
