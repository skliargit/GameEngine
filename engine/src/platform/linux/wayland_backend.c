#include "platform/linux/wayland_backend.h"

#ifdef PLATFORM_LINUX_FLAG

    // Нужна для memfd_create.
    #define _GNU_SOURCE 1

    #include "core/logger.h"
    #include "core/string.h"
    #include "core/memory.h"
    #include "platform/linux/wayland_xdg_protocol.h"
    #include <wayland-client.h>
    #include <vulkan/vulkan.h>
    #include <vulkan/vulkan_wayland.h>
    #include <xkbcommon/xkbcommon.h>
    #include <errno.h>
    #include <string.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/mman.h>
    #include <poll.h>

    // Для включения отладки оконной системы индивидуально.
    #ifndef DEBUG_PLATFORM_FLAG
        #undef LOG_DEBUG
        #undef LOG_TRACE
        #define LOG_DEBUG(...) UNUSED(0)
        #define LOG_TRACE(...) UNUSED(0)
    #endif

    typedef struct platform_window {
        // Соединение с Wayland сервером (НАСЛЕДУЕТСЯ).
        struct wl_display* display;
        // Wayland поверхность окна.
        struct wl_surface* surface;
        // XDG поверхность для управления.
        struct xdg_surface* xsurface;
        // XDG toplevel для оконных операций.
        struct xdg_toplevel* xtoplevel;
        // Разделяемая память для буферов (НАСЛЕДУЕТСЯ).
        struct wl_shm* shm;
        // Временный буфер фона (до рендеринга).
        struct wl_buffer* background;
        // Цвет фона в формате ARGB (0xAARRGGBB).
        u32 background_color;
        // Текущий заголовок окна.
        char* title;
        // Текущая ширина окна в пикселях.
        u32 width;
        // Текущая высота окна в пикселях.
        u32 height;
        // Новая ширина окна, ожидающая применения.
        u32 width_pending;
        // Новая высота окна, ожидающая применения.
        u32 height_pending;
        // Флаг ожидания обработки изменения размера.
        bool resize_pending;
        // Фокус ввода на этом окне.
        bool focused;
        // Callback-функция, вызываемая при попытке закрытия окна, может быть nullptr.
        platform_window_on_close_callback on_close;
        // Callback-функция, вызываемая при изменении размера окна, может быть nullptr.
        platform_window_on_resize_callback on_resize;
        // Callback-функция, вызываемая при изменении состояния фокуса окна, может быть nullptr.
        platform_window_on_focus_callback on_focus;
        // Callback-функция для обработки нажатий и отпусканий клавиш клавиатуры, может быть nullptr.
        platform_window_on_key_callback on_key;
        // Callback-функция для обработки нажатий кнопок мыши, может быть nullptr.
        platform_window_on_mouse_button_callback on_mouse_button;
        // Callback-функция для обработки движения курсора мыши в пределах окна, может быть nullptr.
        platform_window_on_mouse_move_callback on_mouse_move;
        // Callback-функция для обработки прокрутки колеса мыши, может быть nullptr.
        platform_window_on_mouse_wheel_callback on_mouse_wheel;
    } platform_window;

    typedef struct wayland_client {
        // Соединение с Wayland сервером.
        struct wl_display* display;
        // Регистратор глобальных объектов.
        struct wl_registry* registry;
        // Композитор для создания поверхностей.
        struct wl_compositor* compositor;
        // XDG базовый протокол для окон.
        struct xdg_wm_base* xbase;
        // Разделяемая память для буферов.
        struct wl_shm* shm;
        // Устройства ввода (клавиатура, мышь).
        struct wl_seat* seat;
        // Клавиатура для обработки ввода.
        struct wl_keyboard* keyboard;
        // Мышка для обработки ввода.
        struct wl_pointer* pointer;
        // Контекст библиотеки XKBcommon для обработки раскладки клавиатуры.
        struct xkb_context* xkbcontext;
        // Состояние XKB (текущая раскладка, модификаторы, заблокированные клавиши).
        struct xkb_state* xkbstate;
        // Файловые дескрипторы для poll().
        struct pollfd poll_fd[1];
        // Количество отслеживаемых дескрипторов.
        nfds_t poll_fd_count;
        // Файловый дескриптор Wayland соединения.
        i32 displayfd;
        // TODO: Окно с текущим фокусом ввода.
        platform_window* focused_window;
        // TODO: Динамический массив всех окон (пока только одно окно).
        platform_window* window;
    } wayland_client;

    // Объявление функции для создания буфера.
    static struct wl_buffer* wl_buffer_create(struct wl_shm* shm, u32 width, u32 height, u32 argb_color);

    // Обявления обработчиков событий wayland.
    static void registry_add(void* data, struct wl_registry* registry, u32 name, const char* interface, u32 version);
    static void registry_remove(void* data, struct wl_registry* registry, u32 name);
    static void xbase_ping(void* data, struct xdg_wm_base *xbase, u32 serial);
    static void xsurface_configure(void* data, struct xdg_surface* xsurface, u32 serial);
    static void xtoplevel_configure(void* data, struct xdg_toplevel* xtoplevel, i32 width, i32 height, struct wl_array* states);
    static void xtoplevel_close(void* data, struct xdg_toplevel* xtoplevel);
    static void xtoplevel_configure_bounds(void* data, struct xdg_toplevel* xtoplevel, i32 width, i32 height);
    static void xtoplevel_wm_capabilities(void* data, struct xdg_toplevel* xtoplevel, struct wl_array* capabilities);
    static void seat_capabilities(void* data, struct wl_seat* wseat, u32 capabilities);
    static void seat_name(void* data, struct wl_seat* wseat, const char* name);
    static void kb_keymap(void* data, struct wl_keyboard* wkeyboard, u32 format, i32 fd, u32 size);
    static void kb_enter(void* data, struct wl_keyboard* wkeyboard, u32 serial, struct wl_surface* wsurface, struct wl_array* keys);
    static void kb_leave(void* data, struct wl_keyboard* wkeyboard, u32 serial, struct wl_surface* wsurface);
    static void kb_key(void* data, struct wl_keyboard* wkeyboard, u32 serial, u32 time, u32 key, u32 state);
    static void kb_mods(void* data, struct wl_keyboard* wkeyboard, u32 serial, u32 depressed, u32 latched, u32 locked, u32 group);
    static void kb_repeat(void* data, struct wl_keyboard* wkeyboard, i32 rate, i32 delay);
    static void pt_enter(void* data, struct wl_pointer* wpointer, u32 serial, struct wl_surface* wsurface, wl_fixed_t x, wl_fixed_t y);
    static void pt_leave(void* data, struct wl_pointer* wpointer, u32 serial, struct wl_surface* wsurface);
    static void pt_motion(void* data, struct wl_pointer* wpointer, u32 time, wl_fixed_t x, wl_fixed_t y);
    static void pt_button(void* data, struct wl_pointer* wpointer, u32 serial, u32 time, u32 button, u32 state);
    static void pt_axis(void* data, struct wl_pointer* wpointer, u32 time, u32 axis, wl_fixed_t value);
    static void pt_frame(void* data, struct wl_pointer* wpointer);
    static void pt_axis_source(void* data, struct wl_pointer* wpointer, u32 axis_source);
    static void pt_axis_stop(void* data, struct wl_pointer* wpointer, u32 time, u32 axis);
    static void pt_axis_discrete(void* data, struct wl_pointer* wpointer, u32 axis, i32 discrete);
    static void pt_axis_value120(void* data, struct wl_pointer* wpointer, u32 axis, i32 value120);
    static void pt_axis_relative_direction(void* data, struct wl_pointer* wpointer, u32 axis, u32 direction);

    // Компоновка обработчиков в структуры.
    static const struct wl_registry_listener  registry_listeners  = { registry_add, registry_remove };
    static const struct xdg_wm_base_listener  xbase_listeners     = { xbase_ping };
    static const struct xdg_surface_listener  xsurface_listeners  = { xsurface_configure };
    static const struct xdg_toplevel_listener xtoplevel_listeners = { xtoplevel_configure, xtoplevel_close, xtoplevel_configure_bounds, xtoplevel_wm_capabilities };
    static const struct wl_seat_listener      seat_listeners      = { seat_capabilities, seat_name };
    static const struct wl_keyboard_listener  keyboard_listeners  = { kb_keymap, kb_enter, kb_leave, kb_key, kb_mods, kb_repeat };
    static const struct wl_pointer_listener   pointer_listeners   = {
        pt_enter, pt_leave, pt_motion, pt_button, pt_axis, pt_frame, pt_axis_source, pt_axis_stop, pt_axis_discrete,
        pt_axis_value120, pt_axis_relative_direction
    };

    bool wayland_backend_initialize(void* internal_data)
    {
        wayland_client* client = internal_data;

        // Установка соединения с wayland сервером.
        client->display = wl_display_connect(nullptr);
        if(!client->display)
        {
            LOG_ERROR("Failed to connect to Wayland display server.");
            return false;
        }
        LOG_TRACE("Connected to Wayland display.");

        // Получение файлового дескриптора для polling.
        client->displayfd = wl_display_get_fd(client->display);
        LOG_TRACE("Wayland display FD: %d.", client->displayfd);

        // Настройка poll структуры для обработки событий.
        client->poll_fd[0].fd = client->displayfd;
        // TODO: POLLERR - Указывает на возникновение ошибки на файловом дескрипторе.
        //       POLLHUP - Указывает, что соединение "повисло".
        client->poll_fd[0].events = POLLIN; //| POLLERR | POLLHUP;
        client->poll_fd_count = sizeof(client->poll_fd) / sizeof(struct pollfd);

        // Получение registry для обнаружения глобальных объектов.
        client->registry = wl_display_get_registry(client->display);
        if(!client->registry)
        {
            LOG_ERROR("Failed to get Wayland registry.");
            wayland_backend_shutdown(client);
            return false;
        }
        wl_registry_add_listener(client->registry, &registry_listeners, client);

        // Синхронный обмен с сервером для получения глобальных объектов.
        LOG_TRACE("Retrieving global objects...");
        wl_display_roundtrip(client->display);

        // Проверка обязательных глобальных объектов.
        if(!client->compositor)
        {
            LOG_ERROR("Failed to bind 'wl_compositor' interface - compositor not available.");
            wayland_backend_shutdown(client);
            return false;
        }

        if(!client->xbase)
        {
            LOG_ERROR("Failed to bind 'xdg_wm_base' interface - XDG shell not available.");
            wayland_backend_shutdown(client);
            return false;
        }

        if(!client->shm)
        {
            LOG_ERROR("Failed to bind 'wl_shm' interface - shared memory not available.");
            wayland_backend_shutdown(client);
            return false;
        }

        // Дополнительный синхронизация для завершения инициализации.
        wl_display_roundtrip(client->display);

        if(!client->seat)
        {
            LOG_ERROR("Failed to bind 'seat' interface - input devices not available.");
            wayland_backend_shutdown(client);
            return false;
        }

        // Получение контекста XKBCommon.
        client->xkbcontext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        if(!client->xkbcontext)
        {
            LOG_ERROR("Failed to obtain XKB context.");
            wayland_backend_shutdown(client);
            return false;
        }

        LOG_TRACE("Wayland backend initialized successfully.");
        client->window = nullptr;
        return true;
    }

    void wayland_backend_shutdown(void* internal_data)
    {
        wayland_client* client = internal_data;

        LOG_TRACE("Disconnecting from Wayland server...");

        // Уничтожение всех окон (пока только одно окно).
        if(client->window)
        {
            LOG_WARN("Window was not properly destroyed before shutdown.");
            wayland_backend_window_destroy(client->window, client);
            // NOTE: Осовобождение client->window в wayland_backend_window_destroy().
        }

        if(client->xkbstate)   xkb_state_unref(client->xkbstate);
        if(client->xkbcontext) xkb_context_unref(client->xkbcontext);
        if(client->pointer)    wl_pointer_destroy(client->pointer);
        if(client->keyboard)   wl_keyboard_destroy(client->keyboard);
        if(client->seat)       wl_seat_destroy(client->seat);
        if(client->xbase)      xdg_wm_base_destroy(client->xbase);
        if(client->shm)        wl_shm_destroy(client->shm);
        if(client->compositor) wl_compositor_destroy(client->compositor);
        if(client->registry)   wl_registry_destroy(client->registry);

        if(client->display)
        {
            wl_display_flush(client->display);
            wl_display_disconnect(client->display);
        }

        mzero(client, sizeof(wayland_client));
        LOG_TRACE("Wayland backend shutdown complete.");
    }

    bool wayland_backend_poll_events(void* internal_data)
    {
        wayland_client* client = internal_data;

        // Проверка очереди событий (0 - пустая очередь, -1 - непуста).
        // NOTE: Если очередь оказывается непустой, то операцию чтение выполнять или отменять не нужно!
        if(wl_display_prepare_read(client->display) != 0)
        {
            // Обработка очереди сообщений.
            return wl_display_dispatch_pending(client->display) != -1;
        }

        // Ожидание новых событий с таймаутом (N > 0 - время ожидания, 0 - без ожидания, -1 - бесконечное ожидание).
        if(poll(client->poll_fd, client->poll_fd_count, 0) > 0)
        {
            // Чтение новых событий.
            wl_display_read_events(client->display);
        }
        else
        {
            // Отмена миссии.
            wl_display_cancel_read(client->display);
        }

        // Обработка новых событий (если есть).
        return wl_display_dispatch_pending(client->display) != -1;
    }

    bool wayland_backend_is_supported()
    {
        struct wl_display* display = wl_display_connect(nullptr);
        if(!display)
        {
            return false;
        }
        wl_display_disconnect(display);
        return true;
    }

    u64 wayland_backend_memory_requirement()
    {
        return sizeof(wayland_client);
    }

    platform_window* wayland_backend_window_create(const platform_window_config* config, void* internal_data)
    {
        wayland_client* client = internal_data;

        LOG_TRACE("Creating window '%s' (size: %ux%u)...", config->title, config->width, config->height);

        if(client->window)
        {
            LOG_WARN("Window has already been created (only one window supported for now).");
            return nullptr;
        }

        if(!client->display)
        {
            LOG_ERROR("Wayland connection is not available.");
            return nullptr;
        }

        platform_window* window = mallocate(sizeof(platform_window), MEMORY_TAG_APPLICATION);
        mzero(window, sizeof(platform_window));
        window->title = string_duplicate(config->title);
        window->display = client->display;
        window->shm = client->shm;
        window->background_color = 0xFF000000;
        // NOTE: Нет смысла, т.к. после обмена сообщениями с композитором он установит новые значения!
        // window->width = (i32)config->width;
        // window->height = (i32)config->height;
        window->on_close = config->on_close;
        window->on_resize = config->on_resize;
        window->on_focus = config->on_focus;
        window->on_key = config->on_key;
        window->on_mouse_button = config->on_mouse_button;
        window->on_mouse_move = config->on_mouse_move;
        window->on_mouse_wheel = config->on_mouse_wheel;

        // Инициализация поверхности окна.
        window->surface = wl_compositor_create_surface(client->compositor);
        window->xsurface = xdg_wm_base_get_xdg_surface(client->xbase, window->surface);
        xdg_surface_add_listener(window->xsurface, &xsurface_listeners, window);

        window->xtoplevel = xdg_surface_get_toplevel(window->xsurface);
        xdg_toplevel_add_listener(window->xtoplevel, &xtoplevel_listeners, window);

        xdg_toplevel_set_title(window->xtoplevel, window->title);
        xdg_toplevel_set_app_id(window->xtoplevel, window->title);

        // NOTE: Для полноэкранного режима по умолчанию, раскомментируете ниже.
        // xdg_toplevel_set_fullscreen(window->xtoplevel, nullptr);
        // xdg_toplevel_set_maximized();

        // Настройка поверхности.
        wl_surface_commit(window->surface);
        wl_display_roundtrip(client->display);

        // Создание буфера кадра для отображения окна до рендера.
        window->background = wl_buffer_create(window->shm, config->width, config->height, window->background_color);
        if(!window->background)
        {
            LOG_ERROR("Failed to create background buffer.");
            wayland_backend_window_destroy(window, client);
            return nullptr;
        }

        // Подтверждение изменений и обработка событий и захват кадра.
        wl_surface_attach(window->surface, window->background, 0, 0);
        wl_surface_commit(window->surface);
        wl_display_flush(client->display);

        LOG_TRACE("Window '%s' created successfully (awaiting compositor configuration).", window->title);

        client->window = window;
        return window;
    }

    void wayland_backend_window_destroy(platform_window* window, void* internal_data)
    {
        wayland_client* client = internal_data;

        LOG_TRACE("Destroying window '%s'...", window->title);

        // TODO: Удаление окна из списка окон.

        if(window->xtoplevel)  xdg_toplevel_destroy(window->xtoplevel);
        if(window->xsurface)   xdg_surface_destroy(window->xsurface);
        if(window->surface)    wl_surface_destroy(window->surface);
        if(window->background) wl_buffer_destroy(window->background);
        if(window->title)      string_free(window->title);

        mfree(window, sizeof(platform_window), MEMORY_TAG_APPLICATION);
        client->window = nullptr;
        LOG_TRACE("Window destroy complete.");
    }

    const char* wayland_backend_window_get_title(platform_window* window)
    {
        return window->title;
    }

    void wayland_backend_window_get_resolution(platform_window* window, u32* width, u32* height)
    {
        *width = window->width;
        *height = window->height;
    }

    void wayland_backend_enumerate_vulkan_extensions(u32* extension_count, const char** out_extensions)
    {
        static const char* extensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
        };

        if(out_extensions == nullptr)
        {
            *extension_count = sizeof(extensions) / sizeof(char*);
            return;
        }

        mcopy(out_extensions, extensions, sizeof(extensions));
    }

    u32 wayland_backend_create_vulkan_surface(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void** out_vulkan_surface)
    {
        VkWaylandSurfaceCreateInfoKHR surface_create_info = {
            .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
            .display = window->display,
            .surface = window->surface
        };

        return vkCreateWaylandSurfaceKHR(vulkan_instance, &surface_create_info, vulkan_allocator, (VkSurfaceKHR*)out_vulkan_surface);
    }

    void wayland_backend_destroy_vulkan_surface(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void* vulkan_surface)
    {
        UNUSED(window);
        vkDestroySurfaceKHR(vulkan_instance, vulkan_surface, vulkan_allocator);
    }

    u32 wayland_backend_supports_vulkan_presentation(platform_window* window, void* vulkan_pyhical_device, u32 queue_family_index)
    {
        return vkGetPhysicalDeviceWaylandPresentationSupportKHR(vulkan_pyhical_device, queue_family_index, window->display);
    }

    void registry_add(void* data, struct wl_registry* registry, u32 name, const char* interface, u32 version)
    {
        static const u32 compositor_version = 4;
        static const u32 xdg_wm_base_version = 3;
        static const u32 seat_version = 7;
        static const u32 shm_version = 1;
        wayland_client* client = data;

        LOG_TRACE("[%02u] %s (version %u).", name, interface, version);

        if(string_equal(interface, wl_compositor_interface.name))
        {
            client->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, MIN(version, compositor_version));
            LOG_TRACE("  -> Bound compositor v%d.", MIN(version, compositor_version));
        }
        else if(string_equal(interface, xdg_wm_base_interface.name))
        {
            client->xbase = wl_registry_bind(registry, name, &xdg_wm_base_interface, MIN(version, xdg_wm_base_version));
            if(client->xbase)
            {
                xdg_wm_base_add_listener(client->xbase, &xbase_listeners, client);
                LOG_TRACE("  -> Bound XDG WM base v%d.", MIN(version, xdg_wm_base_version));
            }
        }
        else if(string_equal(interface, wl_seat_interface.name))
        {
            client->seat = wl_registry_bind(registry, name, &wl_seat_interface, MIN(version, seat_version));
            if(client->seat)
            {
                wl_seat_add_listener(client->seat, &seat_listeners, client);
                LOG_TRACE("  -> Bound seat v%d.", MIN(version, seat_version));
            }
        }
        else if(string_equal(interface, wl_shm_interface.name))
        {
            client->shm = wl_registry_bind(registry, name, &wl_shm_interface, MIN(version, shm_version));
            LOG_TRACE("  -> Bound SHM v%d", MIN(version, shm_version));
        }

        // TODO: Добавьте другие интерфейсы по необходимости:
        //        - wl_output_interface: для multi-monitor поддержки.
        //        - zwp_linux_dmabuf_v1_interface: для Vulkan DMA-BUF.
        //        - wl_drm_interface: для OpenGL EGL.
    }

    void registry_remove(void* data, struct wl_registry* registry, u32 name)
    {
        UNUSED(name);
        UNUSED(data);
        UNUSED(registry);

        // NOTE: Происходит при удалении мыши, монитора, клавиатуры, графического планшета.
        LOG_TRACE("Wayland has not yet supported remove this event (id %02u).", name);
    }

    void xbase_ping(void* data, struct xdg_wm_base *xbase, u32 serial)
    {
        UNUSED(data);

        // Ping/pong механизм проверяет доступность всего Wayland соединения.
        xdg_wm_base_pong(xbase, serial);
    }

    void xsurface_configure(void* data, struct xdg_surface* xsurface, u32 serial)
    {
        platform_window* window = data;

        // Применение изменений.
        // NOTE: Изменения можно делать, только после подтверждения.
        xdg_surface_ack_configure(xsurface, serial);

        if(window->resize_pending)
        {
            LOG_TRACE("Resize event: window='%s' to %ux%u.", window->title, window->width_pending, window->height_pending);

            // Вызов обработчика изменения размера буфера.
            if(window->on_resize)
            {
                window->on_resize(window->width_pending, window->height_pending);
            }

            // TODO: Запретить обновлять при рендере!
            // Пересоздание буфера с новым размером, но только если он был создан.
            if(window->background)
            {
                wl_buffer_destroy(window->background);
                window->background = wl_buffer_create(
                    window->shm, window->width_pending, window->height_pending, window->background_color
                );

                wl_surface_attach(window->surface, window->background, 0, 0);
                wl_surface_commit(window->surface);
                wl_display_flush(window->display);
            }

            window->width = window->width_pending;
            window->height = window->height_pending;
            window->resize_pending = false;
        }
    }

    void xtoplevel_configure(void* data, struct xdg_toplevel* xtoplevel, i32 width, i32 height, struct wl_array* states)
    {
        UNUSED(states);
        UNUSED(xtoplevel);

        if(width < 0 || height < 0)
        {
            LOG_ERROR("Invalid window dimensions: %ix%i.", width, height);
            return;
        }

        u32 width_u = (u32)width;
        u32 height_u = (u32)height;

        platform_window* window = data;
        if(window->width != width_u || window->height != height_u)
        {
            window->width_pending  = width_u;
            window->height_pending = height_u;
            window->resize_pending = true;
            LOG_TRACE("Window resize pending: %ux%u.", width_u, height_u);
        }
    }

    void xtoplevel_close(void* data, struct xdg_toplevel* xtoplevel)
    {
        UNUSED(xtoplevel);

        // TODO: Закрывать только когда фокус на окне!
        platform_window* window = data;
        LOG_TRACE("Close event: window='%s'.", window->title);

        if(window->on_close)
        {
            window->on_close();
        }
    }

    void xtoplevel_configure_bounds(void* data, struct xdg_toplevel* xtoplevel, i32 width, i32 height)
    {
        UNUSED(data);
        UNUSED(xtoplevel);
        UNUSED(width);
        UNUSED(height);
    }

    void xtoplevel_wm_capabilities(void* data, struct xdg_toplevel* xtoplevel, struct wl_array* capabilities)
    {
        UNUSED(data);
        UNUSED(xtoplevel);
        UNUSED(capabilities);
    }

    void seat_capabilities(void* data, struct wl_seat* seat, u32 capabilities)
    {
        wayland_client* client = data;

        if(capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
        {
            if(client->keyboard)
            {
                wl_keyboard_release(client->keyboard);
                client->keyboard = nullptr;
                LOG_TRACE("Seat keyboard is lost.");
            }
            else
            {
                client->keyboard = wl_seat_get_keyboard(seat);
                wl_keyboard_add_listener(client->keyboard, &keyboard_listeners, client);
                LOG_TRACE("Seat keyboard is found.");
            }
        }

        if(capabilities & WL_SEAT_CAPABILITY_POINTER)
        {
            if(client->pointer)
            {
                wl_pointer_release(client->pointer);
                client->pointer = nullptr;
                LOG_TRACE("Seat pointer is lost.");
            }
            else
            {
                client->pointer = wl_seat_get_pointer(seat);
                wl_pointer_add_listener(client->pointer, &pointer_listeners, client);
                LOG_TRACE("Seat pointer is found.");
            }
        }
    }

    void seat_name(void* data, struct wl_seat* seat, const char* name)
    {
        UNUSED(name);
        UNUSED(data);
        UNUSED(seat);

        LOG_TRACE("Seat interface '%s' found.", name);
    }

    void kb_keymap(void* data, struct wl_keyboard* keyboard, u32 format, i32 fd, u32 size)
    {
        UNUSED(data);
        UNUSED(keyboard);
        UNUSED(format);
        UNUSED(fd);
        UNUSED(size);

        wayland_client* client = data;
        if(format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
        {
            LOG_FATAL("No keymap format.");
            close(fd);
            return;
        }

        char* keymap_str = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if(keymap_str == MAP_FAILED)
        {
            LOG_FATAL("Failed mapped keymap.");
            close(fd);
            return;
        }

        struct xkb_keymap* keymap = xkb_keymap_new_from_string(
            client->xkbcontext, keymap_str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS
        );
        munmap(keymap_str, size);
        close(fd);

        if(!keymap)
        {
            return;
        }

        // Создание состояния xkb.
        if(client->xkbstate)
        {
            xkb_state_unref(client->xkbstate);
        }
        client->xkbstate = xkb_state_new(keymap);

    #ifdef DEBUG_PLATFORM_FLAG
        xkb_keycode_t min = xkb_keymap_min_keycode(keymap);
        xkb_keycode_t max = xkb_keymap_max_keycode(keymap);
        LOG_TRACE("Keycode range %u...%u.", min, max);
    #endif

        xkb_keymap_unref(keymap);
        LOG_TRACE("Keyboard keymap event.");
    }

    void kb_enter(void* data, struct wl_keyboard* keyboard, u32 serial, struct wl_surface* surface, struct wl_array* keys)
    {
        UNUSED(data);
        UNUSED(keyboard);
        UNUSED(serial);
        UNUSED(surface);
        UNUSED(keys);

        // TODO: Найти окно которому принадлежит surface!
        // if(client->window->surface == surface)
        // {
        //     client->focused_window = client->window;
        //     client->window->focused = true;
        //     LOG_TRACE("Window '%s' focused.", client->window->title);
        // }

        wayland_client* client = data;
        LOG_TRACE("Window '%s' focused.", client->window->title);

        if(client->window->on_focus)
        {
            client->window->on_focus(true);
        }
    }

    void kb_leave(void* data, struct wl_keyboard* keyboard, u32 serial, struct wl_surface* surface)
    {
        UNUSED(data);
        UNUSED(keyboard);
        UNUSED(serial);
        UNUSED(surface);

        // TODO: Найти окно которому принадлежит surface!
        // if(client->window->surface == surface)
        // {
        //     client->window->focused = false;
        //     if(client->focused_window == client->window)
        //     {
        //         client->focused_window = nullptr;
        //     }
        //     LOG_TRACE("Window '%s' lost focus.", client->window->title);
        // }

        wayland_client* client = data;
        LOG_TRACE("Window '%s' lost focus.", client->window->title);

        if(client->window->on_focus)
        {
            client->window->on_focus(false);
        }
    }

    static keyboard_key translate_key(u32 scancode)
    {
        // Таблица трансляции нормализованных evdev scancode -> virtual keycode. См. linux/input-event-codes.h
        static const keyboard_key codes[KEY_COUNT] = {
            [0x01] = KEY_ESCAPE,       [0x02] = KEY_1,            [0x03] = KEY_2,            [0x04] = KEY_3,
            [0x05] = KEY_4,            [0x06] = KEY_5,            [0x07] = KEY_6,            [0x08] = KEY_7,
            [0x09] = KEY_8,            [0x0A] = KEY_9,            [0x0B] = KEY_0,            [0x0C] = KEY_MINUS,
            [0x0D] = KEY_EQUAL,        [0x0E] = KEY_BACKSPACE,    [0x0F] = KEY_TAB,          [0x10] = KEY_Q,
            [0x11] = KEY_W,            [0x12] = KEY_E,            [0x13] = KEY_R,            [0x14] = KEY_T,
            [0x15] = KEY_Y,            [0x16] = KEY_U,            [0x17] = KEY_I,            [0x18] = KEY_O,
            [0x19] = KEY_P,            [0x1A] = KEY_LBRACKET,     [0x1B] = KEY_RBRACKET,     [0x1C] = KEY_RETURN,
            [0x1D] = KEY_LCONTROL,     [0x1E] = KEY_A,            [0x1F] = KEY_S,            [0x20] = KEY_D,
            [0x21] = KEY_F,            [0x22] = KEY_G,            [0x23] = KEY_H,            [0x24] = KEY_J,
            [0x25] = KEY_K,            [0x26] = KEY_L,            [0x27] = KEY_SEMICOLON,    [0x28] = KEY_APOSTROPHE,
            [0x29] = KEY_GRAVE,        [0x2A] = KEY_LSHIFT,       [0x2B] = KEY_BACKSLASH,    [0x2C] = KEY_Z,
            [0x2D] = KEY_X,            [0x2E] = KEY_C,            [0x2F] = KEY_V,            [0x30] = KEY_B,
            [0x31] = KEY_N,            [0x32] = KEY_M,            [0x33] = KEY_COMMA,        [0x34] = KEY_DOT,
            [0x35] = KEY_SLASH,        [0x36] = KEY_RSHIFT,       [0x37] = KEY_MULTIPLY,     [0x38] = KEY_LALT,
            [0x39] = KEY_SPACE,        [0x3A] = KEY_CAPSLOCK,     [0x3B] = KEY_F1,           [0x3C] = KEY_F2,
            [0x3D] = KEY_F3,           [0x3E] = KEY_F4,           [0x3F] = KEY_F5,           [0x40] = KEY_F6,
            [0x41] = KEY_F7,           [0x42] = KEY_F8,           [0x43] = KEY_F9,           [0x44] = KEY_F10,
            [0x45] = KEY_NUMLOCK,      [0x46] = KEY_SCROLLLOCK,   [0x47] = KEY_NUMPAD7,      [0x48] = KEY_NUMPAD8,
            [0x49] = KEY_NUMPAD9,      [0x4A] = KEY_SUBTRACT,     [0x4B] = KEY_NUMPAD4,      [0x4C] = KEY_NUMPAD5,
            [0x4D] = KEY_NUMPAD6,      [0x4E] = KEY_ADD,          [0x4F] = KEY_NUMPAD1,      [0x50] = KEY_NUMPAD2,
            [0x51] = KEY_NUMPAD3,      [0x52] = KEY_NUMPAD0,      [0x53] = KEY_DECIMAL,      [0x57] = KEY_F11,
            [0x58] = KEY_F12,          [0x60] = KEY_RETURN,       [0x61] = KEY_RCONTROL,     [0x62] = KEY_DIVIDE,
            [0x63] = KEY_PRINTSCREEN,  [0x64] = KEY_RALT,         [0x66] = KEY_HOME,         [0x67] = KEY_UP,
            [0x68] = KEY_PAGEUP,       [0x69] = KEY_LEFT,         [0x6A] = KEY_RIGHT,        [0x6B] = KEY_END,
            [0x6C] = KEY_DOWN,         [0x6D] = KEY_PAGEDOWN,     [0x6E] = KEY_INSERT,       [0x6F] = KEY_DELETE,
            [0x77] = KEY_PAUSE,        [0x7D] = KEY_LSUPER,       [0x7E] = KEY_RSUPER,       [0x7F] = KEY_MENU,
            [0x8e] = KEY_SLEEP,        [0xB7] = KEY_F13,          [0xB8] = KEY_F14,          [0xB9] = KEY_F15,
            [0xBA] = KEY_F16,          [0xBB] = KEY_F17,          [0xBC] = KEY_F18,          [0xBD] = KEY_F19,
            [0xBE] = KEY_F20,          [0xBF] = KEY_F21,          [0xC0] = KEY_F22,          [0xC1] = KEY_F23,
            [0xC2] = KEY_F24,
        };

        if(scancode >= KEY_COUNT)
        {
            return KEY_UNKNOWN;
        }

        return codes[scancode];
    }

    void kb_key(void* data, struct wl_keyboard* keyboard, u32 serial, u32 time, u32 key, u32 state)
    {
        UNUSED(keyboard);
        UNUSED(serial);
        UNUSED(time);

        wayland_client* client = data;
        keyboard_key vkey = translate_key(key);
        // xkb_keysym_t keysym = xkb_state_key_get_one_sym(client->xkbstate, key + 8);

        if(vkey == KEY_UNKNOWN)
        {
            LOG_WARN("Keyboard key event: Unknown scancode=0x%x (%d), state=%d.", key, key, state);
            return;
        }

        LOG_TRACE("Keyboard key event: scancode=0x%x (%d), state=%d, window='%s'.", key, key, state, client->window->title);
        if(client->window->on_key)
        {
            client->window->on_key(vkey, state == WL_KEYBOARD_KEY_STATE_PRESSED);
        }
    }

    void kb_mods(void* data, struct wl_keyboard* keyboard, u32 serial, u32 depressed, u32 latched, u32 locked, u32 group)
    {
        UNUSED(data);
        UNUSED(keyboard);
        UNUSED(serial);
        UNUSED(depressed);
        UNUSED(latched);
        UNUSED(locked);
        UNUSED(group);
    }

    void kb_repeat(void* data, struct wl_keyboard* keyboard, i32 rate, i32 delay)
    {
        UNUSED(data);
        UNUSED(keyboard);
        UNUSED(rate);
        UNUSED(delay);
    }

    void pt_enter(void* data, struct wl_pointer* pointer, u32 serial, struct wl_surface* surface, wl_fixed_t x, wl_fixed_t y)
    {
        UNUSED(data);
        UNUSED(pointer);
        UNUSED(serial);
        UNUSED(surface);
        UNUSED(x);
        UNUSED(y);
    }

    void pt_leave(void* data, struct wl_pointer* pointer, u32 serial, struct wl_surface* surface)
    {
        UNUSED(data);
        UNUSED(pointer);
        UNUSED(serial);
        UNUSED(surface);
    }

    void pt_motion(void* data, struct wl_pointer* pointer, u32 time, wl_fixed_t x, wl_fixed_t y)
    {
        UNUSED(data);
        UNUSED(pointer);
        UNUSED(time);
        UNUSED(x);
        UNUSED(y);

        wayland_client* client = data;
        i32 position_x = wl_fixed_to_int(x);
        i32 position_y = wl_fixed_to_int(y);

        // LOG_TRACE("Motion event to %dx%d for '%s' window.", position_x, position_y, client->window->title);
        if(client->window->on_mouse_move)
        {
            client->window->on_mouse_move(position_x, position_y);
        }
    }

    static mouse_button translate_button(u32 scancode)
    {
        // В целях оптимизации (коды кнопок 0x110...0x114).
        scancode -= 0x110;

        // Таблица трансляции нормализованных evdev scancode -> virtual button code. См. linux/input-event-codes.h
        static const mouse_button codes[BTN_COUNT - 1] = {
            [0x00] = BTN_LEFT, [0x01] = BTN_RIGHT, [0x02] = BTN_MIDDLE, [0x03] = BTN_BACKWARD, [0x04] = BTN_FORWARD
        };

        if(scancode >= (BTN_COUNT - 1))
        {
            return BTN_UNKNOWN;
        }

        return codes[scancode];
    }

    void pt_button(void* data, struct wl_pointer* pointer, u32 serial, u32 time, u32 button, u32 state)
    {
        UNUSED(data);
        UNUSED(pointer);
        UNUSED(serial);
        UNUSED(time);
        UNUSED(button);
        UNUSED(state);

        wayland_client* client = data;
        mouse_button vbutton = translate_button(button);

        if(vbutton == BTN_UNKNOWN)
        {
            LOG_WARN("Mouse button event: unknown scancode=0x%x (%d), state=%d.", button, button, state);
            return;
        }

        LOG_TRACE("Mouse button event: scancode=0x%x (%d), state=%d, window='%s'.", button, button, state, client->window->title);
        if(client->window->on_mouse_button)
        {
            client->window->on_mouse_button(vbutton, state == WL_POINTER_BUTTON_STATE_PRESSED);
        }
    }

    void pt_axis(void* data, struct wl_pointer* pointer, u32 time, u32 axis, wl_fixed_t value)
    {
        UNUSED(data);
        UNUSED(pointer);
        UNUSED(time);
        UNUSED(axis);
        UNUSED(value);
    }

    void pt_frame(void* data, struct wl_pointer* pointer)
    {
        UNUSED(data);
        UNUSED(pointer);
    }

    void pt_axis_source(void* data, struct wl_pointer* pointer, u32 axis_source)
    {
        UNUSED(data);
        UNUSED(pointer);
        UNUSED(axis_source);
    }

    void pt_axis_stop(void* data, struct wl_pointer* pointer, u32 time, u32 axis)
    {
        UNUSED(data);
        UNUSED(pointer);
        UNUSED(time);
        UNUSED(axis);
    }

    void pt_axis_discrete(void* data, struct wl_pointer* pointer, u32 axis, i32 discrete)
    {
        UNUSED(data);
        UNUSED(pointer);
        UNUSED(axis);
        UNUSED(discrete);

        wayland_client* client = data;
        i32 vertical_delta = 0;
        i32 horizontal_delta = 0;

        if(axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
        {
            vertical_delta = -discrete;
        }
        else if(axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
        {
            // TODO: Необходимо протестировать! Нет реального устройства.
            horizontal_delta = discrete;
        }
        else
        {
            LOG_WARN("Mouse wheel event: unsupported axis - %u.", axis);
            return;
        }
        
        LOG_TRACE("Mouse wheel event: vertical delta=%i, horizontal delta=%i.", vertical_delta, horizontal_delta);
        if(client->window->on_mouse_wheel)
        {
            client->window->on_mouse_wheel(vertical_delta, horizontal_delta);
        }
    }

    void pt_axis_value120(void* data, struct wl_pointer* pointer, u32 axis, i32 value120)
    {
        UNUSED(data);
        UNUSED(pointer);
        UNUSED(axis);
        UNUSED(value120);
    }

    void pt_axis_relative_direction(void* data, struct wl_pointer* pointer, u32 axis, u32 direction)
    {
        UNUSED(data);
        UNUSED(pointer);
        UNUSED(axis);
        UNUSED(direction);
    }

    struct wl_buffer* wl_buffer_create(struct wl_shm* shm, u32 width, u32 height, u32 argb_color)
    {
        u32 stride = width * 4;        // 4 x 8bit = 32bit (4 байта - а это тип i32).
        u32 size = stride * height;

        i32 fd = memfd_create("wayland-buffer", MFD_CLOEXEC);
        if(fd < 0)
        {
            LOG_ERROR("Failed to create shared memory: %s.", strerror(errno));
            return nullptr;
        }

        if(ftruncate(fd, size) < 0)
        {
            LOG_ERROR("Failed to allocate %d bytes: %s.", size, strerror(errno));
            close(fd);
            return nullptr;
        }

        u32* data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if(data == MAP_FAILED)
        {
            LOG_ERROR("Failed to map memory: %s.", strerror(errno));
            close(fd);
            return nullptr;
        }

        // Заполнение буфера заданным цветом (ARGB).
        u32 count = width * height;
        for (u32 i = 0; i < count; ++i)
        {
            data[i] = argb_color;
        }
        munmap(data, size);

        struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
        struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, (i32)width, (i32)height, (i32)stride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);
        close(fd);

        return buffer;
    }

#endif
