#pragma once

#include <core/defines.h>
#include <platform/window.h>

// @brief Конфигурация для создания приложения.
typedef struct application_config {
    // @brief Начальная конфигурация окна.
    struct {
        // @brief Тип бэкенда.
        platform_window_backend_type backend;
        // @brief Заголовок окна.
        const char* title;
        // @brief Начальная ширина окна в пикселях.
        u32 width;
        // @brief Начальная высота окна в пикселях.
        u32 height;
    } window;
} application_config;

API bool application_initialize(const application_config* config);

API void application_shutdown();

API void application_run();
