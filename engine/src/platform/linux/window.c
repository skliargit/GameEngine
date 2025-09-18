#include "platform/window.h"

#ifdef PLATFORM_LINUX_FLAG

    #include "core/logger.h"
    #include "core/memory.h"
    #include "debug/assert.h"
    #include "platform/linux/wayland_backend.h"
    #include "platform/linux/xcb_backend.h"

    typedef struct platform_window_context {
        // Функция инициализации.
        bool (*window_initialize)(void* internal_data);
        // Функция создания окна.
        platform_window* (*window_create)(const platform_window_config* config, void* internal_data);
        // Функция уничтожения окна.
        void (*window_destroy)(platform_window* window, void* internal_data);
        // Неблокирующая функция обработки сообщений окнной системы.
        bool (*poll_events)(void* internal_data);
        // Функция завершающая работу оконной системы.
        void (*shutdown)(void* internal_data);
        // Функция получения расширений Vulkan.
        void (*enumerate_vulkan_extentions)(u32* extention_count, const char** out_extentions);
        // Функция создания поверхности Vulkan.
        u32 (*create_vulkan_surface)(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void** out_vulkan_surface);
        // Функция уничтожения поверхности Vulkan.
        void (*destroy_vulkan_surface)(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void* vulkan_surface);
        // Функция получения поддержки показа окном.
        u32 (*supports_vulkan_presentation)(platform_window* window, void* vulkan_pyhical_device, u32 queue_family_index);
        // TODO: Временно, убрать после добавления динамического аллокатора.
        u64 memory_requirement;
        // Внутренние данные бэкенда (гибкий массив, не влияет на размер структуры).
        u8 internal_data[];
    } platform_window_context;

    static platform_window_context* context = nullptr;

    bool platform_window_initialize(platform_window_backend_type type)
    {
        ASSERT(context == nullptr, "Window subsystem is already initialized.");
        ASSERT(type < PLATFORM_WINDOW_BACKEND_TYPE_COUNT, "Must be less than PLATFORM_WINDOW_BACKEND_TYPE_COUNT.");

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
        if(type == PLATFORM_WINDOW_BACKEND_TYPE_AUTO)
        {
            if(wayland_is_supported)
            {
                type = PLATFORM_WINDOW_BACKEND_TYPE_WAYLAND;
                LOG_TRACE("Wayland backend automatically selected.");
            }
            else if(xcb_is_supported)
            {
                type = PLATFORM_WINDOW_BACKEND_TYPE_XCB;
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
        if(type == PLATFORM_WINDOW_BACKEND_TYPE_WAYLAND && wayland_is_supported)
        {
            backend_memory_requirement = wayland_backend_memory_requirement();
        }
        else if(type == PLATFORM_WINDOW_BACKEND_TYPE_XCB && xcb_is_supported)
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
        if(type == PLATFORM_WINDOW_BACKEND_TYPE_WAYLAND)
        {
            context->window_initialize = wayland_backend_initialize;
            context->window_create = wayland_backend_window_create;
            context->window_destroy = wayland_backend_window_destroy;
            context->poll_events = wayland_backend_poll_events;
            context->shutdown = wayland_backend_shutdown;
            context->enumerate_vulkan_extentions = wayland_backend_enumerate_vulkan_extentions;
            context->create_vulkan_surface = wayland_backend_create_vulkan_surface;
            context->destroy_vulkan_surface = wayland_backend_destroy_vulkan_surface;
            context->supports_vulkan_presentation = wayland_backend_supports_vulkan_presentation;
        }
        else if(type == PLATFORM_WINDOW_BACKEND_TYPE_XCB)
        {
            context->window_initialize = xcb_backend_initialize;
            context->window_create = xcb_backend_window_create;
            context->window_destroy = xcb_backend_window_destroy;
            context->poll_events = xcb_backend_poll_events;
            context->shutdown = xcb_backend_shutdown;
            context->enumerate_vulkan_extentions = xcb_backend_enumerate_vulkan_extentions;
            context->create_vulkan_surface = xcb_backend_create_vulkan_surface;
            context->destroy_vulkan_surface = xcb_backend_destroy_vulkan_surface;
            context->supports_vulkan_presentation = xcb_backend_supports_vulkan_presentation;
        }

        if(!context->window_initialize(context->internal_data))
        {
            LOG_ERROR("Failed to initialize window backend (type: %d).", type);
            return false;
        }

        return true;
    }

    void platform_window_shutdown()
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");

        context->shutdown(context->internal_data);
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

        return context->poll_events(context->internal_data);
    }

    platform_window* platform_window_create(const platform_window_config* config)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(config != nullptr, "Config pointer must be non-null.");

        return context->window_create(config, context->internal_data);
    }

    void platform_window_destroy(platform_window* window)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");

        context->window_destroy(window, context->internal_data);
    }

    void platform_window_enumerate_vulkan_extentions(u32* extention_count, const char** out_extentions)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(extention_count != nullptr, "Pointer 'extention_count' must be non-null.");

        context->enumerate_vulkan_extentions(extention_count, out_extentions);
    }

    u32 platform_window_create_vulkan_surface(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void** out_vulkan_surface)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");
        ASSERT(vulkan_instance != nullptr, "Vulkan instance pointer must be non-null.");
        ASSERT(out_vulkan_surface != nullptr, "Vulkan surface pointer must be non-null.");

        return context->create_vulkan_surface(window, vulkan_instance, vulkan_allocator, out_vulkan_surface);
    }

    void platform_window_destroy_vulkan_surface(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void* vulkan_surface)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");
        ASSERT(vulkan_instance != nullptr, "Vulkan instance pointer must be non-null.");
        ASSERT(vulkan_surface != nullptr, "Vulkan surface pointer must be non-null.");

        context->destroy_vulkan_surface(window, vulkan_instance, vulkan_allocator, vulkan_surface);
    }

    u32 platform_window_supports_vulkan_presentation(platform_window* window, void* vulkan_pyhical_device, u32 queue_family_index)
    {
        ASSERT(context != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");
        ASSERT(vulkan_pyhical_device != nullptr, "Vulkan physical device pointer must be non-null.");

        return context->supports_vulkan_presentation(window, vulkan_pyhical_device, queue_family_index);
    }

#endif
