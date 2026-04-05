#include "platform/window.h"

#ifdef PLATFORM_WINDOWS_FLAG

    #include "core/logger.h"
    #include "core/memory.h"
    #include "core/string.h"
    #include "debug/assert.h"
    #include <Windows.h>
    #include <Windowsx.h>
    #include <hidusage.h>
    #include <vulkan/vulkan.h>
    #include <vulkan/vulkan_win32.h>

    // Для включения отладки для оконной системы индивидуально.
    #ifndef DEBUG_PLATFORM_FLAG
        #undef LOG_DEBUG
        #undef LOG_TRACE
        #define LOG_DEBUG(...) UNUSED(0)
        #define LOG_TRACE(...) UNUSED(0)
    #endif

    struct platform_window {
        // Дискриптор окна.
        HWND hwnd;
        // Текущий заголовок окна.
        const char* title;
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
        // Пустой курсор (НАСЛЕДУЕТ).
        HCURSOR cursor_blank;
        // Стандартный курсор (НАСЛЕДУЕТ).
        HCURSOR cursor_default;
        // Статус блокировки курсора.
        bool cursor_locked;
        // Зарегестрированные слушатели событий (по одному на событие).
        platform_window_event_listener_t event_listeners[PLATFORM_WINDOW_EVENT_COUNT];
    };

    typedef struct platform_window_client {
        //
        HINSTANCE hinstance;
        //
        const char* window_class;
        //
        u64 memory_requirement;
        //
        platform_window_t* window;
        // Пустой курсор.
        HCURSOR cursor_blank;
        // Стандартный курсор.
        HCURSOR cursor_default;
    } platform_window_client;

    static platform_window_client* client = nullptr;

    static keyboard_key key_translate(u64 code, i64 param)
    {
        // TODO: Когда выключен NUMLOCK поведение NUM5 -> VK_CLEAR
        static const keyboard_key codes[KEY_COUNT] = {
            [KEY_BACKSPACE   ] = KEY_BACKSPACE,   [KEY_J           ] = KEY_J,           [KEY_F2          ] = KEY_F2,
            [KEY_TAB         ] = KEY_TAB,         [KEY_K           ] = KEY_K,           [KEY_F3          ] = KEY_F3,
            [KEY_RETURN      ] = KEY_RETURN,      [KEY_L           ] = KEY_L,           [KEY_F4          ] = KEY_F4,
            [KEY_PAUSE       ] = KEY_PAUSE,       [KEY_M           ] = KEY_M,           [KEY_F5          ] = KEY_F5,
            [KEY_CAPSLOCK    ] = KEY_CAPSLOCK,    [KEY_N           ] = KEY_N,           [KEY_F6          ] = KEY_F6,
            [KEY_ESCAPE      ] = KEY_ESCAPE,      [KEY_O           ] = KEY_O,           [KEY_F7          ] = KEY_F7,
            [KEY_SPACE       ] = KEY_SPACE,       [KEY_P           ] = KEY_P,           [KEY_F8          ] = KEY_F8,
            [KEY_PAGEUP      ] = KEY_PAGEUP,      [KEY_Q           ] = KEY_Q,           [KEY_F9          ] = KEY_F9,
            [KEY_PAGEDOWN    ] = KEY_PAGEDOWN,    [KEY_R           ] = KEY_R,           [KEY_F10         ] = KEY_F10,
            [KEY_END         ] = KEY_END,         [KEY_S           ] = KEY_S,           [KEY_F11         ] = KEY_F11,
            [KEY_HOME        ] = KEY_HOME,        [KEY_T           ] = KEY_T,           [KEY_F12         ] = KEY_F12,
            [KEY_LEFT        ] = KEY_LEFT,        [KEY_U           ] = KEY_U,           [KEY_F13         ] = KEY_F13,
            [KEY_UP          ] = KEY_UP,          [KEY_V           ] = KEY_V,           [KEY_F14         ] = KEY_F14,
            [KEY_RIGHT       ] = KEY_RIGHT,       [KEY_W           ] = KEY_W,           [KEY_F15         ] = KEY_F15,
            [KEY_DOWN        ] = KEY_DOWN,        [KEY_X           ] = KEY_X,           [KEY_F16         ] = KEY_F16,
            [KEY_PRINTSCREEN ] = KEY_PRINTSCREEN, [KEY_Y           ] = KEY_Y,           [KEY_F17         ] = KEY_F17,
            [KEY_INSERT      ] = KEY_INSERT,      [KEY_Z           ] = KEY_Z,           [KEY_F18         ] = KEY_F18,
            [KEY_DELETE      ] = KEY_DELETE,      [KEY_LSUPER      ] = KEY_LSUPER,      [KEY_F19         ] = KEY_F19,
            [KEY_0           ] = KEY_0,           [KEY_RSUPER      ] = KEY_RSUPER,      [KEY_F20         ] = KEY_F20,
            [KEY_1           ] = KEY_1,           [KEY_MENU        ] = KEY_MENU,        [KEY_F21         ] = KEY_F21,
            [KEY_2           ] = KEY_2,           [KEY_SLEEP       ] = KEY_SLEEP,       [KEY_F22         ] = KEY_F22,
            [KEY_3           ] = KEY_3,           [KEY_NUMPAD0     ] = KEY_NUMPAD0,     [KEY_F23         ] = KEY_F23,
            [KEY_4           ] = KEY_4,           [KEY_NUMPAD1     ] = KEY_NUMPAD1,     [KEY_F24         ] = KEY_F24,
            [KEY_5           ] = KEY_5,           [KEY_NUMPAD2     ] = KEY_NUMPAD2,     [KEY_NUMLOCK     ] = KEY_NUMLOCK,
            [KEY_6           ] = KEY_6,           [KEY_NUMPAD3     ] = KEY_NUMPAD3,     [KEY_SCROLLLOCK  ] = KEY_SCROLLLOCK,
            [KEY_7           ] = KEY_7,           [KEY_NUMPAD4     ] = KEY_NUMPAD4,     [KEY_SEMICOLON   ] = KEY_SEMICOLON,
            [KEY_8           ] = KEY_8,           [KEY_NUMPAD5     ] = KEY_NUMPAD5,     [KEY_APOSTROPHE  ] = KEY_APOSTROPHE,
            [KEY_9           ] = KEY_9,           [KEY_NUMPAD6     ] = KEY_NUMPAD6,     [KEY_EQUAL       ] = KEY_EQUAL,
            [KEY_A           ] = KEY_A,           [KEY_NUMPAD7     ] = KEY_NUMPAD7,     [KEY_COMMA       ] = KEY_COMMA,
            [KEY_B           ] = KEY_B,           [KEY_NUMPAD8     ] = KEY_NUMPAD8,     [KEY_MINUS       ] = KEY_MINUS,
            [KEY_C           ] = KEY_C,           [KEY_NUMPAD9     ] = KEY_NUMPAD9,     [KEY_DOT         ] = KEY_DOT,
            [KEY_D           ] = KEY_D,           [KEY_MULTIPLY    ] = KEY_MULTIPLY,    [KEY_SLASH       ] = KEY_SLASH,
            [KEY_E           ] = KEY_E,           [KEY_ADD         ] = KEY_ADD,         [KEY_GRAVE       ] = KEY_GRAVE,
            [KEY_F           ] = KEY_F,           [KEY_SUBTRACT    ] = KEY_SUBTRACT,    [KEY_LBRACKET    ] = KEY_LBRACKET,
            [KEY_G           ] = KEY_G,           [KEY_DECIMAL     ] = KEY_DECIMAL,     [KEY_BACKSLASH   ] = KEY_BACKSLASH,
            [KEY_H           ] = KEY_H,           [KEY_DIVIDE      ] = KEY_DIVIDE,      [KEY_RBRACKET    ] = KEY_RBRACKET,
            [KEY_I           ] = KEY_I,           [KEY_F1          ] = KEY_F1,
        };

        if(code >= KEY_COUNT)
        {
            return KEY_UNKNOWN;
        }

        // Преобразование для специальных клавиш.
        keyboard_key key = codes[code];
        if(key == KEY_UNKNOWN)
        {
            param = HIWORD(param);

            switch(code)
            {
                case VK_SHIFT:
                    key = ((param & 0xff) == 0x36) ? KEY_RSHIFT : KEY_LSHIFT;
                    break;
                case VK_CONTROL:
                    key = (param & KF_EXTENDED) ? KEY_RCONTROL : KEY_LCONTROL;
                    break;
                case VK_MENU:
                    key = (param & KF_EXTENDED) ? KEY_RALT : KEY_LALT;
                    break;
            }
        }

        return key;
    }

    static mouse_button button_translate(u32 code)
    {
        switch(code)
        {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
                 return BUTTON_LEFT;
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
                return BUTTON_MIDDLE;
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
                return BUTTON_RIGHT;
        }

        return BUTTON_UNKNOWN;
    }

    static void cursor_lock()
    {
        // TODO: При блокировке курсора за предеами окна по событиям кнопок мыши блокировка исчезает.

        // Получение текущей позиции курсора.
        POINT currentPos;
        GetCursorPos(&currentPos);

        // Создание прямоугольника размером 1x1 пиксель для ограничения курсора.
        RECT lockRect;
        lockRect.left = currentPos.x;
        lockRect.top = currentPos.y;
        lockRect.right = currentPos.x + 1;
        lockRect.bottom = currentPos.y + 1;

        // Ограничение курсора этим прямоугольником.
        ClipCursor(&lockRect);
    }

    static void cursor_unlock()
    {
        ClipCursor(nullptr);
    }

    LRESULT CALLBACK window_process(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        // Текущее состояние кнопки, клавиши.
        bool state = false;
        platform_window_t* window = client->window; // TODO: Текущее окно! GetActiveWindow()???

        switch(msg)
        {
            // TODO:
            // case WM_ERASEBKGND:
            //     return 1;

            // TODO:
            // case WM_CHAR: {
            // } return 0;

            // См. https://learn.microsoft.com/en-us/windows/win32/learnwin32/keyboard-input
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                state = true;
            case WM_KEYUP:
            case WM_SYSKEYUP: {
                keyboard_key key = key_translate(wparam, lparam);
                if(key == KEY_UNKNOWN)
                {
                    LOG_WARN("Keyboard key event: unknown wparam=0x%x (%llu), lparam 0x%x (%lli), state=%d.",
                        wparam, wparam, lparam, lparam, state
                    );
                    return 0;
                }

                // Вызов обработчика окна.
                platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_KEYBOARD_KEY];
                if(listener->callback != nullptr)
                {
                    platform_window_event_context_t context = {
                        .type                 = PLATFORM_WINDOW_EVENT_KEYBOARD_KEY,
                        .user_data            = listener->user_data,
                        .window               = window,
                        .keyboard_key.code    = key,
                        .keyboard_key.state   = state
                    };

                    listener->callback(&context);
                }
            } return 0;

            // См. https://learn.microsoft.com/en-us/windows/win32/learnwin32/mouse-clicks
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_XBUTTONDOWN:
                state = true;
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
            case WM_XBUTTONUP: {
                mouse_button button = button_translate(msg);
                if(button == BUTTON_UNKNOWN)
                {
                    LOG_WARN("Mouse button event: unknown msg=0x%x (%u), state=%d.", msg, msg, state);
                    return 0;
                }

                // Вызов обработчика окна.
                platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_MOUSE_BUTTON];
                if(listener->callback != nullptr)
                {
                    platform_window_event_context_t context = {
                        .type                 = PLATFORM_WINDOW_EVENT_MOUSE_BUTTON,
                        .user_data            = listener->user_data,
                        .window               = window,
                        .mouse_button.code    = button,
                        .mouse_button.state   = state
                    };

                    listener->callback(&context);
                }
            } return 0;

            case WM_INPUT: {
                UINT size;
                BYTE buffer[128]; // Использование стека для скорости (обычно размер < 64 байт).

                // Получение размера структуры.
                GetRawInputData((HRAWINPUT)lparam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));

                // Проверка размера.
                if(size > sizeof(buffer))
                {
                    LOG_ERROR("Raw data greater than buffer size of 128 bytes!");
                    return 0;
                }

                if(GetRawInputData((HRAWINPUT)lparam, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) != size)
                {
                    LOG_ERROR("GetRawInputData does not return correct size!");
                    break; // NOTE: Важно вызвать DefWindowProc, чтобы система могла выполнить стандартную очистку сообщения.
                }

                RAWINPUT* raw = (RAWINPUT*)buffer;
                if(raw->header.dwType == RIM_TYPEMOUSE)
                {
                    RAWMOUSE* mouse = &raw->data.mouse;
                    platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_MOUSE_MOVE_RELATIVE];
                    if(listener->callback != nullptr)
                    {
                        // Вызов обработчика окна для относительного смещения указателя.
                        platform_window_event_context_t context = {
                            .type                   = PLATFORM_WINDOW_EVENT_MOUSE_MOVE_RELATIVE,
                            .user_data              = listener->user_data,
                            .window                 = window,
                            .mouse_move_relative.dx = CAST_I32(mouse->lLastX),
                            .mouse_move_relative.dy = CAST_I32(mouse->lLastY)
                        };

                        listener->callback(&context);
                    }
                }
            } return 0;

            case WM_MOUSEMOVE: {
                platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_MOUSE_MOVE];
                if(listener->callback != nullptr)
                {
                    // Вызов обработчика абсолютных координат указателя, только если курсор не заблокирован!
                    platform_window_event_context_t context = {
                        .type            = PLATFORM_WINDOW_EVENT_MOUSE_MOVE,
                        .user_data       = listener->user_data,
                        .window          = window,
                        .mouse_move.to_x = CAST_I32(GET_X_LPARAM(lparam)),
                        .mouse_move.to_y = CAST_I32(GET_Y_LPARAM(lparam))
                    };

                    listener->callback(&context);
                }
                
            } return 0;

            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL: {
                // Вызов обработчика окна.
                platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_MOUSE_WHEEL];
                if(listener->callback != nullptr)
                {
                    // TODO: Исправать проблему деления.
                    i32 delta_axis = GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;
                    i32 delta_vert = msg == WM_MOUSEWHEEL  ? delta_axis : 0;
                    i32 delta_horz = msg == WM_MOUSEHWHEEL ? delta_axis : 0;

                    platform_window_event_context_t context = {
                        .type                   = PLATFORM_WINDOW_EVENT_MOUSE_WHEEL,
                        .user_data              = listener->user_data,
                        .window                 = window,
                        .mouse_wheel.delta_vert = delta_vert,
                        .mouse_wheel.delta_horz = delta_horz
                    };

                    listener->callback(&context);
                }
            } return 0;

            case WM_SIZE: {
                // TODO: Нужно ли renderer_system_is_initialized()???

                // Вызов обработчика окна.
                platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_RESIZE];
                if(listener->callback != nullptr)
                {
                    // NOTE: Для получения реальной области поверхности.
                    RECT rect;
                    GetClientRect(hwnd, &rect);

                    platform_window_event_context_t context = {
                        .type                    = PLATFORM_WINDOW_EVENT_RESIZE,
                        .user_data               = listener->user_data,
                        .window                  = window,
                        .window_resize.state     = window->resize_pending,
                        .window_resize.to_width  = CAST_U32(rect.right - rect.left),
                        .window_resize.to_height = CAST_U32(rect.bottom - rect.top)
                    };

                    listener->callback(&context);
                }
            } return 0;

            case WM_SETFOCUS: {
                // Вызов обработчика окна.
                platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_FOCUS];
                if(listener->callback != nullptr)
                {
                    platform_window_event_context_t context = {
                        .type               = PLATFORM_WINDOW_EVENT_FOCUS,
                        .user_data          = listener->user_data,
                        .window             = window,
                        .window_focus.state = true
                    };

                    listener->callback(&context);
                }

                // NOTE: Блокировка курсора, если нужно.
                if(window->cursor_locked)
                {
                    cursor_lock();
                }
            } return 0;

            case WM_KILLFOCUS:  {
                // NOTE: Разблокировка!
                cursor_unlock();

                // Вызов обработчика окна.
                platform_window_event_listener_t* listener = &window->event_listeners[PLATFORM_WINDOW_EVENT_FOCUS];
                if(listener->callback != nullptr)
                {
                    platform_window_event_context_t context = {
                        .type               = PLATFORM_WINDOW_EVENT_FOCUS,
                        .user_data          = listener->user_data,
                        .window             = window,
                        .window_focus.state = false
                    };

                    listener->callback(&context);
                }
            } return 0;

            case WM_CLOSE: {
                // Вызов обработчика окна.
                platform_window_event_listener_t* listener = &client->window->event_listeners[PLATFORM_WINDOW_EVENT_SHOULD_CLOSE];
                if(listener->callback != nullptr)
                {
                    platform_window_event_context_t context = {
                        .type      = PLATFORM_WINDOW_EVENT_SHOULD_CLOSE,
                        .user_data = listener->user_data,
                        .window    = client->window,
                    };

                    listener->callback(&context);
                }
            } return 0;

            default:
                // LOG_TRACE("Unhandled event type: %u.", msg);
                break;
        }

        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    bool platform_window_initialize(platform_window_backend_t type)
    {
        ASSERT(client == nullptr, "Window subsystem is already initialized.");
        ASSERT(type < PLATFORM_WINDOW_BACKEND_COUNT, "Must be less than PLATFORM_WINDOW_BACKEND_TYPE_COUNT.");

        if(type != PLATFORM_WINDOW_BACKEND_DEFAULT && type != PLATFORM_WINDOW_BACKEND_WIN32)
        {
            LOG_ERROR("Unsupported window backend type selected for Windows: %d.", type);
            return false;
        }
        type = PLATFORM_WINDOW_BACKEND_WIN32;

        u64 memory_requirement = sizeof(platform_window_client);
        client = mallocate(memory_requirement, MEMORY_TAG_APPLICATION);
        mzero(client, memory_requirement);

        client->memory_requirement = memory_requirement;
        client->window_class = "EngineGameWindow";
        client->hinstance = GetModuleHandle(nullptr);
        client->cursor_default = LoadCursor(nullptr, IDC_ARROW);

        // Регистрация класса окна.
        WNDCLASSEX wc = {
            .cbSize        = sizeof(WNDCLASSEX),
            .style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
            .lpfnWndProc   = window_process,
            .hInstance     = client->hinstance,
            .lpszClassName = client->window_class,
            .hCursor       = client->cursor_default,
            .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
            .hIcon         = LoadIcon(nullptr, IDI_APPLICATION),
            .hIconSm       = LoadIcon(nullptr, IDI_APPLICATION)
        };

        if(!RegisterClassEx(&wc))
        {
            LOG_ERROR("Failed to register window class '%s': %lu.", client->window_class, GetLastError());
            platform_window_shutdown();
            return false;
        }

        // см. https://learn.microsoft.com/ru-ru/windows/win32/inputdev/about-raw-input
        // см. https://learn.microsoft.com/ru-ru/windows/win32/api/winuser/ns-winuser-rawinputdevice
        // см. https://learn.microsoft.com/ru-ru/windows/win32/api/winuser/nf-winuser-registerrawinputdevices
        RAWINPUTDEVICE raw_input = {
            .usUsagePage = HID_USAGE_PAGE_GENERIC,
            .usUsage     = HID_USAGE_GENERIC_MOUSE,
            .dwFlags     = 0,                        // получать сообщения только когда окно активно.
            .hwndTarget  = nullptr                   // получать сообщения для всех окон приложения.
        };

        if(RegisterRawInputDevices(&raw_input, 1, sizeof(RAWINPUTDEVICE)) == false)
        {
            LOG_ERROR("Failed to register raw input device: %lu.", GetLastError());
            return false;
        }

        // Созданеи пустого курсора.
        HBITMAP hAndMask = CreateBitmap(1, 1, 1, 1, NULL);
        HBITMAP hXorMask = CreateBitmap(1, 1, 1, 1, NULL);

        if(!hAndMask || !hXorMask)
        {
            if(hAndMask) DeleteObject(hAndMask);
            if(hXorMask) DeleteObject(hXorMask);
            LOG_ERROR("Failed to create masks to invisible cursor.");
            return false;
        }

        ICONINFO iconinfo = {
            .fIcon = false,
            .xHotspot = 0,
            .yHotspot = 0,
            .hbmMask = hAndMask,
            .hbmColor = hXorMask
        };

        client->cursor_blank = CreateIconIndirect(&iconinfo);

        // Очистка временных ресурсов.
        DeleteObject(hAndMask);
        DeleteObject(hXorMask);

        return true;
    }

    void platform_window_shutdown()
    {
        ASSERT(client != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");

        // Уничтожение всех окон (пока только одно окно).
        if(client->window)
        {
            LOG_WARN("Window was not properly destroyed before shutdown.");
            platform_window_destroy(client->window);
            // NOTE: Осовобождение context->window в platform_window_destroy().
        }

        // Уничтоженрие невидимого курсора.
        DestroyCursor(client->cursor_blank);

        // Убираем регистрацию класса окна
        if(client->window_class)
        {
            UnregisterClass(client->window_class, client->hinstance);
        }

        mfree(client, client->memory_requirement, MEMORY_TAG_APPLICATION);
        client = nullptr;
    }

    bool platform_window_is_initialized()
    {
        return client != nullptr;
    }

    bool platform_window_poll_events()
    {
        ASSERT(client != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");

        MSG msg = {};
        while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return true;
    }

    platform_window_t* platform_window_create(const platform_window_config_t* config)
    {
        ASSERT(client != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(config != nullptr, "Config pointer must be non-null.");

        if(client->window)
        {
            LOG_WARN("Window has already been created (only one window supported for now).");
            return nullptr;
        }

        client->window = mallocate(sizeof(platform_window_t), MEMORY_TAG_APPLICATION);
        mzero(client->window, sizeof(platform_window_t));

        // NOTE: При создании окна важно учитывать, что системные рамки и элементы управления (заголовок, меню и т.д.)
        //       занимают дополнительное место. Функция ниже корректирует прямоугольник клиентской области так, чтобы
        //       после создания окна его внутренняя рабочая область имела требуемый размер.
        RECT rect = {0, 0, config->width, config->height};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW | WS_VISIBLE, false);

        client->window->title          = string_duplicate(config->title);
        client->window->width          = config->width;
        client->window->height         = config->height;
        client->window->cursor_blank   = client->cursor_blank;
        client->window->cursor_default = client->cursor_default;

        client->window->hwnd = CreateWindowEx(
            WS_EX_APPWINDOW,client->window_class, config->title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
            CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, client->hinstance, nullptr
        );

        if(client->window->hwnd == nullptr)
        {
            LOG_ERROR("Failed to create window: %lu.", GetLastError());
            platform_window_destroy(client->window);
            return nullptr;
        }

        // Сохранение указателя на структуру окна в userdata.
        // TODO: Использовать.
        SetWindowLongPtr(client->window->hwnd, GWLP_USERDATA, (LONG_PTR)client->window);

        // Отображение окна.
        ShowWindow(client->window->hwnd, SW_SHOW);
        UpdateWindow(client->window->hwnd);

        LOG_TRACE("Window '%s' created successfully.", client->window->title);

        return client->window;
    }

    void platform_window_destroy(platform_window_t* window)
    {
        ASSERT(client != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");

        LOG_TRACE("Destroying window '%s'...", window->title);

        if(window->hwnd)
        {
            SetWindowLongPtr(window->hwnd, GWLP_USERDATA, (LONG_PTR)nullptr);
            DestroyWindow(window->hwnd);
        }

        if(window->title)
        {
            string_free(window->title);
        }

        mfree(window, sizeof(platform_window_t), MEMORY_TAG_APPLICATION);
        client->window = nullptr;
        LOG_TRACE("Window destroy complete.");
    }

    const char* platform_window_get_title(platform_window_t* window)
    {
        ASSERT(client != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");

        return window->title;
    }

    void platform_window_get_resolution(platform_window_t* window, u32* width, u32* height)
    {
        ASSERT(client != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");
        ASSERT(width != nullptr, "Poiner to width must be nun-null.");
        ASSERT(height != nullptr, "Pointer to height must be non-null.");

        *width = window->width;
        *height = window->height;
    }

    void platform_window_set_event_callback(platform_window_t* window, platform_window_event_t event, platform_window_event_callback callback, void* user_data)
    {
        window->event_listeners[event] = (platform_window_event_listener_t){
            .callback  = callback,
            .user_data = user_data
        };
    }

    void platform_window_hide_cursor(platform_window_t* window)
    {
        SetClassLongPtr(window->hwnd, GCLP_HCURSOR, (LONG_PTR)window->cursor_blank);
    }

    void platform_window_show_cursor(platform_window_t* window)
    {
        SetClassLongPtr(window->hwnd, GCLP_HCURSOR, (LONG_PTR)window->cursor_default);
    }

    void platform_window_lock_cursor(platform_window_t* window)
    {
        cursor_lock();
        window->cursor_locked = true;
    }

    void platform_window_unlock_cursor(platform_window_t* window)
    {
        cursor_unlock();
        window->cursor_locked = false;
    }

    void platform_window_enumerate_vulkan_extensions(u32* extension_count, const char** out_extensions)
    {
        ASSERT(client != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(extension_count != nullptr, "Pointer 'extension_count' must be non-null.");

        static const char* extensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        };

        if(out_extensions == nullptr)
        {
            *extension_count = sizeof(extensions) / sizeof(char*);
            return;
        }

        mcopy(out_extensions, extensions, sizeof(char*) * (*extension_count));
    }

    u32 platform_window_create_vulkan_surface(platform_window_t* window, void* vulkan_instance, void* vulkan_allocator, void** out_vulkan_surface)
    {
        ASSERT(client != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");
        ASSERT(vulkan_instance != nullptr, "Vulkan instance pointer must be non-null.");
        ASSERT(out_vulkan_surface != nullptr, "Vulkan surface pointer must be non-null.");

        VkWin32SurfaceCreateInfoKHR surface_create_info = {
            .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = client->hinstance,
            .hwnd      = window->hwnd
        };

        return vkCreateWin32SurfaceKHR(vulkan_instance, &surface_create_info, vulkan_allocator, (VkSurfaceKHR*)out_vulkan_surface);
    }

    void platform_window_destroy_vulkan_surface(platform_window_t* window, void* vulkan_instance, void* vulkan_allocator, void* vulkan_surface)
    {
        ASSERT(client != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");
        ASSERT(vulkan_instance != nullptr, "Vulkan instance pointer must be non-null.");
        ASSERT(vulkan_surface != nullptr, "Vulkan surface pointer must be non-null.");

        UNUSED(window);
        vkDestroySurfaceKHR(vulkan_instance, vulkan_surface, vulkan_allocator);
    }

    u32 platform_window_supports_vulkan_presentation(platform_window_t* window, void* vulkan_pyhical_device, u32 queue_family_index)
    {
        ASSERT(client != nullptr, "Window subsystem not initialized. Call platform_window_initialize() first.");
        ASSERT(window != nullptr, "Window pointer must be non-null.");
        ASSERT(vulkan_pyhical_device != nullptr, "Vulkan physical device pointer must be non-null.");

        return vkGetPhysicalDeviceWin32PresentationSupportKHR(vulkan_pyhical_device, queue_family_index);
    }

#endif
