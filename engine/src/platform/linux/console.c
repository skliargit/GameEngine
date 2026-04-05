#include "platform/console.h"

#ifdef PLATFORM_LINUX_FLAG

    #include "debug/assert.h"
    #include <stdio.h>
    #include <stdarg.h>
    #include <unistd.h>

    static const char* colors[CONSOLE_COLOR_COUNT] = {
        [CONSOLE_COLOR_DEFAULT] = "0",
        [CONSOLE_COLOR_RED]     = "0;38;5;196",
        [CONSOLE_COLOR_ORANGE]  = "0;38;5;208",
        [CONSOLE_COLOR_GREEN]   = "0;38;5;46",
        [CONSOLE_COLOR_YELLOW]  = "0;38;5;226",
        [CONSOLE_COLOR_BLUE]    = "0;38;5;33",
        [CONSOLE_COLOR_MAGENTA] = "0;38;5;201",
        [CONSOLE_COLOR_CYAN]    = "0;38;5;51",
        [CONSOLE_COLOR_WHITE]   = "0;38;5;15",
        [CONSOLE_COLOR_GRAY]    = "0;38;5;244",
    };

    static bool initialized = false;

    bool platform_console_initialize(void)
    {
        ASSERT(initialized == false, "Console subsystem is already initialized.");

        initialized = isatty(STDOUT_FILENO);
        return initialized;
    }

    void platform_console_shutdown(void)
    {
        initialized = false;
    }

    bool platform_console_is_initialized(void)
    {
        return initialized;
    }

    void platform_console_write(console_stream_t stream, console_color_t color, const char* message)
    {
        if(!initialized) return;

        ASSERT(stream < CONSOLE_STREAM_COUNT, "Must be less than CONSOLE_STREAM_COUNT");
        ASSERT(color < CONSOLE_COLOR_COUNT, "Must be less than CONSOLE_COLOR_COUNT");
        ASSERT(message != nullptr, "Message pointer must be non-null.");

        fprintf((stream == CONSOLE_STREAM_STDOUT ? stdout : stderr), "\033[%sm%s\033[0m", colors[color], message);
    }

    void platform_console_writef(console_stream_t stream, console_color_t color, const char* format, ...)
    {
        if(!initialized) return;

        ASSERT(stream < CONSOLE_STREAM_COUNT, "Must be less than CONSOLE_STREAM_COUNT");
        ASSERT(color < CONSOLE_COLOR_COUNT, "Must be less than CONSOLE_COLOR_COUNT");
        ASSERT(format != nullptr, "Message pointer must be non-null.");
        
        va_list args;
        va_start(args, format);

        fprintf((stream == CONSOLE_STREAM_STDOUT ? stdout : stderr), "\033[%sm", colors[color]);
        vfprintf((stream == CONSOLE_STREAM_STDOUT ? stdout : stderr), format, args);
        fprintf((stream == CONSOLE_STREAM_STDOUT ? stdout : stderr), "\033[0m");

        va_end(args);
    }

    void platform_console_writefv(console_stream_t stream, console_color_t color, const char* format, void* args)
    {
        if(!initialized) return;

        ASSERT(stream < CONSOLE_STREAM_COUNT, "Must be less than CONSOLE_STREAM_COUNT");
        ASSERT(color < CONSOLE_COLOR_COUNT, "Must be less than CONSOLE_COLOR_COUNT");
        ASSERT(format != nullptr, "Message pointer must be non-null.");
        ASSERT(args != nullptr, "Variadic arguments pointer must be non-null.");

        // NOTE: Параметры args могут использоваться только для одного вызова vsnprintf, для повторного использования
        //       необходима копия va_list или последовательность va_end -> va_start -> vsnprintf -> va_end.
        va_list copy;
        va_copy(copy, args);

        fprintf((stream == CONSOLE_STREAM_STDOUT ? stdout : stderr), "\033[%sm", colors[color]);
        vfprintf((stream == CONSOLE_STREAM_STDOUT ? stdout : stderr), format, copy);
        fprintf((stream == CONSOLE_STREAM_STDOUT ? stdout : stderr), "\033[0m");

        va_end(copy);
    }

#endif
