#include "platform/window.h"

#if PLATFORM_LINUX_FLAG

    #include "core/logger.h"
    #include "platform/linux/backend_wayland.h"
    #include "platform/linux/backend_xcb.h"

    // TODO: Временно, обернуть!
    #include <stdlib.h>
    #include <string.h>

    struct platform_window {
        // Функция создания окна.
        bool (*backend_create)(const platform_window_config* config, void* internal_data);
        // Функция уничтожения окна.
        void (*backend_destroy)(void* internal_data);
        // Неблокирующая функция обработки сообщений окна.
        bool (*backend_poll_events)(void* internal_data);
        // Внутренние данные бэкенда (гибкий массив, не влияет на размер структуры).
        u8 internal_data[];
    };

    bool platform_window_create(const platform_window_config* config, platform_window** out_window)
    {
        if(!config || !out_window)
        {
            LOG_ERROR("%s requires valid config and out_window pointers.", __func__);
            return false;
        }

        // Проверка на повторную инициализацию.
        if(*out_window != nullptr)
        {
            LOG_ERROR("%s: Output window pointer must be null (already contains window reference)", __func__);
            return false;
        }

        // Автоматический выбор бэкенда.
        platform_window_backend_type backend_type = config->backend_type;
        if(backend_type == PLATFORM_WINDOW_BACKEND_TYPE_AUTO)
        {
            if(wayland_backend_is_supported())
            {
                backend_type = PLATFORM_WINDOW_BACKEND_TYPE_WAYLAND;
                LOG_TRACE("%s: Backend wayland was automatically selected (preferred).", __func__);
            }
            else if(xcb_backend_is_supported())
            {
                backend_type = PLATFORM_WINDOW_BACKEND_TYPE_XCB;
                LOG_TRACE("%s: Backend xcb(X11) was automatically selected.", __func__);
            }
            else
            {
                LOG_ERROR("%s: No supported window backend found.", __func__);
                return false;
            }
        }

        // Ручной выбор бэкенда (или выбор автоматического режима).
        u64 backend_memory_requirement = 0;
        if(backend_type == PLATFORM_WINDOW_BACKEND_TYPE_WAYLAND && wayland_backend_is_supported())
        {
            backend_memory_requirement = wayland_backend_instance_memory_requirement();
        }
        else if(backend_type == PLATFORM_WINDOW_BACKEND_TYPE_XCB && xcb_backend_is_supported())
        {
            backend_memory_requirement = xcb_backend_instance_memory_requirement();
        }
        else
        {
            LOG_FATAL("%s: Unknown window backend type selected: %d", __func__, backend_type);
            return false;
        }

        // Контроль переполнения.
        if(backend_memory_requirement > U64_MAX - sizeof(platform_window)) {
            LOG_FATAL("%s: Memory requirement too large.", __func__);
            return false;
        }

        u64 memory_requirement = sizeof(platform_window) + backend_memory_requirement;
        // TODO: Обернуть!
        platform_window* window = malloc(memory_requirement);
        if(!window)
        {
            LOG_FATAL("%s: Failed to obtain memory for window instance.", __func__);
            return false;
        }
        memset(window, 0, memory_requirement);

        // Инициализация бэкенда.
        if(backend_type == PLATFORM_WINDOW_BACKEND_TYPE_WAYLAND)
        {
            window->backend_create = wayland_backend_create;
            window->backend_destroy = wayland_backend_destroy;
            window->backend_poll_events = wayland_backend_poll_events;
        }
        else if(backend_type == PLATFORM_WINDOW_BACKEND_TYPE_XCB)
        {
            window->backend_create = xcb_backend_create;
            window->backend_destroy = xcb_backend_destroy;
            window->backend_poll_events = xcb_backend_poll_events;
        }

        // Создание окна.
        if(!window->backend_create(config, window->internal_data))
        {
            LOG_ERROR("%s: Failed to create window.", __func__);
            // TODO: Обернуть!
            free(window);
            return false;
        }

        *out_window = window;
        return true;
    }

    void platform_window_destroy(platform_window* window)
    {
        if(!window)
        {
            LOG_ERROR("%s requires a valid pointer to window.", __func__);
            return;
        }

        window->backend_destroy(window->internal_data);
        // TODO: Обернуть!
        free(window);
    }

    bool platform_window_poll_events(platform_window* window)
    {
        if(!window)
        {
            LOG_ERROR("%s requires a valid pointer to window.", __func__);
            return false;
        }

        return window->backend_poll_events(window->internal_data);
    }

#endif
