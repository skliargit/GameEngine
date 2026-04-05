#include "debug/assert.h"

#include "platform/console.h"
#include <stdarg.h>
#include <stdlib.h>

const static console_color_t color = CONSOLE_COLOR_MAGENTA;

void assert(const char* expr, const char* file, u32 line, const char* func, const char* format, ...)
{
    // Вывод заголовока утверждения.
    platform_console_write(CONSOLE_STREAM_STDERR, color,
        "\n==================================== ASSERTION FAILED ====================================\n"
    );

    // Вывод информации о месте.
    platform_console_writef(CONSOLE_STREAM_STDERR, color,
        "File       : %s\nLine       : %u\nFunction   : %s\nExpression : %s\n", file, line, func, expr
    );

    // Вывод форматируемого сообщения, если есть.
    if(format)
    {
        va_list args;
        va_start(args, format);

        platform_console_write(CONSOLE_STREAM_STDERR, color, "Message    : ");
        platform_console_writefv(CONSOLE_STREAM_STDERR, color, format, args);
        platform_console_write(CONSOLE_STREAM_STDERR, color, "\n");

        va_end(args);
    }

    platform_console_write(CONSOLE_STREAM_STDERR, color,
        "==========================================================================================\n\n"
    );

    // Останавливка программы в отладчике.
    DEBUG_BREAK();

    // Аварийное завершение, если не сработал останов.
    abort();
}
