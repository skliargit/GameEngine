#include "platform/window.h"

#ifdef PLATFORM_WINDOWS_FLAG

    #include "core/logger.h"
    #include "core/memory.h"
    #include "core/string.h"
    #include "debug/assert.h"
    #include <Windows.h>

    // Для включения отладки для оконной системы индивидуально.
    #ifndef DEBUG_WINDOW_FLAG
        #undef LOG_DEBUG
        #undef LOG_TRACE
        #define LOG_DEBUG(...) UNUSED(0)
        #define LOG_TRACE(...) UNUSED(0)
    #endif

    typedef struct platform_window {
        HWND hwnd;
        const char* title;
        u32 width;
        u32 height;
        bool should_close;
    } platform_window;

    typedef struct platform_window_context {
        HINSTANCE hinstance;
        const char* window_class;
        u64 memory_requirement;
        platform_window* window;
    } platform_window_context;

    static platform_window_context* context = nullptr;
    static LRESULT CALLBACK window_process(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    bool platform_window_initialize(platform_window_backend_type type)
    {
        ASSERT(context == nullptr, "Platform window is already initialized.");
        ASSERT(type < PLATFORM_WINDOW_BACKEND_TYPE_COUNT, "Must be less than PLATFORM_WINDOW_BACKEND_TYPE_COUNT.");

        if(type != PLATFORM_WINDOW_BACKEND_TYPE_AUTO && type != PLATFORM_WINDOW_BACKEND_TYPE_WIN32)
        {
            LOG_FATAL("Unsupported window backend type selected for Windows: %d.", type);
            return false;
        }
        type = PLATFORM_WINDOW_BACKEND_TYPE_WIN32;

        u64 memory_requirement = sizeof(platform_window_context);
        context = mallocate(memory_requirement, MEMORY_TAG_APPLICATION);
        mzero(context, memory_requirement);

        context->memory_requirement = memory_requirement;
        context->window_class = "EngineGameWindow";
        context->hinstance = GetModuleHandle(nullptr);

        // Регистрация класса окна.
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = window_process;
        wc.hInstance = context->hinstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = context->window_class;
        wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

        if(!RegisterClassEx(&wc))
        {
            LOG_ERROR("Failed to register window class '%s' (error %lu).", context->window_class, GetLastError());
            platform_window_shutdown();
            return false;
        }

        LOG_TRACE("Platform window initialized successfully with backend type: %d.", type);
        return true;
    }

    void platform_window_shutdown()
    {
        ASSERT(context != nullptr, "Platform window not initialized. Call platform_window_initialize() first.");

        // Уничтожение всех окон (пока только одно окно).
        if(context->window)
        {
            LOG_WARN("Window was not properly destroyed before shutdown.");
            platform_window_destroy(context->window);
            // NOTE: Осовобождение context->window в platform_window_destroy().
        }

        // Убираем регистрацию класса окна
        if(context->window_class)
        {
            UnregisterClass(context->window_class, context->hinstance);
        }

        mfree(context, context->memory_requirement, MEMORY_TAG_APPLICATION);
        context = nullptr;

        LOG_TRACE("Platform window shutdown completed.");
    }

    bool platform_window_is_initialized()
    {
        return context != nullptr;
    }

    bool platform_window_poll_events()
    {
        ASSERT(context != nullptr, "Platform window not initialized. Call platform_window_initialize() first.");

        MSG msg = {};
        while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
            {
                LOG_TRACE("Window close requested, stopping event loop.");
                return false;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return true;
    }

    platform_window* platform_window_create(const platform_window_config* config)
    {
        ASSERT(context != nullptr, "Platform window not initialized. Call platform_window_initialize() first.");
        ASSERT(config != nullptr, "Config pointer must be non-null.");

        LOG_TRACE("Creating window '%s' (size: %dx%d)...", config->title, config->width, config->height);

        if(context->window)
        {
            LOG_WARN("Window has already been created (only one window supported for now).");
            return nullptr;
        }

        context->window = mallocate(sizeof(platform_window), MEMORY_TAG_APPLICATION);
        mzero(context->window, sizeof(platform_window));

        context->window->title = string_duplicate(config->title);
        context->window->width = config->width;
        context->window->height = config->height;
        context->window->should_close = false;

        context->window->hwnd = CreateWindowEx(
            WS_EX_APPWINDOW, context->window_class, config->title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
            CW_USEDEFAULT, config->width, config->height, nullptr, nullptr, context->hinstance, nullptr
        );

        if(!context->window->hwnd)
        {
            LOG_ERROR("Failed to create window (error %lu).", GetLastError());
            platform_window_destroy(context->window);
            return nullptr;
        }

        // Сохранение указателя на структуру окна в userdata.
        // TODO: Использовать.
        SetWindowLongPtr(context->window->hwnd, GWLP_USERDATA, (LONG_PTR)context->window);

        // Отображение окна.
        ShowWindow(context->window->hwnd, SW_SHOW);
        UpdateWindow(context->window->hwnd);

        LOG_TRACE("Window '%s' created successfully.", context->window->title);

        return context->window;
    }

    void platform_window_destroy(platform_window* window)
    {
        ASSERT(context != nullptr, "Platform window not initialized. Call platform_window_initialize() first.");
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

        mfree(window, sizeof(platform_window), MEMORY_TAG_APPLICATION);
        context->window = nullptr;
        LOG_TRACE("Window destroy complete.");
    }

    LRESULT CALLBACK window_process(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch(msg)
        {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP:
                LOG_TRACE("Key event.");
                return 0;

            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
                LOG_TRACE("Button event.");
                return 0;

            case WM_MOUSEMOVE:
                // LOG_TRACE("Motion event.");
                return 0;

            case WM_SIZE:
                LOG_TRACE("Resize event for window '%s' to %ux%u.", context->window->title, context->window->width, context->window->height);
                return 0;

            case WM_DESTROY:
                LOG_TRACE("Close event for window '%s'.", context->window->title);
                PostQuitMessage(0);
                return 0;

            default:
                // LOG_TRACE("Unhandled event type: %u.", msg);
                break;
        }

        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

#endif
