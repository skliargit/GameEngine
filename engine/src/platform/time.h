#pragma once

#include <core/defines.h>

// @brief Срез времени.
typedef struct datetime {
    // @brief Год (обычно 1970 - 2100).
    u16 year;
    // @brief Месяц (обычно 1-12).
    u8 month;
    // @brief День (обычно 1-31).
    u8 day;
    // @brief Часы (обычно от 0 до 23).
    u8 hour;
    // @brief Минуты (обычно от 0 до 59).
    u8 minute;
    // @brief Секунды (обычно от 0 до 59).
    u8 second;
} datetime;

/*
    @brief Получение времени в секундах с 1970-01-01.
    @return Количество секунд с 1970-01-01.
*/
API u64 platform_time_now();

/*
    @brief Получение даты и времени в локальном часовом поясе.
    @note На некоторых системах может быть не потокобезопасной.
    @param time_sec Количество секунд с 1970-01-01.
    @return Заполненная структура с датой и временем.
*/
API datetime platform_time_to_local(u64 time_sec);

/*
    @brief Получение даты и времени в utc часовом поясе.
    @note На некоторых системах может быть не потокобезопасной.
    @param time_sec Количество секунд с 1970-01-01.
    @return Заполненная структура с датой и временем.
*/
API datetime platform_time_to_utc(u64 time_sec);

/*
    @brief Преобразует структуру с датой и временем во время в секундах с 1970-01-01.
    @note Функция автоматически корректирует недопустимые даты.
    @param dt Указатель на структуру с датой и временем.
    @return Количество секунд с 1970-01-01.
*/
API u64 platform_time_from_datetime(const datetime* dt);
