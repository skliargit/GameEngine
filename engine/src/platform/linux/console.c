#include "platform/console.h"

#if PLATFORM_LINUX_FLAG

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

    static const char* format_str = "\033[%sm%s\033[0m";

    void platform_console_write(console_color color, const char* message)
    {
        if(!message) return;

        // TODO: Сделать потокобезопасной (flockfile(stdout) и funlockfile(stdout)).
        fprintf(stdout, format_str, colors[color], message);
    }

    void platform_console_write_error(console_color color, const char* message)
    {
        if(!message) return;

        // TODO: Сделать потокобезопасной (flockfile(stderr) и funlockfile(stderr)).
        fprintf(stderr, format_str, colors[color], message);
    }

#endif
