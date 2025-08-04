#include "core/logger.h"
#include "core/defines.h"

#include "platform/console.h"
#include "platform/string.h"
#include "platform/time.h"

#include <stdarg.h>

// TODO: Обернуть.
#include <stdlib.h>

// Размер стекового буфера.
#define BUFFER_SIZE 2048

// Объявление функции-обработчика по умолчанию.
static void log_default_handler(const log_message message, const void* user_data);

// TODO: Потокобезопасность.
// Переменные со значениями по умолчанию.
static log_handler_pfn current_handler = log_default_handler;
static log_level current_level = LOG_LEVEL_ERROR;
static const void* current_user_data = nullptr;

void log_set_level(log_level level)
{
    if(level >= LOG_LEVEL_COUNT) return;
    // TODO: Потокобезопасность.
    current_level = level;
}

void log_set_handler(log_handler_pfn handler, const void* user_data)
{
    // TODO: Потокобезопасность.
    current_handler = handler;
    current_user_data = user_data;
}

void log_reset_default_handler()
{
    // TODO: Потокобезопасность.
    current_handler = log_default_handler;
    current_user_data = nullptr;
}

void log_write(log_level level, const char* filename, u32 fileline, const char* format, ...)
{
    if(level >= LOG_LEVEL_COUNT || !filename || !format) return;

    // TODO: Потокобезопасность.
    // Проверка наличия функции обработчика сообщений.
    if(current_handler && level <= current_level)
    {
        // Внутренний буфер (стековый для потоко-безопасности).
        char internal_buffer[BUFFER_SIZE];
        void* buffer = internal_buffer;

        // TODO: Особенности определения для Linux и Windows.
        __builtin_va_list args;
        va_start(args, format);

        // Получение размера строки.
        i32 buffer_length = platform_string_format_length(format, args) + 1;
        if(buffer_length > BUFFER_SIZE)
        {
            // TODO: Использовать линейный распределитель или пул памяти.
            buffer = malloc(sizeof(char) * buffer_length);
        }

        // Форматирование сообщения.
        platform_string_format_va(buffer, buffer_length, format, args);
        va_end(args);

        log_message msg;
        msg.filename        = filename;
        msg.filename_length = platform_string_length(filename);
        msg.fileline        = fileline;
        msg.level           = level;
        msg.message         = buffer;
        msg.message_length  = buffer_length;
        msg.timestamp       = platform_time_now();

        current_handler(msg, current_user_data);

        if(buffer_length > BUFFER_SIZE)
        {
            // TODO: Использовать линейный распределитель или пул памяти.
            free(buffer);
        }
    }

    if(level == LOG_LEVEL_FATAL)
    {
        // TODO: Остановка программы!
    }
}

void log_default_handler(const log_message message, const void* user_data)
{
    // Текстовые метки сообщений в соответствии с уровнем по умолчанию.
    static const char* levels[LOG_LEVEL_COUNT] = {
        [LOG_LEVEL_FATAL] = "FATAL", [LOG_LEVEL_ERROR] = "ERROR", [LOG_LEVEL_WARN]  = "WARNG",
        [LOG_LEVEL_INFO]  = "INFOR", [LOG_LEVEL_DEBUG] = "DEBUG", [LOG_LEVEL_TRACE] = "TRACE"
    };

    // Цвета сообщений в соответствии с уровнем по умолчанию.
    static const console_color colors[LOG_LEVEL_COUNT] = {
        [LOG_LEVEL_FATAL] = CONSOLE_COLOR_MAGENTA, [LOG_LEVEL_ERROR] = CONSOLE_COLOR_RED,
        [LOG_LEVEL_WARN]  = CONSOLE_COLOR_ORANGE,  [LOG_LEVEL_INFO]  = CONSOLE_COLOR_GREEN,
        [LOG_LEVEL_DEBUG] = CONSOLE_COLOR_BLUE,    [LOG_LEVEL_TRACE] = CONSOLE_COLOR_WHITE
    };

    // Формат сообщения по умолчанию.
    static const char* format_message = "%hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu %s (%s:%u): %s\n";

    // Внутренний буфер (стековый для потоко-безопасности).
    char internal_buffer[BUFFER_SIZE];
    void* buffer = internal_buffer;

    // Получение размера буфера сообщения.
    u32 buffer_length = message.filename_length + message.message_length + 60;
    if(buffer_length > BUFFER_SIZE)
    {
        // TODO: Использовать линейный распределитель или пул памяти.
        buffer = malloc(sizeof(char) * buffer_length);
    }

    datetime dt = platform_time_to_local(message.timestamp);

    // Копирование сообщения в буфер.
    platform_string_format(buffer, buffer_length, format_message, dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second,
        levels[message.level], message.filename, message.fileline, message.message
    );

    if(message.level <= LOG_LEVEL_ERROR)
    {
        platform_console_write_error(colors[message.level], buffer);
    }
    else
    {
        platform_console_write(colors[message.level], buffer);
    }

    if(buffer_length > BUFFER_SIZE)
    {
        // TODO: Использовать линейный распределитель или пул памяти.
        free(buffer);
    }

    // NOTE: Не используется для функции, но компилятор ругается, а потому заглушка.
    (void)user_data;
}
