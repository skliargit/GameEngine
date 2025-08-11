#include "platform/window.h"

#if PLATFORM_WINDOWS_FLAG

    #include "core/logger.h"

    #include <Windows.h>
    #include <stdlib.h>

    struct platform_window {
        // Window-specific.
        HWND hwnd;
        HINSTANCE hinstance;
        const char* title;
    };

    // Объявление обработчика событий.
    static LRESULT CALLBACK window_process(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

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

        u64 memory_requirement = sizeof(platform_window);
        // TODO: Обернуть!
        platform_window* window = malloc(memory_requirement);
        if(!window)
        {
            LOG_FATAL("%s: Failed to obtain memory for window instance.", __func__);
            return false;
        }
        memset(window, 0, memory_requirement);

        static const char* window_class = "PlatformWindowClass";
        window->hinstance = GetModuleHandle(nullptr);

        WNDCLASS wc = {};
        wc.lpfnWndProc   = window_process;
        wc.hInstance     = window->hinstance;
        wc.lpszClassName = window_class;
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

        if(!RegisterClass(&wc))
        {
            LOG_ERROR("%s: Failed to registration window class '%s' (error %lu).", __func__, window_class, GetLastError());
            free(window);
            return false;
        }

        window->hwnd = CreateWindowEx(
            0, window_class, config->title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, config->width, config->height,
            nullptr, nullptr, window->hinstance, window
        );
        if(!window->hwnd)
        {
            LOG_ERROR("%s: Failed to create window (error %lu).", __func__, GetLastError());
            free(window);
            return false;
        }

        // Показываем окно
        ShowWindow(window->hwnd, SW_SHOW);
        UpdateWindow(window->hwnd);

        *out_window = window;
        return false;
    }

    void platform_window_destroy(platform_window* window)
    {
        if(!window)
        {
            LOG_ERROR("%s requires a valid pointer to window.", __func__);
            return;
        }

        if(window->hwnd)
        {
            SetWindowLongPtr(window->hwnd, GWLP_USERDATA, (LONG_PTR)nullptr);
            DestroyWindow(window->hwnd);
        }

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

        MSG msg = {};
        while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
            {
                return false;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return true;
    }

    LRESULT CALLBACK window_process(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch(msg)
        {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP:
                LOG_TRACE("%s: Key event.", __func__);
                return 0;

            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
                LOG_TRACE("%s: Button event.", __func__);
                return 0;

            case WM_MOUSEMOVE:
                LOG_TRACE("%s: Motion event.", __func__);
                return 0;

            case WM_SIZE:
                LOG_TRACE("%s: Resize event.", __func__);
                return 0;

            case WM_DESTROY:
                LOG_TRACE("%s: Close event.", __func__);
                PostQuitMessage(0);
                return 0;

            default:
                LOG_TRACE("%s: Unsupported event: %u", __func__, msg);
        }

        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

#endif
