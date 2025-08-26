#include "platform/console.h"

#ifdef PLATFORM_WINDOWS_FLAG

    #include "debug/assert.h"
    #include <windows.h>

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

    static HANDLE stdout_handle = nullptr;
    static HANDLE stderr_handle = nullptr;
    static bool initialized = false;

    bool platform_console_initialize()
    {
        if(initialized)
        {
            return true;
        }

        // TODO: Проверка есть ли у процесса консоль, и если нет, то вывод будет игнорироваться,
        //       т.к. приложение вероятно было запущено из проводника, а не из консоли.
        //       1) Определить откуда запущено приложение.
        //       2) Попытаться создать консоль для консольного запуска.
        //       3) Оставить без консоли, если проводник и просто инициализировать систему.
        if(GetConsoleWindow() == nullptr)
        {
            initialized = true;
            return true;
        }

        // Получение хэндлов стандартных потоков.
        stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
        stderr_handle = GetStdHandle(STD_ERROR_HANDLE);

        // Если оба хэндла невалидны, считаем что консоли нет.
        if(stdout_handle == nullptr || stderr_handle == nullptr)
        {
            return false;
        }

        initialized = true;
        return true;
    }

    void platform_console_shutdown()
    {
        stdout_handle = nullptr;
        stderr_handle = nullptr;
        initialized = false;
    }

    bool platform_console_is_initialized()
    {
        return initialized;
    }

    void platform_console_write(console_stream stream, console_color color, const char* message)
    {
        if(!initialized || !stdout_handle || !stderr_handle)
        {
            return;
        }

        // В отладочном режиме проверяется корректность ввода, это вполне легально, т.к.
        // потоки получены и если утверждения будут нарушены, то не вызовет циклическую зависимость.
        ASSERT(stream < CONSOLE_STREAM_COUNT, "Must be less than CONSOLE_STREAM_COUNT");
        ASSERT(color < CONSOLE_COLOR_COUNT, "Must be less than CONSOLE_COLOR_COUNT");
        ASSERT(message != nullptr, "Message pointer must be non-null.");

        HANDLE out = (stream == CONSOLE_STREAM_STDOUT ? stdout_handle : stderr_handle);

        // Установка нового значения цвета.
        SetConsoleTextAttribute(out, colors[color]);

        u64 length = strlen(message);
        DWORD written = 0;
        WriteConsoleA(out, message, (DWORD)length, &written, nullptr);

        // Восстанавление старого значения цвета.
        SetConsoleTextAttribute(out, colors[CONSOLE_COLOR_DEFAULT]);
    }

#endif
