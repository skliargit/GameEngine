#include "core/logger.h"
#include "core/string.h"
#include "core/memory.h"
#include "debug/assert.h"
#include "platform/console.h"
#include "platform/time.h"

#include <stdarg.h>

// TODO: Потокобезопасность!

// Размер стекового буфера.
#define BUFFER_SIZE 1024

typedef struct log_system_context {
    log_level_t level;
    log_handler_callback handler;
    const void* user_data;
} log_system_context;

// Объявление функции-обработчика по умолчанию.
static void log_default_handler(const log_message_t* message);

// Контекст системы логгирования с предустановками.
static log_system_context context = {
#ifdef DEBUG_FLAG
    .level = LOG_LEVEL_TRACE,
    .handler = log_default_handler,
    .user_data = nullptr
#else
    .level = LOG_LEVEL_ERROR,
    .handler = log_default_handler,
    .user_data = nullptr
#endif
};

void log_set_level(log_level_t level)
{
    ASSERT(level < LOG_LEVEL_COUNT, "Must be less than LOG_LEVEL_COUNT.");

    context.level = level;
}

void log_set_handler(log_handler_callback handler, const void* user_data)
{
    context.handler = handler;
    context.user_data = user_data;
}

void log_reset_default_handler()
{
    context.handler = log_default_handler;
    context.user_data = nullptr;
}

void log_write(log_level_t level, const char* file, u32 line, const char* format, ...)
{
    ASSERT(level < LOG_LEVEL_COUNT, "Must be less than LOG_LEVEL_COUNT.");
    ASSERT(file != nullptr, "File name must be provided.");
    ASSERT(format != nullptr, "Format string must be non-null.");

    // Проверка наличия функции обработчика сообщений.
    if(context.handler && level <= context.level)
    {
        // Внутренний буфер (стековый для потоко-безопасности).
        char internal_buffer[BUFFER_SIZE];
        void* buffer = internal_buffer;

        // NOTE: Особенность определения va_list для Linux и Windows.
        __builtin_va_list args;
        va_start(args, format);

        // Получение размера строки.
        i32 buffer_length = string_format_va(nullptr, 0, format, args) + 1;
        if(buffer_length > BUFFER_SIZE)
        {
            // TODO: Использовать линейный распределитель или пул памяти.
            buffer = mallocate(buffer_length, MEMORY_TAG_STRING);
        }

        // Форматирование сообщения.
        string_format_va(buffer, buffer_length, format, args);

        va_end(args);

        log_message_t msg = {
            .filename        = file,
            .filename_length = string_length(file),
            .fileline        = line,
            .level           = level,
            .message         = buffer,
            .message_length  = buffer_length,
            .timestamp       = platform_time_now(),
            .user_data       = context.user_data
        };

        context.handler(&msg);

        if(buffer_length > BUFFER_SIZE)
        {
            // TODO: Использовать линейный распределитель или пул памяти.
            mfree(buffer, buffer_length, MEMORY_TAG_STRING);
        }
    }

    if(level == LOG_LEVEL_FATAL)
    {
        DEBUG_BREAK();
    }
}

void log_default_handler(const log_message_t* message)
{
    // Текстовые метки сообщений в соответствии с уровнем по умолчанию.
    static const char* levels[LOG_LEVEL_COUNT] = {
        [LOG_LEVEL_FATAL] = "FATAL", [LOG_LEVEL_ERROR] = "ERROR", [LOG_LEVEL_WARN]  = "WARNG",
        [LOG_LEVEL_INFO]  = "INFOR", [LOG_LEVEL_DEBUG] = "DEBUG", [LOG_LEVEL_TRACE] = "TRACE"
    };

    // Цвета сообщений в соответствии с уровнем по умолчанию.
    static const console_color_t colors[LOG_LEVEL_COUNT] = {
        [LOG_LEVEL_FATAL] = CONSOLE_COLOR_MAGENTA, [LOG_LEVEL_ERROR] = CONSOLE_COLOR_RED,
        [LOG_LEVEL_WARN]  = CONSOLE_COLOR_ORANGE,  [LOG_LEVEL_INFO]  = CONSOLE_COLOR_GREEN,
        [LOG_LEVEL_DEBUG] = CONSOLE_COLOR_BLUE,    [LOG_LEVEL_TRACE] = CONSOLE_COLOR_GRAY
    };

    // Формат сообщения по умолчанию.
    static const char* format_message = "%hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu %s (%s:%-3u): %s\n";

    // Кешированная дата.
    static platform_datetime dt;
    static u64 ts = 0;

    // Внутренний буфер (стековый для потоко-безопасности).
    char internal_buffer[BUFFER_SIZE];
    void* buffer = internal_buffer;

    // Получение размера буфера сообщения.
    u32 buffer_length = message->filename_length + message->message_length + 60;
    if(buffer_length > BUFFER_SIZE)
    {
        // TODO: Использовать линейный распределитель или пул памяти.
        buffer = mallocate(buffer_length, MEMORY_TAG_STRING);
    }

    if(ts < message->timestamp)
    {
        ts = message->timestamp;
        dt = platform_time_to_local(message->timestamp);
    }

    // Копирование сообщения в буфер.
    string_format(buffer, buffer_length, format_message, dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second,
        levels[message->level], message->filename, message->fileline, message->message
    );

    if(message->level <= LOG_LEVEL_ERROR)
    {
        platform_console_write(CONSOLE_STREAM_STDERR, colors[message->level], buffer);
    }
    else
    {
        platform_console_write(CONSOLE_STREAM_STDOUT, colors[message->level], buffer);
    }

    if(buffer_length > BUFFER_SIZE)
    {
        // TODO: Использовать линейный распределитель или пул памяти.
        mfree(buffer, buffer_length, MEMORY_TAG_STRING);
    }
}
