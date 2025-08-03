#pragma once

#include <core/defines.h>

/*
    @file console.h
    @brief Интерфейс для работы с консольным вводом-выводом с поддержкой цветов.

    Предоставляет функции для вывода цветного текста в стандартные потоки вывода и ошибок.
*/

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

/*
    @brief Выводит сообщение в стандартный поток вывода.
    @param color Цвет которым будет выведено сообщение.
    @param message Сообщение которое будет напечатано.
*/
API void platform_console_write(console_color color, const char* message);

/*
    @brief Выводит сообщение в стандартный поток ошибок.
    @param color Цвет которым будет выведено сообщение.
    @param message Сообщение которое будет напечатано.
*/
API void platform_console_write_error(console_color color, const char* message);
