#include "platform/linux/wayland_backend.h"

#ifdef PLATFORM_LINUX_FLAG

    // Нужна для memfd_create.
    #define _GNU_SOURCE 1

    #include "core/logger.h"
    #include "core/string.h"
    #include "core/memory.h"
    #include "platform/linux/wayland_xdg_protocol.h"
    #include <wayland-client.h>
    #include <errno.h>
    #include <string.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/mman.h>
    #include <poll.h>

    // Для включения отладки для оконной системы индивидуально.
    #ifndef DEBUG_WINDOW_FLAG
        #undef LOG_DEBUG
        #undef LOG_TRACE
        #define LOG_DEBUG(...) UNUSED(0)
        #define LOG_TRACE(...) UNUSED(0)
    #endif

    typedef struct platform_window {
        struct wl_display* display;       // Соединение с Wayland сервером (НАСЛЕДУЕТСЯ).
        struct wl_surface* surface;       // Wayland поверхность окна.
        struct xdg_surface* xsurface;     // XDG поверхность для управления.
        struct xdg_toplevel* xtoplevel;   // XDG toplevel для оконных операций.
        struct wl_shm* shm;               // Разделяемая память для буферов (НАСЛЕДУЕТСЯ).
        struct wl_buffer* background;     // Временный буфер фона (до рендеринга).
        u32 background_color;             // Цвет фона в формате ARGB (0xAARRGGBB).
        char* title;                      // Текущий заголовок окна.
        i32 width;                        // Текущая ширина окна в пикселях.
        i32 height;                       // Текущая высота окна в пикселях.
        i32 width_pending;                // Новая ширина окна, ожидающая применения.
        i32 height_pending;               // Новая высота окна, ожидающая применения.
        bool resize_pending;              // Флаг ожидания обработки изменения размера.
        bool should_close;                // TODO: Флаг запроса на закрытие окна (пока что вылет с ошибкой).
        bool focused;                     // Фокус ввода на этом окне.
    } platform_window;

    typedef struct wayland_client {
        struct wl_display* display;       // Соединение с Wayland сервером.
        struct wl_registry* registry;     // Регистратор глобальных объектов.
        struct wl_compositor* compositor; // Композитор для создания поверхностей.
        struct xdg_wm_base* xbase;        // XDG базовый протокол для окон.
        struct wl_shm* shm;               // Разделяемая память для буферов.
        struct wl_seat* seat;             // Устройства ввода (клавиатура, мышь).
        struct wl_keyboard* keyboard;     // Клавиатура для обработки ввода.
        struct wl_pointer* pointer;       // Мышка для обработки ввода.
        struct pollfd poll_fd[1];         // Файловые дескрипторы для poll().
        nfds_t poll_fd_count;             // Количество отслеживаемых дескрипторов.
        i32 displayfd;                    // Файловый дескриптор Wayland соединения.
        platform_window* focused_window;  // TODO: Окно с текущим фокусом ввода.
        platform_window* window;          // TODO: Динамический массив всех окон (пока только одно окно).
    } wayland_client;

    // Объявление функции для создания буфера.
    static struct wl_buffer* wl_buffer_create(struct wl_shm* shm, i32 width, i32 height, u32 argb_color);

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
            LOG_WARN("No seat interface found - input devices won't be available.");
        }
        else
        {
            LOG_TRACE("Seat interface available.");
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

        // TODO: Временная мера!
        // Проверка на закрытие окна
        if(client->window->should_close)
        {
            LOG_TRACE("Window close requested, stopping event loop.");
            return false;
        }

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

        LOG_TRACE("Creating window '%s' (size: %dx%d)...", config->title, config->width, config->height);

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

    void registry_add(void* data, struct wl_registry* registry, u32 name, const char* interface, u32 version)
    {
        static const u32 compositor_version = 4;
        static const u32 xdg_wm_base_version = 3;
        static const u32 seat_version = 7;
        static const u32 shm_version = 1;
        wayland_client* client = data;

        // TODO: Ограничить вывод списка сообщений идним полным проходом?
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

        // NOTE: Происходит при удалении мышки, монитора, клавиатуры, графического планшета.
        LOG_TRACE("Wayland has not yet supported remove this event (id %02u).", name);
    }

    void xbase_ping(void* data, struct xdg_wm_base *xbase, u32 serial)
    {
        UNUSED(data);

        // NOTE: Ping/pong механизм проверяет доступность всего Wayland соединения.
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
            LOG_TRACE("Resize event for window '%s' to %dx%d.", window->title, window->width_pending, window->height_pending);

            // TODO: Вызов обработчика изменения размера буфера.

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

        platform_window* window = data;
        if(window->width != width || window->height != height)
        {
            window->width_pending  = width;
            window->height_pending = height;
            window->resize_pending = true;
            LOG_TRACE("Window resize pending: %dx%d.", width, height);
        }
    }

    void xtoplevel_close(void* data, struct xdg_toplevel* xtoplevel)
    {
        UNUSED(xtoplevel);

        // TODO: Закрывать только когда фокус на окне!
        platform_window* window = data;
        window->should_close = true;
        LOG_TRACE("Close event for window '%s'.", window->title);
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
    }

    void kb_enter(void* data, struct wl_keyboard* keyboard, u32 serial, struct wl_surface* surface, struct wl_array* keys)
    {
        UNUSED(data);
        UNUSED(keyboard);
        UNUSED(serial);
        UNUSED(surface);
        UNUSED(keys);
    }

    void kb_leave(void* data, struct wl_keyboard* keyboard, u32 serial, struct wl_surface* surface)
    {
        UNUSED(data);
        UNUSED(keyboard);
        UNUSED(serial);
        UNUSED(surface);
    }

    void kb_key(void* data, struct wl_keyboard* keyboard, u32 serial, u32 time, u32 key, u32 state)
    {
        UNUSED(data);
        UNUSED(keyboard);
        UNUSED(serial);
        UNUSED(time);
        UNUSED(key);
        UNUSED(state);

        LOG_TRACE("Key event.");
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

        // TODO: Найти окно которому принадлежит surface!
        // if(client->window->surface == surface)
        // {
        //     client->focused_window = client->window;
        //     client->window->focused = true;
        //     LOG_TRACE("Window '%s' focused.", client->window->title);
        // }

        wayland_client* client = data;
        UNUSED(client);
        LOG_TRACE("Window '%s' focused.", client->window->title);
    }

    void pt_leave(void* data, struct wl_pointer* pointer, u32 serial, struct wl_surface* surface)
    {
        UNUSED(data);
        UNUSED(pointer);
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
        //     LOG_TRACE("Window '%s' has lost focus.", client->window->title);
        // }

        wayland_client* client = data;
        UNUSED(client);
        LOG_TRACE("Window '%s' has lost focus.", client->window->title);
    }

    void pt_motion(void* data, struct wl_pointer* pointer, u32 time, wl_fixed_t x, wl_fixed_t y)
    {
        UNUSED(data);
        UNUSED(pointer);
        UNUSED(time);
        UNUSED(x);
        UNUSED(y);

        // LOG_TRACE("Motion event.");
    }

    void pt_button(void* data, struct wl_pointer* pointer, u32 serial, u32 time, u32 button, u32 state)
    {
        UNUSED(data);
        UNUSED(pointer);
        UNUSED(serial);
        UNUSED(time);
        UNUSED(button);
        UNUSED(state);

        LOG_TRACE("Button event.");
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

    struct wl_buffer* wl_buffer_create(struct wl_shm* shm, i32 width, i32 height, u32 argb_color)
    {
        i32 stride = width * 4;        // 4 x 8bit = 32bit (4 байта - а это тип i32).
        i32 size = stride * height;

        if(width <= 0 || height <= 0)
        {
            LOG_ERROR("Invalid buffer dimensions: %dx%d.", width, height);
            return nullptr;
        }

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
        i32 count = width * height;
        for (int i = 0; i < count; ++i)
        {
            data[i] = argb_color;
        }
        munmap(data, size);

        struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
        struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);
        close(fd);

        return buffer;
    }

#endif
