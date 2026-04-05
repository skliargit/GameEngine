#include "platform/linux/wayland_backend.h"

#ifdef PLATFORM_LINUX_FLAG

    // Нужна для memfd_create.
    #define _GNU_SOURCE 1

    #include "core/logger.h"
    #include "core/string.h"
    #include "core/memory.h"
    #include "platform/linux/wayland_protocols/xdg_shell.h"
    #include "platform/linux/wayland_protocols/pointer_constraints.h"
    #include "platform/linux/wayland_protocols/relative_pointer.h"
    #include <wayland-client.h>
    #include <wayland-cursor.h>
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

    // TODO: Добавить оверлей слои!
    typedef struct platform_window {
        // Соединение с Wayland сервером (НАСЛЕДУЕТСЯ).
        struct wl_display* display;
        // Поверхность окна.
        struct wl_surface* window_surface;
        // Поверхность курсора.
        struct wl_surface* cursor_surface;
        // XDG поверхность для управления.
        struct xdg_surface* xsurface;
        // XDG toplevel для оконных операций.
        struct xdg_toplevel* xtoplevel;
        // Разделяемая память для буферов (НАСЛЕДУЕТСЯ).
        struct wl_shm* shm;
        // Мышка для обработки ввода (НАСЛЕДУЕТСЯ).
        struct wl_pointer* pointer;
        // Менеджер фиксации курсора (НАСЛЕДУЕТСЯ).
        struct zwp_pointer_constraints_v1* pointer_constraints;
        // Фиксатор курсора.
        struct zwp_locked_pointer_v1* pointer_locker;
        // Менеджер относительных значений курсора.
        struct zwp_relative_pointer_manager_v1* pointer_relative_manager;
        // Относительное смещение курсора.
        struct zwp_relative_pointer_v1* pointer_relative;
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
        // Текущий центр курсора по x.
        u32 cursor_hotspot_x;
        // Текущий центр курсора по y.
        u32 cursor_hotspot_y;
        // Последний номер события курсора.
        u32 cursor_last_serial;
        //
        struct wl_surface* cursor_surface_current;
        u32 cursor_hotspot_x_current;
        u32 cursor_hotspot_y_current;
        // Зарегестрированные слушатели событий (по одному на событие).
        platform_window_event_listener_t event_listeners[PLATFORM_WINDOW_EVENT_COUNT];
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
        // Менеджер фиксации курсора.
        struct zwp_pointer_constraints_v1* pointer_constraints;
        // Менеджер относительного движения мыши.
        struct zwp_relative_pointer_manager_v1* pointer_relative_manager;
        // Тема курсора.
        struct wl_cursor_theme* cursor_theme;
        // Конкретные курсоры.
        struct wl_cursor* cursors[1];
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

    static struct wl_buffer* wl_buffer_create(struct wl_shm* shm, u32 width, u32 height, u32 argb_color)
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

    static void xbase_ping(void* data, struct xdg_wm_base *xbase, u32 serial)
    {
        UNUSED(data);

        // NOTE: Ping/pong механизм проверяет доступность всего Wayland соединения.
        xdg_wm_base_pong(xbase, serial);
    }

    static void xsurface_configure(void* data, struct xdg_surface* xsurface, u32 serial)
    {
        platform_window* window = data;

        // Изменения можно делать, только после подтверждения!
        xdg_surface_ack_configure(xsurface, serial);

        // Применение изменений.
        if(window->resize_pending)
        {
            window->resize_pending = false;
            window->width = window->width_pending;
            window->height = window->height_pending;

            // Вызов обработчика окна.
            platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_RESIZE];
            if(listener->callback != nullptr)
            {
                platform_window_event_context_t context = {
                    .type                    = PLATFORM_WINDOW_EVENT_RESIZE,
                    .user_data               = listener->user_data,
                    .window                  = window,
                    .window_resize.state     = window->resize_pending,
                    .window_resize.to_width  = window->width,
                    .window_resize.to_height = window->height
                };

                listener->callback(&context);
            }

            // TODO: Добавить слои если возможно для overflow.
            // NOTE: С изменением частоты кадра, меняется и время перерисовки окна.
            // if(window->background)
            // {
            //     wl_buffer_destroy(window->background);
            //     window->background = wl_buffer_create(
            //         window->shm, window->width_pending, window->height_pending, window->background_color
            //     );

            //     wl_surface_attach(window->surface, window->background, 0, 0);
            //     wl_surface_commit(window->surface);
            //     wl_display_flush(window->display);
            // }

            // FIXME: Плавность изменения, пока цепочка обмена не запущена. Возможно вынести в отдельную функцию.
            wl_surface_commit(window->window_surface);
            wl_display_flush(window->display);
        }
    }

    static void xtop_configure(void* data, struct xdg_toplevel* xtoplevel, i32 width, i32 height, struct wl_array* states)
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

            // Вызов обработчика окна.
            platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_RESIZE];
            if(listener->callback != nullptr)
            {
                platform_window_event_context_t context = {
                    .type                    = PLATFORM_WINDOW_EVENT_RESIZE,
                    .user_data               = listener->user_data,
                    .window                  = window,
                    .window_resize.state     = window->resize_pending,
                    .window_resize.to_width  = window->width_pending,
                    .window_resize.to_height = window->height_pending
                };

                listener->callback(&context);
            }
        }
    }

    static void xtop_close(void* data, struct xdg_toplevel* xtoplevel)
    {
        UNUSED(xtoplevel);

        // TODO: Закрывать только когда фокус на окне!
        platform_window* window = data;

        // Вызов обработчика окна.
        platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_SHOULD_CLOSE];
        if(listener->callback != nullptr)
        {
            platform_window_event_context_t context = {
                .type      = PLATFORM_WINDOW_EVENT_SHOULD_CLOSE,
                .user_data = listener->user_data,
                .window    = window,
            };

            listener->callback(&context);
        }
    }

    static void xtop_configure_bounds(void* data, struct xdg_toplevel* xtop, i32 width, i32 height)
    {
        UNUSED(data); UNUSED(xtop); UNUSED(width); UNUSED(height);
    }

    static void xtop_wm_capabilities(void* data, struct xdg_toplevel* xtop, struct wl_array* capabilities)
    {
        UNUSED(data); UNUSED(xtop); UNUSED(capabilities);
    }

    static void kb_keymap(void* data, struct wl_keyboard* keyboard, u32 format, i32 fd, u32 size)
    {
        UNUSED(keyboard);

        wayland_client* client = data;
        if(format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
        {
            LOG_FATAL("No keymap format.");
            close(fd);
            return;
        }

        char* keymap_str = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
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
    }

    static void kb_enter(void* data, struct wl_keyboard* keyboard, u32 serial, struct wl_surface* surface, struct wl_array* keys)
    {
        UNUSED(keyboard); UNUSED(serial); UNUSED(surface); UNUSED(keys);

        // TODO: Найти окно которому принадлежит surface!

        wayland_client* client = data;

        // Вызов обработчика окна.
        platform_window_event_listener_t* listener = &client->window->event_listeners[PLATFORM_WINDOW_EVENT_KEYBOARD_ENTER];
        if(listener->callback != nullptr)
        {
            platform_window_event_context_t context = {
                .type                 = PLATFORM_WINDOW_EVENT_KEYBOARD_ENTER,
                .user_data            = listener->user_data,
                .window               = client->window,
                .keyboard_focus.state = true
            };

            listener->callback(&context);
        }

        // Вызов обработчика окна.
        listener = &client->window->event_listeners[PLATFORM_WINDOW_EVENT_FOCUS];
        if(listener->callback != nullptr)
        {
            platform_window_event_context_t context = {
                .type                 = PLATFORM_WINDOW_EVENT_FOCUS,
                .user_data            = listener->user_data,
                .window               = client->window,
                .window_focus.state   = true
            };

            listener->callback(&context);
        }
    }

    static void kb_leave(void* data, struct wl_keyboard* keyboard, u32 serial, struct wl_surface* surface)
    {
        UNUSED(keyboard); UNUSED(serial); UNUSED(surface);

        // TODO: Найти окно которому принадлежит surface!
        wayland_client* client = data;

        // Вызов обработчика окна.
        platform_window_event_listener_t* listener = &client->window->event_listeners[PLATFORM_WINDOW_EVENT_KEYBOARD_LEAVE];
        if(listener->callback != nullptr)
        {
            platform_window_event_context_t context = {
                .type                 = PLATFORM_WINDOW_EVENT_KEYBOARD_LEAVE,
                .user_data            = listener->user_data,
                .window               = client->window,
                .keyboard_focus.state = false
            };

            listener->callback(&context);
        }

        // Вызов обработчика окна.
        listener = &client->window->event_listeners[PLATFORM_WINDOW_EVENT_FOCUS];
        if(listener->callback != nullptr)
        {
            platform_window_event_context_t context = {
                .type                 = PLATFORM_WINDOW_EVENT_FOCUS,
                .user_data            = listener->user_data,
                .window               = client->window,
                .window_focus.state   = false
            };

            listener->callback(&context);
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

    static void kb_key(void* data, struct wl_keyboard* keyboard, u32 serial, u32 time, u32 key, u32 state)
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

        // Вызов обработчика окна.
        platform_window_event_listener_t* listener = &client->window->event_listeners[PLATFORM_WINDOW_EVENT_KEYBOARD_KEY];
        if(listener->callback != nullptr)
        {
            platform_window_event_context_t context = {
                .type                 = PLATFORM_WINDOW_EVENT_KEYBOARD_KEY,
                .user_data            = listener->user_data,
                .window               = client->window,
                .keyboard_key.code    = vkey,
                .keyboard_key.state   = (state == WL_KEYBOARD_KEY_STATE_PRESSED)
            };

            listener->callback(&context);
        }
    }

    static void kb_mods(void* data, struct wl_keyboard *kb, u32 serial, u32 depressed, u32 latched, u32 locked, u32 group)
    {
        UNUSED(data); UNUSED(kb); UNUSED(serial); UNUSED(depressed); UNUSED(latched); UNUSED(locked); UNUSED(group);
    }

    static void kb_repeat(void* data, struct wl_keyboard *kb, i32 rate, i32 delay)
    {
        UNUSED(data); UNUSED(kb); UNUSED(rate); UNUSED(delay);
    }

    static void pt_enter(void* data, struct wl_pointer* pointer, u32 serial, struct wl_surface* surface, wl_fixed_t x, wl_fixed_t y)
    {
        UNUSED(pointer); UNUSED(serial); UNUSED(surface); UNUSED(x); UNUSED(y);

        platform_window* window = ((wayland_client*)data)->window;

        // Обновление номера события курсора.
        window->cursor_last_serial = serial;

        // Восстановление состояние курсора.
        wl_pointer_set_cursor(window->pointer, serial, window->cursor_surface_current, window->cursor_hotspot_x_current, window->cursor_hotspot_y_current);

        // Вызов обработчика окна.
        platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_MOUSE_ENTER];
        if(listener->callback != nullptr)
        {
            platform_window_event_context_t context = {
                .type                 = PLATFORM_WINDOW_EVENT_MOUSE_ENTER,
                .user_data            = listener->user_data,
                .window               = window,
                .mouse_focus.state    = true,
            };

            listener->callback(&context);
        }
    }

    static void pt_leave(void* data, struct wl_pointer* pointer, u32 serial, struct wl_surface* surface)
    {
        UNUSED(pointer);
        UNUSED(serial);
        UNUSED(surface);

        wayland_client* client = data;

        // Вызов обработчика окна.
        platform_window_event_listener_t* listener = &client->window->event_listeners[PLATFORM_WINDOW_EVENT_MOUSE_LEAVE];
        if(listener->callback != nullptr)
        {
            platform_window_event_context_t context = {
                .type                 = PLATFORM_WINDOW_EVENT_MOUSE_LEAVE,
                .user_data            = listener->user_data,
                .window               = client->window,
                .mouse_focus.state    = false,
            };

            listener->callback(&context);
        }
    }

    static void pt_motion(void* data, struct wl_pointer* pointer, u32 time, wl_fixed_t x, wl_fixed_t y)
    {
        UNUSED(pointer);
        UNUSED(time);

        // Вызов обработчика окна.
        platform_window* window = ((wayland_client*)data)->window;
        platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_MOUSE_MOVE];
        if(listener->callback != nullptr)
        {
            platform_window_event_context_t context = {
                .type                   = PLATFORM_WINDOW_EVENT_MOUSE_MOVE,
                .user_data              = listener->user_data,
                .window                 = window,
                .mouse_move.to_x        = wl_fixed_to_int(x),
                .mouse_move.to_y        = wl_fixed_to_int(y)
            };

            listener->callback(&context);
        }
    }

    static void pt_motion_relative(
        void* data, struct zwp_relative_pointer_v1* pointer_relative, u32 utime_hi, u32 utime_lo, wl_fixed_t dx, wl_fixed_t dy,
        wl_fixed_t dx_unaccel, wl_fixed_t dy_unaccel
    )
    {
        UNUSED(pointer_relative); UNUSED(utime_hi); UNUSED(utime_lo); UNUSED(dx); UNUSED(dy);

        // Вызов обработчика окна.
        platform_window* window = data;
        platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_MOUSE_MOVE_RELATIVE];
        if(listener->callback != nullptr)
        {
            // NOTE: Отправляются сырые данные мыши, не зависит от системных настроек не имеют ускорения!
            platform_window_event_context_t context = {
                .type                   = PLATFORM_WINDOW_EVENT_MOUSE_MOVE_RELATIVE,
                .user_data              = listener->user_data,
                .window                 = window,
                .mouse_move_relative.dx = wl_fixed_to_int(dx_unaccel),
                .mouse_move_relative.dy = wl_fixed_to_int(dy_unaccel)
            };

            listener->callback(&context);
        }
    }

    static mouse_button translate_button(u32 scancode)
    {
        // В целях оптимизации (коды кнопок 0x110...0x114).
        scancode -= 0x110;

        // Таблица трансляции нормализованных evdev scancode -> virtual button code. См. linux/input-event-codes.h
        static const mouse_button codes[] = {
            [0x00] = BUTTON_LEFT, [0x01] = BUTTON_RIGHT, [0x02] = BUTTON_MIDDLE, [0x03] = BUTTON_BACKWARD, [0x04] = BUTTON_FORWARD
        };

        if(scancode >= ARRAY_SIZE(codes))
        {
            return BUTTON_UNKNOWN;
        }

        return codes[scancode];
    }

    static void pt_button(void* data, struct wl_pointer* pointer, u32 serial, u32 time, u32 button, u32 state)
    {
        UNUSED(pointer);
        UNUSED(serial);
        UNUSED(time);

        wayland_client* client = data;
        mouse_button vbutton = translate_button(button);

        if(vbutton == BUTTON_UNKNOWN)
        {
            LOG_WARN("Mouse button event: unknown scancode=0x%x (%d), state=%d.", button, button, state);
            return;
        }

        // Вызов обработчика окна.
        platform_window_event_listener_t* listener = &client->window->event_listeners[PLATFORM_WINDOW_EVENT_MOUSE_BUTTON];
        if(listener->callback != nullptr)
        {
            platform_window_event_context_t context = {
                .type                 = PLATFORM_WINDOW_EVENT_MOUSE_BUTTON,
                .user_data            = listener->user_data,
                .window               = client->window,
                .mouse_button.code    = vbutton,
                .mouse_button.state   = (state == WL_POINTER_BUTTON_STATE_PRESSED)
            };

            listener->callback(&context);
        }
    }

    static void pt_wheel(void* data, struct wl_pointer* pointer, u32 axis, i32 discrete)
    {
        UNUSED(pointer);
        i32 delta_vert = 0;
        i32 delta_horz = 0;

        if(axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
        {
            delta_vert = -discrete;
        }
        else if(axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
        {
            delta_horz = discrete;
        }
        else
        {
            LOG_WARN("Mouse wheel event: unsupported axis - %u.", axis);
            return;
        }

        // Вызов обработчика окна.
        platform_window* window = ((wayland_client*)data)->window;
        platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_MOUSE_WHEEL];
        if(listener->callback != nullptr)
        {
            platform_window_event_context_t context = {
                .type                   = PLATFORM_WINDOW_EVENT_MOUSE_WHEEL,
                .user_data              = listener->user_data,
                .window                 = window,
                .mouse_wheel.delta_vert = delta_vert,
                .mouse_wheel.delta_horz = delta_horz
            };

            listener->callback(&context);
        }
    }

    static void pt_axis(void* data, struct wl_pointer *pt, u32 time, u32 axis, wl_fixed_t val)
    {
        UNUSED(data); UNUSED(pt); UNUSED(time); UNUSED(axis); UNUSED(val);
    }

    static void pt_frame(void* data, struct wl_pointer *pt)
    {
        UNUSED(data); UNUSED(pt);
    }

    static void pt_axis_src(void* data, struct wl_pointer *pt, u32 axis_src)
    {
        UNUSED(data); UNUSED(pt); UNUSED(axis_src);
    }

    static void pt_axis_stop(void* data, struct wl_pointer *pt, u32 time, u32 axis)
    {
        UNUSED(data); UNUSED(pt); UNUSED(time); UNUSED(axis);
    }

    static void pt_axis_val120(void* data, struct wl_pointer *pt, u32 axis, i32 val120)
    {
        UNUSED(data); UNUSED(pt); UNUSED(axis); UNUSED(val120);
    }

    static void pt_axis_relative(void* data, struct wl_pointer *pt, u32 axis, u32 direction)
    {
        UNUSED(data); UNUSED(pt); UNUSED(axis); UNUSED(direction);
    }

    static void seat_capabilities(void* data, struct wl_seat* seat, u32 capabilities)
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
                static const struct wl_keyboard_listener kb_listeners = { kb_keymap, kb_enter, kb_leave, kb_key, kb_mods, kb_repeat };
                client->keyboard = wl_seat_get_keyboard(seat);
                wl_keyboard_add_listener(client->keyboard, &kb_listeners, client);
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
                static const struct wl_pointer_listener pt_listeners = {
                    pt_enter, pt_leave, pt_motion, pt_button, pt_axis, pt_frame, pt_axis_src,
                    pt_axis_stop, pt_wheel, pt_axis_val120, pt_axis_relative
                };
                client->pointer = wl_seat_get_pointer(seat);
                wl_pointer_add_listener(client->pointer, &pt_listeners, client);
                LOG_TRACE("Seat pointer is found.");
            }
        }
    }

    static void seat_name(void* data, struct wl_seat* seat, const char* name)
    {
        UNUSED(data); UNUSED(seat); UNUSED(name);
    }

    static void registry_add(void* data, struct wl_registry* registry, u32 name, const char* interface, u32 version)
    {
        static const u32 compositor_ver = 4;
        static const u32 xdg_shell_ver  = 3;
        static const u32 seat_ver       = 7;
        static const u32 shm_ver        = 1;
        static const u32 pt_constraints = 1;
        static const u32 pt_relative    = 1;
        wayland_client* client = data;

        LOG_TRACE("[%02u] %s (version %u).", name, interface, version);

        if(string_equal(interface, wl_compositor_interface.name))
        {
            client->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, MIN(version, compositor_ver));
            LOG_TRACE("  -> Bound compositor v%d.", MIN(version, compositor_ver));
        }
        else if(string_equal(interface, xdg_wm_base_interface.name))
        {
            client->xbase = wl_registry_bind(registry, name, &xdg_wm_base_interface, MIN(version, xdg_shell_ver));
            LOG_TRACE("  -> Bound xdg shell v%d.", MIN(version, xdg_shell_ver));
        }
        else if(string_equal(interface, wl_seat_interface.name))
        {
            client->seat = wl_registry_bind(registry, name, &wl_seat_interface, MIN(version, seat_ver));
            LOG_TRACE("  -> Bound seat v%d.", MIN(version, seat_ver));
        }
        else if(string_equal(interface, wl_shm_interface.name))
        {
            client->shm = wl_registry_bind(registry, name, &wl_shm_interface, MIN(version, shm_ver));
            LOG_TRACE("  -> Bound shared memory v%d", MIN(version, shm_ver));
        }
        else if(string_equal(interface, zwp_pointer_constraints_v1_interface.name))
        {
            client->pointer_constraints = wl_registry_bind(registry, name, &zwp_pointer_constraints_v1_interface, MIN(version, pt_constraints));
            LOG_TRACE("  -> Bound pointer constraints v%d.", MIN(version, pt_constraints));
        }
        else if(string_equal(interface, zwp_relative_pointer_manager_v1_interface.name))
        {
            client->pointer_relative_manager = wl_registry_bind(registry, name, &zwp_relative_pointer_manager_v1_interface, MIN(version, pt_relative));
            LOG_TRACE("  -> Bound relative pointer manager v%d.", MIN(version, pt_relative));
        }

        // TODO: Добавьте другие интерфейсы по необходимости:
        //        - wl_output_interface: для multi-monitor поддержки.
        //        - zwp_linux_dmabuf_v1_interface: для Vulkan DMA-BUF.
        //        - wl_drm_interface: для OpenGL EGL.
    }

    static void registry_remove(void* data, struct wl_registry* registry, u32 name)
    {
        UNUSED(name);
        UNUSED(data);
        UNUSED(registry);

        LOG_WARN("Wayland has not yet supported remove this event (id %02u).", name);
    }

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

        // Получение файлового дескриптора для polling.
        client->displayfd = wl_display_get_fd(client->display);

        // Настройка poll структуры для обработки событий.
        client->poll_fd[0].fd = client->displayfd;
        // TODO: POLLERR - Указывает на возникновение ошибки на файловом дескрипторе.
        //       POLLHUP - Указывает, что соединение "повисло".
        client->poll_fd[0].events = POLLIN; //| POLLERR | POLLHUP;
        client->poll_fd_count = ARRAY_SIZE(client->poll_fd);

        // Получение registry для обнаружения глобальных объектов.
        client->registry = wl_display_get_registry(client->display);
        if(!client->registry)
        {
            LOG_ERROR("Failed to get Wayland registry.");
            wayland_backend_shutdown(client);
            return false;
        }
        static const struct wl_registry_listener registry_listeners = { registry_add, registry_remove };
        wl_registry_add_listener(client->registry, &registry_listeners, client);

        // Синхронный обмен с сервером для получения глобальных объектов.
        LOG_TRACE("Retrieving global objects:");
        wl_display_roundtrip(client->display);

        // Проверка обязательных глобальных объектов.
        if(!client->compositor)
        {
            LOG_ERROR("Interface 'compositor' not available.");
            wayland_backend_shutdown(client);
            return false;
        }

        if(!client->xbase)
        {
            LOG_ERROR("Interface 'xdg shell' not available.");
            wayland_backend_shutdown(client);
            return false;
        }

        if(!client->shm)
        {
            LOG_ERROR("Interface 'shared memory' not available.");
            wayland_backend_shutdown(client);
            return false;
        }

        if(!client->seat)
        {
            LOG_ERROR("Interface 'seat' not available.");
            wayland_backend_shutdown(client);
            return false;
        }

        if(!client->pointer_constraints)
        {
            LOG_ERROR("Interface 'pointer constraints' not available.");
            wayland_backend_shutdown(client);
            return false;
        }

        if(!client->pointer_relative_manager)
        {
            LOG_ERROR("Interface 'pointer relative manager' not available.");
            wayland_backend_shutdown(client);
            return false;
        }

        static const struct xdg_wm_base_listener xbase_listeners = { xbase_ping };
        xdg_wm_base_add_listener(client->xbase, &xbase_listeners, client);

        static const struct wl_seat_listener seat_listeners = { seat_capabilities, seat_name };
        wl_seat_add_listener(client->seat, &seat_listeners, client);

        // Дополнительный синхронизация для завершения инициализации.
        wl_display_roundtrip(client->display);

        // Загрузка темы курсора.
        client->cursor_theme = wl_cursor_theme_load(nullptr, 24, client->shm);

        // Получение ссылок на курсоры внутри темы.
        // TODO: enum список указателей.
        client->cursors[0] = wl_cursor_theme_get_cursor(client->cursor_theme, "left_ptr");

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

        // TODO: Уничтожение всех окон (пока только одно окно).
        if(client->window)
        {
            LOG_WARN("Window was not properly destroyed before shutdown.");
            wayland_window_destroy(client->window, client);
        }

        if(client->pointer_relative_manager) zwp_relative_pointer_manager_v1_destroy(client->pointer_relative_manager);
        if(client->pointer_constraints) zwp_pointer_constraints_v1_destroy(client->pointer_constraints);
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

    platform_window* wayland_window_create(const platform_window_config_t* config, void* internal_data)
    {
        wayland_client* client = internal_data;

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
        *window = (struct platform_window){
            .display                  = client->display,
            .shm                      = client->shm,
            .pointer                  = client->pointer,
            .pointer_constraints      = client->pointer_constraints,
            .pointer_relative_manager = client->pointer_relative_manager,
            .title                    = string_duplicate(config->title),
            .background_color         = 0xFF000000
        };

        // Инициализация поверхности окна и курсора.
        window->window_surface = wl_compositor_create_surface(client->compositor);
        window->cursor_surface = wl_compositor_create_surface(client->compositor);

        // Инициализация поверхности оболочки.
        window->xsurface = xdg_wm_base_get_xdg_surface(client->xbase, window->window_surface);
        static const struct xdg_surface_listener xsurf_listeners = { xsurface_configure };
        xdg_surface_add_listener(window->xsurface, &xsurf_listeners, window);

        window->xtoplevel = xdg_surface_get_toplevel(window->xsurface);
        static const struct xdg_toplevel_listener xtop_listeners = { xtop_configure, xtop_close, xtop_configure_bounds, xtop_wm_capabilities };
        xdg_toplevel_add_listener(window->xtoplevel, &xtop_listeners, window);

        xdg_toplevel_set_title(window->xtoplevel, window->title);
        xdg_toplevel_set_app_id(window->xtoplevel, window->title);

        // NOTE: Для полноэкранного режима по умолчанию, раскомментируете ниже.
        // xdg_toplevel_set_fullscreen(window->xtoplevel, nullptr);
        // xdg_toplevel_set_maximized();

        // Настройка поверхности.
        wl_surface_commit(window->window_surface);
        wl_display_roundtrip(client->display);

        window->pointer_relative = zwp_relative_pointer_manager_v1_get_relative_pointer(window->pointer_relative_manager, window->pointer);
        static const struct zwp_relative_pointer_v1_listener pt_listeners = { pt_motion_relative };
        zwp_relative_pointer_v1_add_listener(window->pointer_relative, &pt_listeners, window);

        // Создание буфера кадра для отображения окна до рендера.
        window->background = wl_buffer_create(window->shm, config->width, config->height, window->background_color);
        if(!window->background)
        {
            LOG_ERROR("Failed to create background buffer.");
            wayland_window_destroy(window, client);
            return nullptr;
        }

        // Подтверждение изменений и обработка событий и захват кадра.
        wl_surface_attach(window->window_surface, window->background, 0, 0);
        wl_surface_commit(window->window_surface);
        wl_display_flush(client->display);

        // Задание курсора по умолчанию.
        // TODO: Сделать настраиваемым.
        struct wl_cursor_image* img = client->cursors[0]->images[0]; 
        struct wl_buffer* buffer = wl_cursor_image_get_buffer(img);

        wl_surface_attach(window->cursor_surface, buffer, 0, 0);
        wl_surface_damage(window->cursor_surface, 0, 0, img->width, img->height);
        wl_surface_commit(window->cursor_surface);

        window->cursor_hotspot_x = img->hotspot_x;
        window->cursor_hotspot_y = img->hotspot_y;
        

        LOG_TRACE("Window '%s' created successfully.", window->title);

        client->window = window;
        return window;
    }

    void wayland_window_destroy(platform_window* window, void* internal_data)
    {
        wayland_client* client = internal_data;

        // Вызов обработчика окна.
        platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_DESTROY];
        if(listener->callback != nullptr)
        {
            platform_window_event_context_t context = {
                .type      = PLATFORM_WINDOW_EVENT_DESTROY,
                .user_data = listener->user_data,
                .window    = window,
            };

            listener->callback(&context);
        }

        // TODO: Удаление окна из списка окон.

        if(window->pointer_relative) zwp_relative_pointer_v1_destroy(window->pointer_relative);
        if(window->pointer_locker) zwp_locked_pointer_v1_destroy(window->pointer_locker);
        if(window->xtoplevel)  xdg_toplevel_destroy(window->xtoplevel);
        if(window->xsurface)   xdg_surface_destroy(window->xsurface);
        if(window->window_surface)    wl_surface_destroy(window->window_surface);
        if(window->background) wl_buffer_destroy(window->background);
        if(window->title)      string_free(window->title);

        mfree(window, sizeof(platform_window), MEMORY_TAG_APPLICATION);
        client->window = nullptr;
    }

    const char* wayland_window_get_title(platform_window* window)
    {
        return window->title;
    }

    void wayland_window_get_resolution(platform_window* window, u32* width, u32* height)
    {
        *width = window->width;
        *height = window->height;
    }

    void wayland_window_set_event_callback(platform_window* window, platform_window_event_t event, platform_window_event_callback callback, void* user_data)
    {
        window->event_listeners[event] = (platform_window_event_listener_t){
            .callback  = callback,
            .user_data = user_data
        };
    }

    void wayland_window_hide_cursor(platform_window_t* window)
    {
        window->cursor_surface_current = nullptr;
        window->cursor_hotspot_x_current = 0;
        window->cursor_hotspot_y_current = 0;
        wl_pointer_set_cursor(window->pointer, window->cursor_last_serial, nullptr, 0, 0);
    }

    void wayland_window_show_cursor(platform_window_t* window)
    {
        window->cursor_surface_current = window->cursor_surface;
        window->cursor_hotspot_x_current = window->cursor_hotspot_x;
        window->cursor_hotspot_y_current = window->cursor_hotspot_y;
        wl_pointer_set_cursor(window->pointer, window->cursor_last_serial, window->cursor_surface, window->cursor_hotspot_x, window->cursor_hotspot_y);
    }

    void wayland_window_lock_cursor(platform_window_t* window)
    {
        if(window->pointer_locker != nullptr) return;

        // NOTE: Однажды вызванная не требует повторной блокировки.
        window->pointer_locker = zwp_pointer_constraints_v1_lock_pointer(
            window->pointer_constraints, window->window_surface, window->pointer, nullptr, ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT
        );

        // TODO: Нужно ли устанавливать курсор в центр.
        // zwp_locked_pointer_v1_set_cursor_position_hint
    }

    void wayland_window_unlock_cursor(platform_window_t* window)
    {
        if(window->pointer_locker == nullptr) return;

        zwp_locked_pointer_v1_destroy(window->pointer_locker);
        window->pointer_locker = nullptr;
    }

    void wayland_enumerate_vulkan_extensions(u32* extension_count, const char** out_extensions)
    {
        static const char* extensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
        };

        if(out_extensions == nullptr)
        {
            *extension_count = ARRAY_SIZE(extensions);
            return;
        }

        mcopy(out_extensions, extensions, sizeof(char*) * (*extension_count));
    }

    u32 wayland_window_create_vulkan_surface(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void** out_vulkan_surface)
    {
        VkWaylandSurfaceCreateInfoKHR surface_create_info = {
            .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
            .display = window->display,
            .surface = window->window_surface
        };

        return vkCreateWaylandSurfaceKHR(vulkan_instance, &surface_create_info, vulkan_allocator, (VkSurfaceKHR*)out_vulkan_surface);
    }

    void wayland_window_destroy_vulkan_surface(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void* vulkan_surface)
    {
        UNUSED(window);
        vkDestroySurfaceKHR(vulkan_instance, vulkan_surface, vulkan_allocator);
    }

    u32 wayland_window_supports_vulkan_presentation(platform_window* window, void* vulkan_pyhical_device, u32 queue_family_index)
    {
        return vkGetPhysicalDeviceWaylandPresentationSupportKHR(vulkan_pyhical_device, queue_family_index, window->display);
    }

#endif
