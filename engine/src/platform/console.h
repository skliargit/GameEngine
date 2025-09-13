/*
    @file console.h
    @brief Кросс-платформенный интерфейс для работы с консольным выводом.
    @author Дмитрий Скляр.
    @version 1.1
    @date 12-09-2025

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

// @brief Доступные цвета текста для консольного вывода.
typedef enum console_color {
    // @brief Цвет по умолчанию.
    CONSOLE_COLOR_DEFAULT,
    // @brief Красный цвет.
    CONSOLE_COLOR_RED,
    // @brief Оранжевый цвет.
    CONSOLE_COLOR_ORANGE,
    // @brief Зеленый цвет.
    CONSOLE_COLOR_GREEN,
    // @brief Желтый цвет.
    CONSOLE_COLOR_YELLOW,
    // @brief Синий цвет.
    CONSOLE_COLOR_BLUE,
    // @brief Пурпурный цвет.
    CONSOLE_COLOR_MAGENTA,
    // @brief Голубой цвет.
    CONSOLE_COLOR_CYAN,
    // @brief Белый цвет.
    CONSOLE_COLOR_WHITE,
    // @brief Серый цвет.
    CONSOLE_COLOR_GRAY,
    // @brief Максимальное количество цветов (не является реальным цветом).
    CONSOLE_COLOR_COUNT
} console_color;

// @brief Доступные потоки для консольного вывода.
typedef enum console_stream {
    // @brief Стандартный поток вывода (stdout).
    CONSOLE_STREAM_STDOUT,
    // @brief Стандартный поток ошибок (stderr).
    CONSOLE_STREAM_STDERR,
    // @brief Максимальное количество потоков (не является реальным потоком).
    CONSOLE_STREAM_COUNT
} console_stream;

/*
    @brief Инициализирует подсистему для работы с консолью.
    @note Должна быть вызвана один раз при старте приложения.
    @warning Не thread-safe. Должна вызываться из основного потока.
    @return true - инициализация успешна, false - произошла ошибка или консоль недоступна.
*/
API bool platform_console_initialize();

/*
    @brief Завершает работу подсистемы для работы с консолью.
    @note Должна быть вызвана при завершении приложения.
    @warning Не thread-safe. Должна вызываться из основного потока.
*/
API void platform_console_shutdown();

/*
    @brief Проверяет, была ли инициализирована подсистема работы с консолью.
    @note Может использоваться для проверки состояния подсистемы перед вызовом других функций.
    @warning Не thread-safe. Должна вызываться из того же потока, что и инициализация/завершение.
    @return true - подсистема инициализирована и готова к работе, false - подсистема не инициализирована.
*/
API bool platform_console_is_initialized();

/*
    @brief Выводит сообщение в заданный стандартный поток с заданным цветом.
    @warning Не thread-safe. Клиентский код должен обеспечить синхронизацию при использовании из нескольких потоков.
    @param stream Поток вывода (stdout/stderr).
    @param color Цвет которым будет выведено сообщение.
    @param message Сообщение которое будет напечатано.
*/
API void platform_console_write(console_stream stream, console_color color, const char* message);

/*
    @brief Выводит сообщение в стандартный поток вывода c заданным цветом.
    @warning Не thread-safe. Клиентский код должен обеспечить синхронизацию при использовании из нескольких потоков.
    @param color Цвет которым будет выведено сообщение.
    @param message Сообщение которое будет напечатано.
*/
#define platform_console_write_stdout(color, message) platform_console_write(CONSOLE_STREAM_STDOUT, color, message)

/*
    @brief Выводит сообщение в стандартный поток ошибок c заданным цветом.
    @warning Не thread-safe. Клиентский код должен обеспечить синхронизацию при использовании из нескольких потоков.
    @param color Цвет которым будет выведено сообщение.
    @param message Сообщение которое будет напечатано.
*/
#define platform_console_write_stderr(color, message) platform_console_write(CONSOLE_STREAM_STDERR, color, message)
