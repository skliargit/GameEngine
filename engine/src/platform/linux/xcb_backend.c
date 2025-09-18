#include "platform/linux/xcb_backend.h"

#ifdef PLATFORM_LINUX_FLAG

    #include "core/logger.h"
    #include "core/string.h"
    #include "core/memory.h"
    #include <xcb/xcb.h>
    #include <vulkan/vulkan.h>
    #include <vulkan/vulkan_xcb.h>
    #include <xkbcommon/xkbcommon.h>
    #include <xkbcommon/xkbcommon-x11.h>
    #include <stdlib.h>

    // Для включения отладки оконной системы индивидуально.
    #ifndef DEBUG_PLATFORM_FLAG
        #undef LOG_DEBUG
        #undef LOG_TRACE
        #define LOG_DEBUG(...) UNUSED(0)
        #define LOG_TRACE(...) UNUSED(0)
    #endif

    typedef struct platform_window {
        // Соединение с X сервером (НАСЛЕДУЕТСЯ).
        xcb_connection_t* connection;
        // Идентификатор окна XCB.
        xcb_window_t window;
        // Визуал (НАСЛЕДУЕТСЯ).
        xcb_visualid_t visual;
        // Атом для обработки закрытия окна.
        xcb_atom_t wm_delete;
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

    typedef struct xcb_client {
        // Соединение с X сервером.
        xcb_connection_t* connection;
        // Информация о экране.
        xcb_screen_t* screen;
        // Визуал.
        xcb_visualid_t visual;
        // Контекст библиотеки XKBcommon для обработки раскладки клавиатуры.
        struct xkb_context* xkbcontext;
        // Карта клавиш XKB (раскладка, символы, модификаторы, группы клавиш).
        struct xkb_keymap* xkbkeymap;
        // Состояние XKB (текущая раскладка, модификаторы, заблокированные клавиши).
        struct xkb_state* xkbstate;
        // ID основной клавиатуры.
        i32 xkbdevice_id;
        // TODO: Окно с текущим фокусом ввода.
        platform_window* focused_window;
        // TODO: Динамический массив всех окон (пока только одно окно).
        platform_window* window;
    } xcb_client;

    static xcb_screen_t* get_screen(xcb_connection_t *connection, int screen_num)
    {
        xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(connection));
        for(int i = 0; i < screen_num; i++)
        {
            xcb_screen_next(&iter);
        }
        return iter.data;
    }

    bool xcb_backend_initialize(void* internal_data)
    {
        xcb_client* client = internal_data;

        // Установка соединение с X сервером.
        client->connection = xcb_connect(nullptr, nullptr);
        if(xcb_connection_has_error(client->connection))
        {
            LOG_ERROR("Failed to connect to X server.");
            return false;
        }
        LOG_TRACE("Connected to X server.");

        // Получение первого экрана.
        client->screen = get_screen(client->connection, 0);
        if(!client->screen)
        {
            LOG_ERROR("Failed to get screen from X server.");
            xcb_backend_shutdown(client);
            return false;
        }

        // Получение визуала.
        xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(client->screen);
        xcb_visualid_t visual_id = XCB_NONE;

        while(depth_iter.rem)
        {
            xcb_depth_t* depth = depth_iter.data;
            xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth);

            while(visual_iter.rem)
            {
                xcb_visualtype_t *visual = visual_iter.data;

                if(visual->_class == XCB_VISUAL_CLASS_TRUE_COLOR)
                {
                    // NOTE: Здесь прерывается т.к. это предпочитаемый формат.
                    visual_id = visual->visual_id;
                    break;
                }
                else if(visual->_class == XCB_VISUAL_CLASS_DIRECT_COLOR)
                {
                    // Возможный формат, если не будет TRUE COLOR.
                    visual_id = visual->visual_id;
                }

                xcb_visualtype_next(&visual_iter);
            }

            if(visual_id != XCB_NONE)
            {
                break;
            }

            xcb_depth_next(&depth_iter);
        }

        if(visual_id == XCB_NONE)
        {
            LOG_ERROR("No suitable visual was found.");
            xcb_backend_shutdown(client);
            return false;
        }
        LOG_TRACE("Xcb visual found (id: %u).", visual_id);

        // Создание контекста XKB.
        client->xkbcontext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        if(!client->xkbcontext)
        {
            LOG_ERROR("Failed to create XKB context.");
            xcb_backend_shutdown(client);
            return false;
        }

        // Настройка расширения XKB.
        if(!xkb_x11_setup_xkb_extension(
            client->connection, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
            XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, nullptr, nullptr, nullptr, nullptr
        ))
        {
            LOG_ERROR("Failed to setup XKB extension.");
            xcb_backend_shutdown(client);
            return false;
        }

        // Получения ID ядра клавиатуры.
        client->xkbdevice_id = xkb_x11_get_core_keyboard_device_id(client->connection);
        if(client->xkbdevice_id == -1)
        {
            LOG_ERROR("Failed to get core keyboard device id.");
            xcb_backend_shutdown(client);
            return false;
        }

        client->xkbkeymap = xkb_x11_keymap_new_from_device(
            client->xkbcontext, client->connection, client->xkbdevice_id, XKB_KEYMAP_COMPILE_NO_FLAGS
        );
        if(!client->xkbkeymap)
        {
            LOG_ERROR("Failed to obtain XKB keymap.");
            xcb_backend_shutdown(client);
            return false;
        }

        client->xkbstate = xkb_x11_state_new_from_device(client->xkbkeymap, client->connection, client->xkbdevice_id);
        if(!client->xkbstate)
        {
            LOG_ERROR("Failed to create XKB state.");
            xcb_backend_shutdown(client);
            return false;
        }

        LOG_TRACE("XCB backend initialized successfully.");
        client->window = nullptr;
        return true;
    }

    void xcb_backend_shutdown(void* internal_data)
    {
        xcb_client* client = internal_data;

        LOG_TRACE("Disconnecting from X server...");

        // Уничтожение всех окон (пока только одно окно).
        if(client->window)
        {
            LOG_WARN("Window was not properly destroyed before shutdown.");
            xcb_backend_window_destroy(client->window, client);
            // NOTE: Осовобождение client->window в xcb_backend_window_destroy().
        }

        if(client->xkbstate)   xkb_state_unref(client->xkbstate);
        if(client->xkbkeymap)  xkb_keymap_unref(client->xkbkeymap);
        if(client->xkbcontext) xkb_context_unref(client->xkbcontext);
        if(client->connection) xcb_disconnect(client->connection);

        mzero(client, sizeof(xcb_client));
        LOG_TRACE("XCB backend shutdown complete.");
    }

    static keyboard_key translate_key(xkb_keycode_t keysym)
    {
        // Таблица трансляции нижних XKB keysym (0x00XX) -> virtual keycode.
        static const keyboard_key lcodes[KEY_COUNT] = {
            [XKB_KEY_space] = KEY_SPACE,            [XKB_KEY_n           ] = KEY_N,
            [XKB_KEY_0    ] = KEY_0,                [XKB_KEY_o           ] = KEY_O,
            [XKB_KEY_1    ] = KEY_1,                [XKB_KEY_p           ] = KEY_P,
            [XKB_KEY_2    ] = KEY_2,                [XKB_KEY_q           ] = KEY_Q,
            [XKB_KEY_3    ] = KEY_3,                [XKB_KEY_r           ] = KEY_R,
            [XKB_KEY_4    ] = KEY_4,                [XKB_KEY_s           ] = KEY_S,
            [XKB_KEY_5    ] = KEY_5,                [XKB_KEY_t           ] = KEY_T,
            [XKB_KEY_6    ] = KEY_6,                [XKB_KEY_u           ] = KEY_U,
            [XKB_KEY_7    ] = KEY_7,                [XKB_KEY_v           ] = KEY_V,
            [XKB_KEY_8    ] = KEY_8,                [XKB_KEY_w           ] = KEY_W,
            [XKB_KEY_9    ] = KEY_9,                [XKB_KEY_x           ] = KEY_X,
            [XKB_KEY_a    ] = KEY_A,                [XKB_KEY_y           ] = KEY_Y,
            [XKB_KEY_b    ] = KEY_B,                [XKB_KEY_z           ] = KEY_Z,
            [XKB_KEY_c    ] = KEY_C,                [XKB_KEY_semicolon   ] = KEY_SEMICOLON,
            [XKB_KEY_d    ] = KEY_D,                [XKB_KEY_apostrophe  ] = KEY_QUOTE,
            [XKB_KEY_e    ] = KEY_E,                [XKB_KEY_equal       ] = KEY_EQUAL,
            [XKB_KEY_f    ] = KEY_F,                [XKB_KEY_comma       ] = KEY_COMMA,
            [XKB_KEY_g    ] = KEY_G,                [XKB_KEY_minus       ] = KEY_MINUS,
            [XKB_KEY_h    ] = KEY_H,                [XKB_KEY_period      ] = KEY_DOT,
            [XKB_KEY_i    ] = KEY_I,                [XKB_KEY_slash       ] = KEY_SLASH,
            [XKB_KEY_j    ] = KEY_J,                [XKB_KEY_grave       ] = KEY_GRAVE,
            [XKB_KEY_k    ] = KEY_K,                [XKB_KEY_bracketleft ] = KEY_LBRACKET,
            [XKB_KEY_l    ] = KEY_L,                [XKB_KEY_backslash   ] = KEY_BACKSLASH,
            [XKB_KEY_m    ] = KEY_M,                [XKB_KEY_bracketright] = KEY_RBRACKET,
        };

        // Таблица трансляции верхних XKB keysym (0xFFXX) -> virtual keycode.
        static const keyboard_key hcodes[KEY_COUNT] = {
            [XKB_KEY_BackSpace    & 0x00ff] = KEY_BACKSPACE,    [XKB_KEY_Tab          & 0x00ff] = KEY_TAB,
            [XKB_KEY_KP_Subtract  & 0x00ff] = KEY_SUBTRACT,     [XKB_KEY_Return       & 0x00ff] = KEY_RETURN,
            [XKB_KEY_KP_Decimal   & 0x00ff] = KEY_DECIMAL,      [XKB_KEY_Pause        & 0x00ff] = KEY_PAUSE,
            [XKB_KEY_KP_Divide    & 0x00ff] = KEY_DIVIDE,       [XKB_KEY_Caps_Lock    & 0x00ff] = KEY_CAPSLOCK,
            [XKB_KEY_KP_Enter     & 0x00ff] = KEY_RETURN,       [XKB_KEY_Escape       & 0x00ff] = KEY_ESCAPE,
            [XKB_KEY_F1           & 0x00ff] = KEY_F1,           [XKB_KEY_Page_Up      & 0x00ff] = KEY_PAGEUP,
            [XKB_KEY_F2           & 0x00ff] = KEY_F2,           [XKB_KEY_Page_Down    & 0x00ff] = KEY_PAGEDOWN,
            [XKB_KEY_F3           & 0x00ff] = KEY_F3,           [XKB_KEY_End          & 0x00ff] = KEY_END,
            [XKB_KEY_F4           & 0x00ff] = KEY_F4,           [XKB_KEY_Home         & 0x00ff] = KEY_HOME,
            [XKB_KEY_F5           & 0x00ff] = KEY_F5,           [XKB_KEY_Left         & 0x00ff] = KEY_LEFT,
            [XKB_KEY_F6           & 0x00ff] = KEY_F6,           [XKB_KEY_Up           & 0x00ff] = KEY_UP,
            [XKB_KEY_F7           & 0x00ff] = KEY_F7,           [XKB_KEY_Right        & 0x00ff] = KEY_RIGHT,
            [XKB_KEY_F8           & 0x00ff] = KEY_F8,           [XKB_KEY_Down         & 0x00ff] = KEY_DOWN,
            [XKB_KEY_F9           & 0x00ff] = KEY_F9,           [XKB_KEY_Print        & 0x00ff] = KEY_PRINTSCREEN,
            [XKB_KEY_F10          & 0x00ff] = KEY_F10,          [XKB_KEY_Insert       & 0x00ff] = KEY_INSERT,
            [XKB_KEY_F11          & 0x00ff] = KEY_F11,          [XKB_KEY_Delete       & 0x00ff] = KEY_DELETE,
            [XKB_KEY_F12          & 0x00ff] = KEY_F12,          [XKB_KEY_Menu         & 0x00ff] = KEY_MENU,
            [XKB_KEY_F13          & 0x00ff] = KEY_F13,          [XKB_KEY_Super_L      & 0x00ff] = KEY_LSUPER,
            [XKB_KEY_F14          & 0x00ff] = KEY_F14,          [XKB_KEY_Super_R      & 0x00ff] = KEY_RSUPER,
            [XKB_KEY_F15          & 0x00ff] = KEY_F15,          [XKB_KEY_KP_0         & 0x00ff] = KEY_NUMPAD0,
            [XKB_KEY_F16          & 0x00ff] = KEY_F16,          [XKB_KEY_KP_1         & 0x00ff] = KEY_NUMPAD1,
            [XKB_KEY_F17          & 0x00ff] = KEY_F17,          [XKB_KEY_KP_2         & 0x00ff] = KEY_NUMPAD2,
            [XKB_KEY_F18          & 0x00ff] = KEY_F18,          [XKB_KEY_KP_3         & 0x00ff] = KEY_NUMPAD3,
            [XKB_KEY_F19          & 0x00ff] = KEY_F19,          [XKB_KEY_KP_4         & 0x00ff] = KEY_NUMPAD4,
            [XKB_KEY_F20          & 0x00ff] = KEY_F20,          [XKB_KEY_KP_5         & 0x00ff] = KEY_NUMPAD5,
            [XKB_KEY_F21          & 0x00ff] = KEY_F21,          [XKB_KEY_KP_6         & 0x00ff] = KEY_NUMPAD6,
            [XKB_KEY_F22          & 0x00ff] = KEY_F22,          [XKB_KEY_KP_7         & 0x00ff] = KEY_NUMPAD7,
            [XKB_KEY_F23          & 0x00ff] = KEY_F23,          [XKB_KEY_KP_8         & 0x00ff] = KEY_NUMPAD8,
            [XKB_KEY_F24          & 0x00ff] = KEY_F24,          [XKB_KEY_KP_9         & 0x00ff] = KEY_NUMPAD9,
            [XKB_KEY_Num_Lock     & 0x00ff] = KEY_NUMLOCK,      [XKB_KEY_KP_Multiply  & 0x00ff] = KEY_MULTIPLY,
            [XKB_KEY_Scroll_Lock  & 0x00ff] = KEY_SCROLLLOCK,   [XKB_KEY_KP_Add       & 0x00ff] = KEY_ADD,
            [XKB_KEY_Shift_L      & 0x00ff] = KEY_LSHIFT,       [XKB_KEY_Alt_R        & 0x00ff] = KEY_RALT,
            [XKB_KEY_Shift_R      & 0x00ff] = KEY_RSHIFT,       [XKB_KEY_Alt_L        & 0x00ff] = KEY_LALT,
            [XKB_KEY_Control_L    & 0x00ff] = KEY_LCONTROL,     [XKB_KEY_Control_R    & 0x00ff] = KEY_RCONTROL,
            [0xff9e               & 0x00ff] = KEY_NUMPAD0,      [0xff9c               & 0x00ff] = KEY_NUMPAD1,
            [0xff99               & 0x00ff] = KEY_NUMPAD2,      [0xff9b               & 0x00ff] = KEY_NUMPAD3,
            [0xff96               & 0x00ff] = KEY_NUMPAD4,      [0xff9d               & 0x00ff] = KEY_NUMPAD5,
            [0xff98               & 0x00ff] = KEY_NUMPAD6,      [0xff95               & 0x00ff] = KEY_NUMPAD7,
            [0xff97               & 0x00ff] = KEY_NUMPAD8,      [0xff9a               & 0x00ff] = KEY_NUMPAD9,
            [0xff9f               & 0x00ff] = KEY_DECIMAL,
        };

        // Диапазон 0x0000-0x00FF.
        if(keysym < U8_MAX)
        {
            return lcodes[keysym];
        }

        // Диапазон 0xFF00-0xFFFF.
        if(keysym <= U16_MAX && keysym >= (U16_MAX - U8_MAX))
        {
            return hcodes[keysym & U8_MAX];
        }

        // Специальная клавиша.
        if(keysym == XKB_KEY_XF86Sleep)
        {
            return KEY_SLEEP;
        }

        return KEY_UNKNOWN;
    }

    static mouse_button translate_button(xcb_button_t scancode)
    {
        // Таблица трансляции xcb scancode -> virtual button code.
        static const mouse_button codes[] = {
            [0x01] = BTN_LEFT, [0x02] = BTN_MIDDLE, [0x03] = BTN_RIGHT, [0x08] = BTN_BACKWARD, [0x09] = BTN_FORWARD
        };

        if(scancode >= (sizeof(codes)/sizeof(mouse_button)))
        {
            return BTN_UNKNOWN;
        }

        return codes[scancode];
    }

    bool xcb_backend_poll_events(void* internal_data)
    {
        xcb_client* client = internal_data;
        xcb_generic_event_t* event;

        while((event = xcb_poll_for_event(client->connection)))
        {
            u8 response_type = event->response_type & ~0x80;

            // Выбор обрабатываемого события.
            switch(response_type)
            {
                // Обработка клавиш клавиатуры.
                case XCB_KEY_PRESS:
                case XCB_KEY_RELEASE:
                {
                    xcb_key_press_event_t* keyboard = (xcb_key_press_event_t*)event;
                    xkb_keysym_t keysym = xkb_state_key_get_one_sym(client->xkbstate, keyboard->detail);
                    bool state = keyboard->response_type == XCB_KEY_PRESS;
                    keyboard_key vkey = translate_key(keysym);

                    if(vkey == KEY_UNKNOWN)
                    {
                        LOG_WARN("Keyboard key event: unknown keysym=0x%x (%d), state=%d.", keysym, keysym, state);
                        break;
                    }

                    LOG_TRACE("Keyboard key event: keysym=0x%x (%d), state=%d, window='%s'.", keysym, keysym, state, client->window->title);
                    if(client->window->on_key)
                    {
                        client->window->on_key(vkey, state);
                    }
                } break;

                // Обработка изменения раскладки клавиатуры.
                case XCB_MAPPING_NOTIFY:
                {
                } break;

                // Обработка кнопок мыши.
                case XCB_BUTTON_PRESS:
                {
                    xcb_button_press_event_t* mouse = (xcb_button_press_event_t*)event;

                    // Проверка на вертикальную и горизонтальную прокрутку.
                    if(client->window->on_mouse_wheel)
                    {
                        if(mouse->detail == 4)
                        {
                            client->window->on_mouse_wheel(1, 0);
                            break;
                        }
                        else if(mouse->detail == 5)
                        {
                            client->window->on_mouse_wheel(-1, 0);
                            break;
                        }
                        else if(mouse->detail == 6)
                        {
                            client->window->on_mouse_wheel(0, -1);
                            break;
                        }
                        else if(mouse->detail == 7)
                        {
                            client->window->on_mouse_wheel(0, 1);
                            break;
                        }
                    }
                }
                case XCB_BUTTON_RELEASE:
                {
                    xcb_button_press_event_t* mouse = (xcb_button_press_event_t*)event;
                    bool state = mouse->response_type == XCB_BUTTON_PRESS;

                    // Проверка кнопок мыши.
                    mouse_button vbutton = translate_button(mouse->detail);
                    if(vbutton == BTN_UNKNOWN)
                    {
                        LOG_TRACE("Mouse button event: unknown scancode=0x%x (%hhu), state=%d.",
                            mouse->detail, mouse->detail, state
                        );
                        break;
                    }

                    LOG_TRACE("Mouse button event: scancode=0x%x (%hhu), state=%d, window='%s'.",
                        mouse->detail, mouse->detail, state, client->window->title
                    );
                    if(client->window->on_mouse_button)
                    {
                        client->window->on_mouse_button(vbutton, state);
                    }
                } break;

                // Обработка положения указателя.
                case XCB_MOTION_NOTIFY:
                {
                    xcb_motion_notify_event_t* mouse = (xcb_motion_notify_event_t*)event;

                    // LOG_TRACE("Motion event to %hdx%hd for '%s' window.", mouse->event_x, mouse->event_y, client->window->title);
                    if(client->window->on_mouse_move)
                    {
                        client->window->on_mouse_move((i32)mouse->event_x, (i32)mouse->event_y);
                    }
                } break;

                // Обработка изменения размера окна.
                case XCB_CONFIGURE_NOTIFY:
                {
                    xcb_configure_notify_event_t* e = (xcb_configure_notify_event_t*)event;
                    if(client->window->width != e->width || client->window->height != e->height)
                    {
                        client->window->width_pending  = (u32)e->width;
                        client->window->height_pending = (u32)e->height;
                        client->window->resize_pending = true;
                        LOG_TRACE("Window resize pending: %hux%hu.", e->width, e->height);
                    }
                } break;

                // Обработка перерисовки окна.
                case XCB_EXPOSE:
                {
                    if(client->window->resize_pending)
                    {
                        LOG_TRACE("Resize event: window='%s' to %ux%u.",
                            client->window->title, client->window->width_pending, client->window->height_pending
                        );

                        // Вызов обработчика изменения размера буфера.
                        if(client->window->on_resize)
                        {
                            client->window->on_resize(client->window->width_pending, client->window->height_pending);
                        }

                        client->window->width = client->window->width_pending;
                        client->window->height = client->window->height_pending;
                        client->window->resize_pending = false;
                    }
                } break;

                // Обработка события фокуса окна.
                case XCB_ENTER_NOTIFY:
                {
                    // TODO: Найти окно которому принадлежит id!
                    // xcb_enter_notify_event_t* e = (xcb_enter_notify_event_t*)event;
                    // if(client->window->id == e->event)
                    // {
                    //     client->focused_window = client->window;
                    //     client->window->focused = true;
                    // }

                    LOG_TRACE("Window '%s' focused.", client->window->title);
                    if(client->window->on_focus)
                    {
                        client->window->on_focus(true);
                    }
                } break;

                case XCB_LEAVE_NOTIFY:
                {
                    // TODO: Найти окно которому принадлежит id!
                    // xcb_enter_notify_event_t* e = (xcb_enter_notify_event_t*)event;
                    // if(client->window->id == e->event)
                    // {
                    //     client->window->focused = false;
                    //     if(client->focused_window == client->window)
                    //     {
                    //         client->focused_window = nullptr;
                    //     }
                    // }

                    LOG_TRACE("Window '%s' lost focus.", client->window->title);
                    if(client->window->on_focus)
                    {
                        client->window->on_focus(false);
                    }
                } break;

                // Обработка запроса о закрытии окна.
                case XCB_CLIENT_MESSAGE:
                {
                    xcb_client_message_event_t* e = (xcb_client_message_event_t*)event;

                    LOG_TRACE("Close event for window '%s'.", client->window->title);
                    if(client->window->on_close && e->data.data32[0] == client->window->wm_delete)
                    {
                        client->window->on_close();
                    }

                } break;

                // Обработка появления окна.
                case XCB_MAP_NOTIFY:
                {
                } break;

                // Обработка неизвестных событий.
                default:
                    LOG_TRACE("Unhandled event type: %d.", response_type);
            }

            // Удаление обработанного события.
            free(event);
        }

        return true;
    }

    bool xcb_backend_is_supported()
    {
        xcb_connection_t* connection = xcb_connect(nullptr, nullptr);
        if(xcb_connection_has_error(connection))
        {
            return false;
        }
        xcb_disconnect(connection);
        return true;
    }

    u64 xcb_backend_memory_requirement()
    {
        return sizeof(xcb_client);
    }

    platform_window* xcb_backend_window_create(const platform_window_config* config, void* internal_data)
    {
        xcb_client* client = internal_data;

        LOG_TRACE("Creating window '%s' (size: %ux%u)...", config->title, config->width, config->height);

        if(client->window)
        {
            LOG_WARN("Window has already been created (only one window supported for now).");
            return nullptr;
        }

        if(!client->connection || xcb_connection_has_error(client->connection))
        {
            LOG_ERROR("XCB connection is not available.");
            return nullptr;
        }

        platform_window* window = mallocate(sizeof(platform_window), MEMORY_TAG_APPLICATION);
        mzero(window, sizeof(platform_window));
        window->connection = client->connection;
        window->visual = client->visual;
        window->title = string_duplicate(config->title);
        window->width = config->width;
        window->height = config->height;
        window->on_close = config->on_close;
        window->on_resize = config->on_resize;
        window->on_focus = config->on_focus;
        window->on_key = config->on_key;
        window->on_mouse_button = config->on_mouse_button;
        window->on_mouse_move = config->on_mouse_move;
        window->on_mouse_wheel = config->on_mouse_wheel;

        // Генерация ID для нового окна.
        window->window = xcb_generate_id(client->connection);
        LOG_TRACE("XCB window ID: %u.", window->window);

        // Настройка событий окна.
        u32 value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;                // Установка фона и событий для окна.
        u32 value_list[] = {
            client->screen->black_pixel,
            XCB_EVENT_MASK_KEY_PRESS      | XCB_EVENT_MASK_KEY_RELEASE      |  // События клавиш клавиатуры.
            XCB_EVENT_MASK_BUTTON_PRESS   | XCB_EVENT_MASK_BUTTON_RELEASE   |  // События кнопок мыши.
            XCB_EVENT_MASK_ENTER_WINDOW   | XCB_EVENT_MASK_LEAVE_WINDOW     |  // События фокуса окна.
            XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY |  // События движение мыши и изменениеы размера окна.
            XCB_EVENT_MASK_EXPOSURE                                            // Событие перерисовка окна.
        };

        // Создание окна.
        xcb_create_window(
            client->connection, XCB_COPY_FROM_PARENT, window->window, client->screen->root, 0, 0, config->width, config->height,
            0, XCB_WINDOW_CLASS_INPUT_OUTPUT, client->screen->root_visual, value_mask, value_list
        );

        // Установка заголовка окна.
        xcb_change_property(
            client->connection, XCB_PROP_MODE_REPLACE, window->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
            string_length(window->title), window->title
        );

        // Установка класса окна (WM_CLASS).
        // NOTE: Согласно ICCCM и EWMH, свойство WM_CLASS состоит из двух частей: "instance_name\0class_name\0".
        char wm_class[50];
        u32 wm_class_length = string_format(wm_class, 50, "Window%d%cGameEngineWindow", window->window, '\0') + 1;
        xcb_change_property(
            client->connection, XCB_PROP_MODE_REPLACE, window->window, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8,
            wm_class_length, wm_class
        );

        // Подписка окна на протокол WM_DELETE_WINDOW.
        // NOTE: Без него программа не получает события о закрытии окна, сервер просто грохнет окно
        //       и приложение уйдет в ошибки при очередном цикле обработки событий.
        // Запрос.
        xcb_intern_atom_cookie_t cookies[2] = {
            xcb_intern_atom(client->connection, 0, 16, "WM_DELETE_WINDOW"),
            xcb_intern_atom(client->connection, 0, 12, "WM_PROTOCOLS")
        };

        // Ответ.
        xcb_intern_atom_reply_t* replies[2] = {
            xcb_intern_atom_reply(client->connection, cookies[0], nullptr), // wm_delete_window.
            xcb_intern_atom_reply(client->connection, cookies[1], nullptr)  // wm_protocols.
        };

        // Проверка ответов.
        if(!replies[0] || !replies[1])
        {
            if(replies[0]) free(replies[0]);
            if(replies[1]) free(replies[1]);

            LOG_ERROR("Failed to get WM protocol atoms.");
            xcb_backend_window_destroy(window, client);
            return nullptr;
        }

        // Сохранение атомов.
        xcb_atom_t wm_delete = replies[0]->atom;
        xcb_atom_t wm_protocols = replies[1]->atom;
        free(replies[0]);
        free(replies[1]);

        // Подписка на WM_DELETE_WINDOW.
        xcb_change_property(
            client->connection, XCB_PROP_MODE_REPLACE, window->window, wm_protocols, XCB_ATOM_ATOM, 32, 1, &wm_delete
        );

        // Сохранение атома(числа) для дальнейшего использования.
        window->wm_delete = wm_delete;

        // Показать окно.
        xcb_map_window(client->connection, window->window);
        if(xcb_flush(client->connection) <= 0)
        {
            LOG_ERROR("Failed to flush XCB connection (server disconnected).");
            xcb_backend_window_destroy(window, client);
            return false;
        }

        LOG_TRACE("Window '%s' created successfully.", window->title);

        client->window = window;
        return client->window;
    }

    void xcb_backend_window_destroy(platform_window* window, void* internal_data)
    {
        xcb_client* client = internal_data;

        LOG_TRACE("Destroying window '%s'...", window->title);

        // TODO: Удаление окна из списка окон.

        if(window->window >= 0) xcb_destroy_window(client->connection, window->window);
        if(window->title)   string_free(window->title);
        xcb_flush(client->connection);

        mfree(window, sizeof(platform_window), MEMORY_TAG_APPLICATION);
        client->window = nullptr;
        LOG_TRACE("Window destroy complete.");
    }

    void xcb_backend_enumerate_vulkan_extensions(u32* extension_count, const char** out_extensions)
    {
        static const char* extensions[] = {VK_KHR_XCB_SURFACE_EXTENSION_NAME};

        if(out_extensions == nullptr)
        {
            *extension_count = sizeof(extensions) / sizeof(char*);
            return;
        }

        mcopy(out_extensions, extensions, sizeof(extensions));
    }

    u32 xcb_backend_create_vulkan_surface(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void** out_vulkan_surface)
    {
        VkXcbSurfaceCreateInfoKHR surface_create_info = {
            .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
            .connection = window->connection,
            .window = window->window
        };

        return vkCreateXcbSurfaceKHR(vulkan_instance, &surface_create_info, vulkan_allocator, *out_vulkan_surface);
    }

    void xcb_backend_destroy_vulkan_surface(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void* vulkan_surface)
    {
        UNUSED(window);
        vkDestroySurfaceKHR(vulkan_instance, vulkan_surface, vulkan_allocator);
    }

    u32 xcb_backend_supports_vulkan_presentation(platform_window* window, void* vulkan_pyhical_device, u32 queue_family_index)
    {
        return vkGetPhysicalDeviceXcbPresentationSupportKHR(vulkan_pyhical_device, queue_family_index, window->connection, window->visual);
    }

#endif
