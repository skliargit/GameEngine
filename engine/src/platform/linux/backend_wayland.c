#include "platform/linux/backend_wayland.h"

#if PLATFORM_LINUX_FLAG

    // Нужна для memfd_create.
    #define _GNU_SOURCE 1

    #include "core/logger.h"
    #include "platform/string.h"
    #include "platform/linux/wayland_xdg_protocol.h"

    #include <wayland-client.h>
    #include <errno.h>
    #include <string.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/mman.h>
    #include <poll.h>

    // TODO: Обработчики событий окна.
    typedef struct wl_internal_data {
        // Для работы с окном приложения.
        struct wl_display* display;
        struct wl_registry* registry;
        struct wl_compositor* compositor;
        struct wl_surface* surface;
        struct xdg_wm_base* xbase;
        struct xdg_surface* xsurface;
        struct xdg_toplevel* xtoplevel;
        struct wl_shm* shm;
        struct wl_buffer* background;
        // Для работы с устройствами ввода.
        struct wl_seat* seat;
        struct wl_keyboard* keyboard;
        struct wl_pointer* pointer;
        // Обработка событий.
        struct pollfd poll_fd[1];
        nfds_t poll_fd_count;
        // Кэшированые значения.
        i32 displayfd;
        i32 width;
        i32 height;
        // Флаги.
        bool do_resize;
        bool should_close;
    } wl_internal_data;

    // Объявление функции для создания буфера.
    static struct wl_buffer* wl_buffer_create(wl_internal_data* context, i32 width, i32 height, u32 argb_color);

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

    bool wayland_backend_create(const platform_window_config* config, void* internal_data)
    {
        wl_internal_data* context = internal_data;

        // Установка соединения с wayland сервером.
        context->display = wl_display_connect(nullptr);
        if(!context->display)
        {
            LOG_ERROR("%s: Failed to connect to wayland display.", __func__);
            return false;
        }

        // Получение файлового дескриптора контекста.
        context->displayfd = wl_display_get_fd(context->display);
        if(context->displayfd == -1)
        {
            LOG_ERROR("%s: Failed to get wayland faile descriptor.", __func__);
            wl_display_disconnect(context->display);
            return false;
        }

        // Настройка чтения событий из дескриптора.
        context->poll_fd[0].fd = context->displayfd;
        context->poll_fd[0].events = POLLIN;
        context->poll_fd_count = sizeof(context->poll_fd) / sizeof(struct pollfd);

        context->registry = wl_display_get_registry(context->display);
        wl_registry_add_listener(context->registry, &registry_listeners, context);

        // Синхронизация для получения глобальных объектов.
        LOG_TRACE("Supported wayland protocols in current environment:");
        wl_display_roundtrip(context->display);

        // Проверка поддержки интерфейсов.
        if(!context->compositor)
        {
            LOG_ERROR("%s: Failed to bind 'wl_compositor' interface.", __func__);
            return false;
        }

        if(!context->xbase)
        {
            LOG_ERROR("%s: Failed to bind 'xdg_wm_base' interface.", __func__);
            return false;
        }

        if(!context->seat)
        {
            LOG_ERROR("%s: Failed to bind 'seat' interface.", __func__);
            return false;
        }

        // Инициализация поверхности окна.
        context->surface = wl_compositor_create_surface(context->compositor);
        context->xsurface = xdg_wm_base_get_xdg_surface(context->xbase, context->surface);
        xdg_surface_add_listener(context->xsurface, &xsurface_listeners, context);

        context->xtoplevel = xdg_surface_get_toplevel(context->xsurface);
        xdg_toplevel_add_listener(context->xtoplevel, &xtoplevel_listeners, context);

        xdg_toplevel_set_title(context->xtoplevel, config->title);
        xdg_toplevel_set_app_id(context->xtoplevel, config->title);

        // NOTE: Для полноэкранного режима по умолчанию, раскомментируете ниже.
        // xdg_toplevel_set_fullscreen(context->xtoplevel, null);
        // xdg_toplevel_set_maximized();

        // Настройка поверхности.
        // NOTE: Вызывается для ... до нее захват буфера работать не будет!
        wl_surface_commit(context->surface);
        wl_display_roundtrip(context->display);

        // Создание буфера кадра для отображения окна до рендера.
        context->background = wl_buffer_create(context, config->width, config->height, 0xFF101010);
        if(!context->background)
        {
            LOG_ERROR("%s: Failed to create background buffer.", __func__);
            return false;
        }

        // Подтверждение изменений и обработка событий и захват кадра.
        wl_surface_attach(context->surface, context->background, 0, 0);
        wl_surface_commit(context->surface);
        wl_display_flush(context->display);

        return true;
    }

    void wayland_backend_destroy(void* internal_data)
    {
        wl_internal_data* context = internal_data;

        if(context->display)
        {
            if(context->pointer)    wl_pointer_destroy(context->pointer);
            if(context->keyboard)   wl_keyboard_destroy(context->keyboard);
            if(context->seat)       wl_seat_destroy(context->seat);
            if(context->xtoplevel)  xdg_toplevel_destroy(context->xtoplevel);
            if(context->xsurface)   xdg_surface_destroy(context->xsurface);
            if(context->xbase)      xdg_wm_base_destroy(context->xbase);
            if(context->surface)    wl_surface_destroy(context->surface);
            if(context->compositor) wl_compositor_destroy(context->compositor);
            if(context->registry)   wl_registry_destroy(context->registry);

            wl_display_disconnect(context->display);
        }
    }

    bool wayland_backend_poll_events(void* internal_data)
    {
        wl_internal_data* context = internal_data;

        // Проверка очереди событий.
        if(wl_display_prepare_read(context->display) != 0)
        {
            // Обработка очереди сообщений.
            return wl_display_dispatch_pending(context->display) != -1 && !context->should_close;
        }

        // Ожидание новых событий с таймаутом (N > 0 - время ожидания, 0 - без ожидания, -1 - бесконечное ожидание).
        // TODO: Установить ноль при рендере.
        if(poll(context->poll_fd, context->poll_fd_count, 16) > 0)
        {
            // Чтение новых событий.
            wl_display_read_events(context->display);
        }
        else
        {
            // Отмена миссии.
            wl_display_cancel_read(context->display);
        }

        // Обработка новых событий (если есть).
        return wl_display_dispatch_pending(context->display) != -1 && !context->should_close;

        // NOTE: Для обычного окна хватит блокирующего метода.
        // return wl_display_dispatch(context->display) != -1 && !context->should_close;
    }

    bool wayland_backend_is_supported()
    {
        struct wl_display* display = wl_display_connect(nullptr);
        bool result = (display != nullptr);
        if(result) wl_display_disconnect(display);
        return result;
    }

    u64 wayland_backend_instance_memory_requirement()
    {
        return sizeof(wl_internal_data);
    }

    void registry_add(void* data, struct wl_registry* registry, u32 name, const char* interface, u32 version)
    {
        wl_internal_data* context = data;

        if(platform_string_equal(interface, wl_compositor_interface.name))
        {
            context->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
        }
        else if(platform_string_equal(interface, xdg_wm_base_interface.name))
        {
            context->xbase = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
            if(context->xbase) xdg_wm_base_add_listener(context->xbase, &xbase_listeners, context);
        }
        else if(platform_string_equal(interface, wl_seat_interface.name))
        {
            context->seat = wl_registry_bind(registry, name, &wl_seat_interface, 2);
            if(context->seat) wl_seat_add_listener(context->seat, &seat_listeners, context);
        }
        else if(platform_string_equal(interface, wl_shm_interface.name))
        {
            context->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
        }

        LOG_TRACE("[%02u] %s (version %u)", name, interface, version);
    }

    void registry_remove(void* data, struct wl_registry* registry, u32 name)
    {
        (void)data;
        (void)registry;
        (void)name;
        // NOTE: Происходит при удалении мышки, монитора, клавиатуры, графического планшета.
        LOG_TRACE("%s: Wayland has not yet supported this event (id %02u).", __func__, name);
    }

    void xbase_ping(void* data, struct xdg_wm_base *xbase, u32 serial)
    {
        (void)data;
        // NOTE: Проверяется доступность окна, не упало ли оно.
        xdg_wm_base_pong(xbase, serial);
    }

    void xsurface_configure(void* data, struct xdg_surface* xsurface, u32 serial)
    {
        wl_internal_data* context = data;

        // Применение изменений.
        // NOTE: Изменения можно делать, только после подтверждения.
        xdg_surface_ack_configure(xsurface, serial);

        if(context->do_resize)
        {
            LOG_TRACE("%s: Resize event.", __func__); // Т.к. фактическое изменение окна доступно здесь.

            // TODO: Вызов обработчика изменения размера буфера.

            // TODO: Применить изменения (помогает с изменением экрана).
            if(context->background)
            {
                wl_buffer_destroy(context->background);
                context->background = nullptr;
            }

            context->background = wl_buffer_create(context, context->width, context->height, 0xAA101010);
            wl_surface_attach(context->surface, context->background, 0, 0);
            wl_surface_commit(context->surface);
            wl_display_flush(context->display);

            context->do_resize = false;
        }
    }

    void xtoplevel_configure(void* data, struct xdg_toplevel* xtoplevel, i32 width, i32 height, struct wl_array* states)
    {
        (void)data;
        (void)xtoplevel;
        (void)width;
        (void)height;
        (void)states;

        wl_internal_data* context = data;
        if(context->width != width || context->height != height)
        {
            context->do_resize = true;
            context->width  = width;
            context->height = height;
        }
    }

    void xtoplevel_close(void* data, struct xdg_toplevel* xtoplevel)
    {
        (void)xtoplevel;
        wl_internal_data* context = data;
        context->should_close = true;
        LOG_TRACE("%s: Close event.", __func__);
    }

    void xtoplevel_configure_bounds(void* data, struct xdg_toplevel* xtoplevel, i32 width, i32 height)
    {
        (void)data;
        (void)xtoplevel;
        (void)width;
        (void)height;
    }

    void xtoplevel_wm_capabilities(void* data, struct xdg_toplevel* xtoplevel, struct wl_array* capabilities)
    {
        (void)data;
        (void)xtoplevel;
        (void)capabilities;
    }

    void seat_capabilities(void* data, struct wl_seat* seat, u32 capabilities)
    {
        wl_internal_data* context = data;

        if(capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
        {
            if(context->keyboard)
            {
                wl_keyboard_release(context->keyboard);
                context->keyboard = nullptr;
                LOG_TRACE("Wayland seat keyboard is lost.");
            }
            else
            {
                context->keyboard = wl_seat_get_keyboard(seat);
                wl_keyboard_add_listener(context->keyboard, &keyboard_listeners, context);
                LOG_TRACE("Wayland seat keyboard is found.");
            }
        }

        if(capabilities & WL_SEAT_CAPABILITY_POINTER)
        {
            if(context->pointer)
            {
                wl_pointer_release(context->pointer);
                context->pointer = nullptr;
                LOG_TRACE("Wayland seat pointer is lost.");
            }
            else
            {
                context->pointer = wl_seat_get_pointer(seat);
                wl_pointer_add_listener(context->pointer, &pointer_listeners, context);
                LOG_TRACE("Wayland seat pointer is found.");
            }
        }
    }

    void seat_name(void* data, struct wl_seat* seat, const char* name)
    {
        (void)data;
        (void)seat;
        LOG_TRACE("Wayland seat interface %s found.", name);
    }

    void kb_keymap(void* data, struct wl_keyboard* keyboard, u32 format, i32 fd, u32 size)
    {
        (void)data; (void)keyboard; (void)format; (void)fd; (void)size;
    }

    void kb_enter(void* data, struct wl_keyboard* keyboard, u32 serial, struct wl_surface* surface, struct wl_array* keys)
    {
        (void)data;
        (void)keyboard;
        (void)serial;
        (void)surface;
        (void)keys;
    }

    void kb_leave(void* data, struct wl_keyboard* keyboard, u32 serial, struct wl_surface* surface)
    {
        (void)data;
        (void)keyboard;
        (void)serial;
        (void)surface;
    }

    void kb_key(void* data, struct wl_keyboard* keyboard, u32 serial, u32 time, u32 key, u32 state)
    {
        (void)data;
        (void)keyboard;
        (void)serial;
        (void)time;
        (void)key;
        (void)state;
        LOG_TRACE("%s: Key event.", __func__);
    }

    void kb_mods(void* data, struct wl_keyboard* keyboard, u32 serial, u32 depressed, u32 latched, u32 locked, u32 group)
    {
        (void)data;
        (void)keyboard;
        (void)serial;
        (void)depressed;
        (void)latched;
        (void)locked;
        (void)group;
    }

    void kb_repeat(void* data, struct wl_keyboard* keyboard, i32 rate, i32 delay)
    {
        (void)data;
        (void)keyboard;
        (void)rate;
        (void)delay;
    }

    void pt_enter(void* data, struct wl_pointer* pointer, u32 serial, struct wl_surface* surface, wl_fixed_t x, wl_fixed_t y)
    {
        (void)data;
        (void)pointer;
        (void)serial;
        (void)surface;
        (void)x;
        (void)y;
    }

    void pt_leave(void* data, struct wl_pointer* pointer, u32 serial, struct wl_surface* surface)
    {
        (void)data;
        (void)pointer;
        (void)serial;
        (void)surface;
    }

    void pt_motion(void* data, struct wl_pointer* pointer, u32 time, wl_fixed_t x, wl_fixed_t y)
    {
        (void)data;
        (void)pointer;
        (void)time;
        (void)x;
        (void)y;
        // LOG_TRACE("%s: Motion event.", __func__);
    }

    void pt_button(void* data, struct wl_pointer* pointer, u32 serial, u32 time, u32 button, u32 state)
    {
        (void)data;
        (void)pointer;
        (void)serial;
        (void)time;
        (void)button;
        (void)state;
        LOG_TRACE("%s: Button event.", __func__);
    }

    void pt_axis(void* data, struct wl_pointer* pointer, u32 time, u32 axis, wl_fixed_t value)
    {
        (void)data;
        (void)pointer;
        (void)time;
        (void)axis;
        (void)value;
    }

    void pt_frame(void* data, struct wl_pointer* pointer)
    {
        (void)data;
        (void)pointer;
    }

    void pt_axis_source(void* data, struct wl_pointer* pointer, u32 axis_source)
    {
        (void)data;
        (void)pointer;
        (void)axis_source;
    }

    void pt_axis_stop(void* data, struct wl_pointer* pointer, u32 time, u32 axis)
    {
        (void)data;
        (void)pointer;
        (void)time;
        (void)axis;
    }

    void pt_axis_discrete(void* data, struct wl_pointer* pointer, u32 axis, i32 discrete)
    {
        (void)data;
        (void)pointer;
        (void)axis;
        (void)discrete;
    }

    void pt_axis_value120(void* data, struct wl_pointer* pointer, u32 axis, i32 value120)
    {
        (void)data;
        (void)pointer;
        (void)axis;
        (void)value120;
    }

    void pt_axis_relative_direction(void* data, struct wl_pointer* pointer, u32 axis, u32 direction)
    {
        (void)data;
        (void)pointer;
        (void)axis;
        (void)direction;
    }

    struct wl_buffer* wl_buffer_create(wl_internal_data* context, i32 width, i32 height, u32 argb_color)
    {
        i32 stride = width * 4;        // 4 x 8bit = 32bit (4 байта - а это тип i32).
        i32 size = stride * height;

        i32 fd = memfd_create("wayland-buffer", MFD_CLOEXEC);
        if(fd < 0)
        {
            LOG_ERROR("%s: Failed to create shared memory: %s", __func__, strerror(errno));
            return nullptr;
        }

        if(ftruncate(fd, size) < 0)
        {
            LOG_ERROR("%s: Failed to allocate %d bytes: %s", __func__, size, strerror(errno));
            close(fd);
            return nullptr;
        }

        u32* data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if(data == MAP_FAILED)
        {
            LOG_ERROR("%s: Failed to map memory: %s.", __func__, strerror(errno));
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

        struct wl_shm_pool *pool = wl_shm_create_pool(context->shm, fd, size);
        struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);
        close(fd);

        return buffer;
    }

#endif
