#include "platform/console.h"

#if PLATFORM_WINDOWS_FLAG

    #include <windows.h>

    // NOTE: Вывод на консоль в Windows очень дорогая операция, по моим замерам: ~500 мкс...1.5 мс
    //       в то время на Linux ~5-8 мкс. Замена с WriteConsoleA на fputs и отказ от цвеного вывода
    //       снизит нижний порог до ~350 мкс.

    static const WORD colors[CONSOLE_COLOR_COUNT] = {
        [CONSOLE_COLOR_DEFAULT] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
        [CONSOLE_COLOR_RED]     = FOREGROUND_RED,
        [CONSOLE_COLOR_ORANGE]  = FOREGROUND_RED | FOREGROUND_GREEN,
        [CONSOLE_COLOR_GREEN]   = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        [CONSOLE_COLOR_YELLOW]  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        [CONSOLE_COLOR_BLUE]    = FOREGROUND_BLUE | FOREGROUND_INTENSITY,
        [CONSOLE_COLOR_MAGENTA] = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
        [CONSOLE_COLOR_CYAN]    = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
        [CONSOLE_COLOR_WHITE]   = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
        [CONSOLE_COLOR_GRAY]    = FOREGROUND_INTENSITY
    };

    void platform_console_write(console_color color, const char* message)
    {
        static HANDLE stdout_handle = nullptr;

        // TODO: Потокобезопасно.
        if(!stdout_handle)
        {
            stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
        }

        if(!message) return;

        // Установка нового значения цветаы.
        SetConsoleTextAttribute(stdout_handle, colors[color]);

        // TODO: fputs!
        u64 length = strlen(message);
        DWORD written = 0;
        WriteConsoleA(stdout_handle, message, (DWORD)length, &written, nullptr);

        // Восстанавление старого значения цвета.
        SetConsoleTextAttribute(stdout_handle, colors[CONSOLE_COLOR_DEFAULT]);
    }

    void platform_console_write_error(console_color color, const char* message)
    {
        static HANDLE stderr_handle = nullptr;

        // TODO: Потокобезопасно.
        if(!stderr_handle)
        {
            stderr_handle = GetStdHandle(STD_ERROR_HANDLE);
        }

        if(!message) return;

        // Установка нового значения цветаы.
        SetConsoleTextAttribute(stderr_handle, colors[color]);

        // TODO: fputs!
        u64 length = strlen(message);
        DWORD written = 0;
        WriteConsoleA(stderr_handle, message, (DWORD)length, &written, nullptr);

        // Восстанавление старого значения цвета.
        SetConsoleTextAttribute(stderr_handle, colors[CONSOLE_COLOR_DEFAULT]);
    }

#endif
