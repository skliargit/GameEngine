#include "platform/window.h"

#ifdef PLATFORM_LINUX_FLAG

    #include "core/logger.h"
    #include "core/memory.h"
    #include "debug/assert.h"
    #include "platform/linux/wayland_backend.h"
    #include "platform/linux/xcb_backend.h"

    // TODO: Общую логику перенести на данный уровень!!! Аналогично для windows! Система плагинов?

    typedef struct platform_window_context {
        // Функция инициализации.
        bool (*backend_initialize)(void* internal_data);
        // Функция завершающая работу оконной системы.
        void (*backend_shutdown)(void* internal_data);
        // Неблокирующая функция обработки сообщений окнной системы.
        bool (*backend_poll_events)(void* internal_data);
        // Функция создания окна.
        platform_window_t* (*window_create)(const platform_window_config_t* config, void* internal_data);
        // Функция уничтожения окна.
        void (*window_destroy)(platform_window_t* window, void* internal_data);
        // Функция получения текущего заголовка окна.
        const char* (*window_get_title)(platform_window_t* window);
        // Функция получения текущих размеров окна.
        void (*window_get_resolution)(platform_window_t* window, u32* width, u32* height);
        // Функция установки обработчика событий.
        void (*window_set_event_callback)(platform_window_t* window, platform_window_event_t event, platform_window_event_callback callback, void* user_data);
        // Функция сокрытия курсора.
        void (*window_hide_cursor)(platform_window_t* window);
        // Функция отображения курсора.
        void (*window_show_cursor)(platform_window_t* window);
        // Функция блокировки курсора в окне.
        void (*window_lock_cursor)(platform_window_t* window);
        // Функция освобождения курсора.
        void (*window_unlock_cursor)(platform_window_t* window);
        // Функция получения расширений Vulkan.
        void (*backend_enumerate_vulkan_extensions)(u32* extension_count, const char** out_extensions);
        // Функция создания поверхности Vulkan.
        u32 (*window_create_vulkan_surface)(platform_window_t* window, void* vulkan_instance, void* vulkan_allocator, void** out_vulkan_surface);
        // Функция уничтожения поверхности Vulkan.
        void (*window_destroy_vulkan_surface)(platform_window_t* window, void* vulkan_instance, void* vulkan_allocator, void* vulkan_surface);
        // Функция получения поддержки показа окном.
        u32 (*window_supports_vulkan_presentation)(platform_window_t* window, void* vulkan_pyhical_device, u32 queue_family_index);
        // TODO: Временно, убрать после добавления динамического аллокатора.
        u64 memory_requirement;
        // Внутренние данные бэкенда (гибкий массив, не влияет на размер структуры).
        u8 internal_data[];
    } platform_window_context;

    static platform_window_context* context = nullptr;

    bool platform_window_initialize(platform_window_backend_t type)
    {
        ASSERT(context == nullptr, "Window subsystem is already initialized.");
        ASSERT(type < PLATFORM_WINDOW_BACKEND_COUNT, "Must be less than PLATFORM_WINDOW_BACKEND_TYPE_COUNT.");

        // Кеширование ответов.
        bool wayland_is_supported = wayland_backend_is_supported();
        bool xcb_is_supported = xcb_backend_is_supported();

        // Вывод отладочной информации о поддерживаемых бэкендах.
        if(wayland_is_supported)
        {
            LOG_TRACE("Wayland backend is supported.");
        }
        else
        {
            LOG_TRACE("Wayland backend is not supported (server not available).");
        }

        if(xcb_is_supported)
        {
            LOG_TRACE("XCB backend is supported.");
        }
        else
        {
            LOG_TRACE("XCB backend is not supported (X server not available).");
        }

        // Автоматический выбор бэкенда.
        if(type == PLATFORM_WINDOW_BACKEND_DEFAULT)
        {
            if(wayland_is_supported)
            {
                type = PLATFORM_WINDOW_BACKEND_WAYLAND;
                LOG_TRACE("Wayland backend automatically selected.");
            }
            else if(xcb_is_supported)
            {
                type = PLATFORM_WINDOW_BACKEND_XCB;
                LOG_TRACE("XCB (X11) backend automatically selected.");
            }
            else
            {
                LOG_ERROR("No supported window backend found.");
                return false;
            }
        }

        // Ручной выбор бэкенда (или выбор автоматического режима).
        u64 backend_memory_requirement = 0;
        if(type == PLATFORM_WINDOW_BACKEND_WAYLAND && wayland_is_supported)
        {
            backend_memory_requirement = wayland_backend_memory_requirement();
        }
        else if(type == PLATFORM_WINDOW_BACKEND_XCB && xcb_is_supported)
        {
            backend_memory_requirement = xcb_backend_memory_requirement();
        }
        else
        {
            LOG_ERROR("Unsupported window backend type selected for Linux: %d.", type);
            return false;
        }

        u64 memory_requirement = sizeof(platform_window_context) + backend_memory_requirement;
        context = mallocate(memory_requirement, MEMORY_TAG_APPLICATION);
        mzero(context, memory_requirement);

        // Необходимо для уничтожения.
        context->memory_requirement = memory_requirement;

        // Инициализация бэкенда.
        if(type == PLATFORM_WINDOW_BACKEND_WAYLAND)
        {
            context->backend_initialize = wayland_backend_initialize;
            context->backend_shutdown = wayland_backend_shutdown;
            context->backend_poll_events = wayland_backend_poll_events;
            context->window_create = wayland_window_create;
            context->window_destroy = wayland_window_destroy;
            context->window_get_title = wayland_window_get_title;
            context->window_get_resolution = wayland_window_get_resolution;
            context->window_set_event_callback = wayland_window_set_event_callback;
            context->window_hide_cursor = wayland_window_hide_cursor;
            context->window_show_cursor = wayland_window_show_cursor;
            context->window_lock_cursor = wayland_window_lock_cursor;
            context->window_unlock_cursor = wayland_window_unlock_cursor;
            context->backend_enumerate_vulkan_extensions = wayland_enumerate_vulkan_extensions;
            context->window_create_vulkan_surface = wayland_window_create_vulkan_surface;
            context->window_destroy_vulkan_surface = wayland_window_destroy_vulkan_surface;
            context->window_supports_vulkan_presentation = wayland_window_supports_vulkan_presentation;
        }
        else if(type == PLATFORM_WINDOW_BACKEND_XCB)
        {
            context->backend_initialize = xcb_backend_initialize;
            context->backend_shutdown = xcb_backend_shutdown;
            context->backend_poll_events = xcb_backend_poll_events;
            context->window_create = xcb_window_create;
            context->window_destroy = xcb_window_destroy;
            context->window_get_title = xcb_window_get_title;
            context->window_get_resolution = xcb_window_get_resolution;
            context->window_set_event_callback = xcb_window_set_event_callback;
            context->window_hide_cursor = xcb_window_hide_cursor;
            context->window_show_cursor = xcb_window_show_cursor;
            context->window_lock_cursor = xcb_window_lock_cursor;
            context->window_unlock_cursor = xcb_window_unlock_cursor;
            context->backend_enumerate_vulkan_extensions = xcb_enumerate_vulkan_extensions;
            context->window_create_vulkan_surface = xcb_window_create_vulkan_surface;
            context->window_destroy_vulkan_surface = xcb_window_destroy_vulkan_surface;
            context->window_supports_vulkan_presentation = xcb_window_supports_vulkan_presentation;
        }

        if(!context->backend_initialize(context->internal_data))
        {
            LOG_ERROR("Failed to initialize window backend (type: %d).", type);
            return false;
        }

        return true;
    }

    void platform_window_shutdown()
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");

        context->backend_shutdown(context->internal_data);
        mfree(context, context->memory_requirement, MEMORY_TAG_APPLICATION);
        context = nullptr;
    }

    bool platform_window_is_initialized()
    {
        return context != nullptr;
    }

    bool platform_window_poll_events()
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");

        return context->backend_poll_events(context->internal_data);
    }

    platform_window_t* platform_window_create(const platform_window_config_t* config)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(config != nullptr, "Config pointer must be non-null.");

        return context->window_create(config, context->internal_data);
    }

    void platform_window_destroy(platform_window_t* window)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");

        context->window_destroy(window, context->internal_data);
    }

    const char* platform_window_get_title(platform_window_t* window)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");

        return context->window_get_title(window);
    }

    void platform_window_get_resolution(platform_window_t* window, u32* width, u32* height)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");
        ASSERT(width != nullptr, "Poiner to width must be nun-null.");
        ASSERT(height != nullptr, "Pointer to height must be non-null.");

        context->window_get_resolution(window, width, height);
    }

    void platform_window_set_event_callback(platform_window_t* window, platform_window_event_t event, platform_window_event_callback callback, void* user_data)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");
        ASSERT(event < PLATFORM_WINDOW_EVENT_COUNT, "Window event type must be less than PLATFORM_WINDOW_EVENT_TYPE_COUNT.");

        context->window_set_event_callback(window, event, callback, user_data);
    }

    void platform_window_hide_cursor(platform_window_t* window)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");

        context->window_hide_cursor(window);
    }

    void platform_window_show_cursor(platform_window_t* window)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");

        context->window_show_cursor(window);
    }

    void platform_window_lock_cursor(platform_window_t* window)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");

        context->window_lock_cursor(window);
    }

    void platform_window_unlock_cursor(platform_window_t* window)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");

        context->window_unlock_cursor(window);
    }

    void platform_window_enumerate_vulkan_extensions(u32* extension_count, const char** out_extensions)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(extension_count != nullptr, "Pointer 'extension_count' must be non-null.");

        context->backend_enumerate_vulkan_extensions(extension_count, out_extensions);
    }

    u32 platform_window_create_vulkan_surface(platform_window_t* window, void* vulkan_instance, void* vulkan_allocator, void** out_vulkan_surface)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");
        ASSERT(vulkan_instance != nullptr, "Vulkan instance pointer must be non-null.");
        ASSERT(out_vulkan_surface != nullptr, "Vulkan surface pointer must be non-null.");

        return context->window_create_vulkan_surface(window, vulkan_instance, vulkan_allocator, out_vulkan_surface);
    }

    void platform_window_destroy_vulkan_surface(platform_window_t* window, void* vulkan_instance, void* vulkan_allocator, void* vulkan_surface)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");
        ASSERT(vulkan_instance != nullptr, "Vulkan instance pointer must be non-null.");
        ASSERT(vulkan_surface != nullptr, "Vulkan surface pointer must be non-null.");

        context->window_destroy_vulkan_surface(window, vulkan_instance, vulkan_allocator, vulkan_surface);
    }

    u32 platform_window_supports_vulkan_presentation(platform_window_t* window, void* vulkan_pyhical_device, u32 queue_family_index)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");
        ASSERT(vulkan_pyhical_device != nullptr, "Vulkan physical device pointer must be non-null.");

        return context->window_supports_vulkan_presentation(window, vulkan_pyhical_device, queue_family_index);
    }

#endif
