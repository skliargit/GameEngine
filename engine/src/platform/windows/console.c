#include "platform/console.h"

#ifdef PLATFORM_WINDOWS_FLAG

    #include "debug/assert.h"
    #include <windows.h>
    #include <stdio.h>

    static const char* colors[CONSOLE_COLOR_COUNT] = {
        [CONSOLE_COLOR_DEFAULT] = "0",
        [CONSOLE_COLOR_RED]     = "38;5;196",
        [CONSOLE_COLOR_ORANGE]  = "38;5;208",
        [CONSOLE_COLOR_GREEN]   = "38;5;46",
        [CONSOLE_COLOR_YELLOW]  = "38;5;226",
        [CONSOLE_COLOR_BLUE]    = "38;5;33",
        [CONSOLE_COLOR_MAGENTA] = "38;5;201",
        [CONSOLE_COLOR_CYAN]    = "38;5;51",
        [CONSOLE_COLOR_WHITE]   = "38;5;15",
        [CONSOLE_COLOR_GRAY]    = "38;5;244",
    };

    static bool initialized = false;
    static bool has_attached = false;

    bool platform_console_initialize()
    {
        ASSERT(initialized == false, "Console subsystem is already initialized.");

        // Попытка присоединиться к консоли.
        has_attached = AttachConsole(ATTACH_PARENT_PROCESS);
        if (!has_attached)
        {
            return false;
        }

        // Попытка переопределить потоки вывода и ошибок в присоедененную консоль.
        FILE* old_stdout;
        FILE* old_stderr;

        if(freopen_s(&old_stdout, "CONOUT$", "w", stdout) != 0)
        {
            platform_console_shutdown();
            return false;
        }

        if(freopen_s(&old_stderr, "CONOUT$", "w", stderr) != 0)
        {
            platform_console_shutdown();
            return false;
        }

        // Безопасное закрытие старых потоков.
        if(old_stdout != stdout)
        {
            fclose(old_stdout);
        }

        if(old_stderr != stderr)
        {
            fclose(old_stderr);
        }

        fprintf(stdout, "\n");
        initialized = true;
        return true;
    }

    void platform_console_shutdown()
    {
        FILE* old_stdout;
        FILE* old_stderr;

        // Восстанавление потоков в NUL.
        freopen_s(&old_stdout, "NUL", "w", stdout);
        freopen_s(&old_stderr, "NUL", "w", stderr);

        // Безопасное закрытие старых потоков.
        if(old_stdout != stdout)
        {
            fclose(old_stdout);
        }

        if(old_stderr != stderr)
        {
            fclose(old_stderr);
        }

        if(has_attached)
        {
            FreeConsole();
            has_attached = false;
        }

        initialized = false;
    }

    bool platform_console_is_initialized()
    {
        return initialized;
    }

    void platform_console_write(console_stream stream, console_color color, const char* message)
    {
        if(!initialized)
        {
            return;
        }

        ASSERT(stream < CONSOLE_STREAM_COUNT, "Must be less than CONSOLE_STREAM_COUNT");
        ASSERT(color < CONSOLE_COLOR_COUNT, "Must be less than CONSOLE_COLOR_COUNT");
        ASSERT(message != nullptr, "Message pointer must be non-null.");

        fprintf((stream == CONSOLE_STREAM_STDOUT ? stdout : stderr), "\033[%sm%s\033[0m", colors[color], message);
    }

#endif
