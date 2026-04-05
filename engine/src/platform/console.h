/*
    @file console.h
    @brief Кросс-платформенный интерфейс для работы с консольным выводом.
    @author Дмитрий Скляр.
    @version 1.2
    @date 25-02-2026

    @license Лицензия Apache, версия 2.0 («Лицензия»);
          Вы не имеете права использовать этот файл без соблюдения условий Лицензии.
          Копию Лицензии можно получить по адресу http://www.apache.org/licenses/LICENSE-2.0
          Если иное не предусмотрено действующим законодательством или не согласовано в письменной форме,
          программное обеспечение, распространяемое по Лицензии, распространяется на условиях «КАК ЕСТЬ»,
          БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ ИЛИ УСЛОВИЙ, явных или подразумеваемых. См. Лицензию для получения
          информации о конкретных языках, регулирующих разрешения и ограничения по Лицензии.

    @note Реализации функций являются платформозависимыми и находятся в соответствующих
          platform/ модулях (windows, linux и т.д.).

    @note Для корректной работы необходимо предварительно инициализировать:
            - Подсистему консоли platform_console_initialize()
*/

#pragma once

#include <core/defines.h>

/**
    @brief Доступные потоки для консольного вывода.
*/
typedef enum console_stream {
    CONSOLE_STREAM_STDOUT,    /**< Стандартный поток вывода (stdout).                 */
    CONSOLE_STREAM_STDERR,    /**< Стандартный поток ошибок (stderr).                 */
    CONSOLE_STREAM_COUNT      /**< Количество потоков (не является реальным потоком). */
} console_stream_t;

/**
    @brief Доступные цвета текста для консольного вывода.
*/
typedef enum console_color {
    CONSOLE_COLOR_DEFAULT,    /**< Цвет по умолчанию.                                 */
    CONSOLE_COLOR_RED,        /**< Красный цвет.                                      */
    CONSOLE_COLOR_ORANGE,     /**< Оранжевый цвет.                                    */
    CONSOLE_COLOR_GREEN,      /**< Зеленый цвет.                                      */
    CONSOLE_COLOR_YELLOW,     /**< Желтый цвет.                                       */
    CONSOLE_COLOR_BLUE,       /**< Синий цвет.                                        */
    CONSOLE_COLOR_MAGENTA,    /**< Пурпурный цвет.                                    */
    CONSOLE_COLOR_CYAN,       /**< Голубой цвет.                                      */
    CONSOLE_COLOR_WHITE,      /**< Белый цвет.                                        */
    CONSOLE_COLOR_GRAY,       /**< Серый цвет.                                        */
    CONSOLE_COLOR_COUNT       /**< Количество цветов (не является реальным цветом).   */
} console_color_t;

/**
    @brief Инициализирует подсистему для работы с консолью.
    @return true - инициализация успешна, false - произошла ошибка или консоль недоступна.

    @warning Не thread-safe. Должна вызываться из основного потока.
    @note Должна быть вызвана один раз при старте приложения.
*/
bool platform_console_initialize(void);

/**
    @brief Завершает работу подсистемы для работы с консолью.

    @warning Не thread-safe. Должна вызываться из основного потока.
    @note Должна быть вызвана при завершении приложения.
*/
void platform_console_shutdown(void);

/**
    @brief Проверяет, была ли инициализирована подсистема работы с консолью.
    @return true - подсистема инициализирована и готова к работе, false - подсистема не инициализирована.

    @warning Не thread-safe. Должна вызываться из того же потока, что и инициализация/завершение.
    @note Может использоваться для проверки состояния подсистемы перед вызовом других функций.
*/
CORE_API bool platform_console_is_initialized(void);

/**
    @brief Выводит сообщение в заданный стандартный поток.
    @param stream Тип стандартного потока.
    @param color Цвет сообщения.
    @param message Строка или указатель на строку сообщения, не может быть nullptr.

    @warning Не thread-safe. Клиентский код должен обеспечить синхронизацию при использовании из нескольких потоков.
*/
CORE_API void platform_console_write(console_stream_t stream, console_color_t color, const char* message);

/**
    @brief Выводит форматируемое сообщение со списком аргументов в заданный стандартный поток.
    @param stream Тип стандартного потока.
    @param color Цвет сообщения.
    @param format Строка или указатель на строку форматируемого сообшения, не может быть nullptr.
    @param ... Список аргументов форматируемого сообщения.

    @warning Не thread-safe. Клиентский код должен обеспечить синхронизацию при использовании из нескольких потоков.
*/
CORE_API void platform_console_writef(console_stream_t stream, console_color_t color, const char* format, ...);

/**
    @brief Выводит форматируемое сообщение с указателем на список аргументов в заданный стандартный поток.
    @param stream Тип стандартного потока.
    @param color Цвет сообщения.
    @param format Строка или указатель на строку форматируемого сообшения, не может быть nullptr.
    @param args Указатель на список аргументов форматируемого сообщения, не может быть nullptr.

    @warning Не thread-safe. Клиентский код должен обеспечить синхронизацию при использовании из нескольких потоков.
*/
CORE_API void platform_console_writefv(console_stream_t stream, console_color_t color, const char* format, void* args);
