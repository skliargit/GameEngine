#include "platform/console.h"

#ifdef PLATFORM_LINUX_FLAG

    #include "debug/assert.h"
    #include <stdio.h>

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

    bool platform_console_initialize()
    {
        ASSERT(initialized == false, "Console subsystem is already initialized.");

        initialized = true;
        return true;
    }

    void platform_console_shutdown()
    {
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
