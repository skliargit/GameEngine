#pragma once

#include <core/defines.h>
#include <platform/window.h>

// @brief Конфигурация для создания приложения.
typedef struct application_config {
    // @brief Начальная конфигурация окна.
    platform_window_config window;
} application_config;

API bool application_initialize(const application_config* config);

API void application_shutdown();

API void application_run();
