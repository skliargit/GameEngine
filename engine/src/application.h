/*
    @file application.h
    @brief Интерфейс управления основным циклом приложения и жизненным циклом движка.
    @author Дмитрий Скляр.
    @version 1.0
    @date 11-09-2025

    @license Лицензия Apache, версия 2.0 («Лицензия»);
          Вы не имеете права использовать этот файл без соблюдения условий Лицензии.
          Копию Лицензии можно получить по адресу http://www.apache.org/licenses/LICENSE-2.0
          Если иное не предусмотрено действующим законодательством или не согласовано в письменной форме,
          программное обеспечение, распространяемое по Лицензии, распространяется на условиях «КАК ЕСТЬ»,
          БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ ИЛИ УСЛОВИЙ, явных или подразумеваемых. См. Лицензию для получения
          информации о конкретных языках, регулирующих разрешения и ограничения по Лицензии.

    @note Система предоставляет:
            - Управление главным циклом приложения
            - Конфигурируемую систему callback-ов для жизненного цикла
            - Статистику производительности в реальном времени
            - Управление ограничением FPS
            - Обработку оконных событий и ввода
*/

#pragma once

#include <core/defines.h>
#include <platform/window.h>
#include <renderer/renderer.h>

typedef struct application_config application_config;

/*
    @brief Callback-функция инициализации приложения.
    @param config Указатель на конфигурацию приложения.
    @returns true если инициализация прошла успешно, иначе false.
*/
typedef bool (*application_initialize_callback)(const application_config* config);

/*
    @brief Callback-функция завершения работы проложения.
*/
typedef void (*application_shutdown_callback)();

/*
    @brief Callback-функция обработки изменения размеров окна.
    @param width Новая ширина области отрисовки в пикселях.
    @param height Новая высота области отрисовки в пикселях.
*/
typedef void (*application_resize_callback)(u32 width, u32 height);

/*
    @brief Callback-функция обновления логики приложения.
    @param delta_time Время, прошедшее с предыдущего кадра, в секундах.
    @returns true чтобы продолжить выполнение, false для запроса выхода.
*/
typedef bool (*application_update_callback)(f32 delta_time);

/*
    @brief Callback-функция отрисовки кадра.
    @param delta_time Время, прошедшее с предыдущего кадра, в секундах.
    @returns true если рендеринг успешен, false в случае ошибки.
*/
typedef bool (*application_render_callback)(f32 delta_time);

// @brief Статистика производительности за кадр.
typedef struct application_frame_stats {
    // @brief Время обработки оконных событий и ввода в секундах.
    f64 window_time;
    // @brief Время обновления логики приложения в секундах.
    f64 update_time;
    // @brief Время отрисовки кадра в секундах.
    f64 render_time;
    // @brief Общее время выполнения кадра в секундах.
    f64 frame_time;
    // @brief Планируемое время сна для соблюдения target_fps в секундах.
    f64 sleep_time;
    // @brief Реальное время сна в секундах.
    f64 sleep_actual_time;
    // @brief Ошибка планирования сна в секундах (+ пересып / - недосып).
    f64 sleep_error_time;
    // @brief Текущее количество кадров в секунду.
    u16 fps;
    // TODO: Среднее количество кадров в секунду за последние 60 кадров.
    u16 fps_avg;
    // TODO: Минимальное количество кадров в секунду за период измерения.
    u16 fps_min;
    // TODO: Максимальное количество кадров в секунду за период измерения.
    u16 fps_max;
} application_frame_stats;

// @brief Конфигурация для создания и настройки приложения.
typedef struct application_config {
    // @brief Начальная конфигурация окна приложения.
    struct {
        // @brief Тип бэкенда оконной системы.
        platform_window_backend_type backend_type;
        // @brief Заголовок окна.
        const char* title;
        // @brief Начальная ширина окна в пикселях.
        u32 width;
        // @brief Начальная высота окна в пикселях.
        u32 height;
    } window;

    // @brief Настройки производительности приложения.
    struct {
        // @brief Целевое количество кадров в секунду (0 для неограниченного).
        u16 target_fps;
    } performance;

    // @brief Callback-функция, вызываемая при инициализации приложения.
    application_initialize_callback initialize;
    // @brief Callback-функция, вызываемая при завершении работы проложения.
    application_shutdown_callback shutdown;
    // @brief Callback-функция, вызываемая при изменении размеров окна.
    application_resize_callback on_resize; // TODO: здесь unsigned, в а window.c i32 исправить!
    // @brief Callback-функиця, вызываемая при обновлении логики приложения.
    application_update_callback update;
    // @brief Callback-функция, вызываемая при отрисовке кадра.
    application_render_callback render;
} application_config;

/*
    @brief Инициализирует приложение с указанной конфигурацией.
    @note Функция должна быть вызвана перед application_run().
    @param config Указатель на структуру конфигурации приложения.
    @return true если приложение успешно создано, иначе false.
*/
API bool application_initialize(const application_config* config);

/*
    @brief Запускает главный цикл приложения.
    @note Функция блокирует выполнение до завершения работы приложения.
    @return true если выполнение завершено успешно, false в случае ошибки.
*/
API bool application_run();

/*
    @brief Запрашивает корректное завершение работы приложения.
*/
API void application_quit();

/*
    @brief Немедленно останавливает работу приложения и освобождает все возможные ресурсы.
    @warning Используйте эту функцию только в исключительных ситуациях (критические ошибки).
*/
API void application_terminate();

/*
    @brief Возвращает копию структуры со статистикой производительности за последний кадр.
    @return Структура application_frame_stats, содержащая актуальные метрики производительности.
*/
API application_frame_stats application_get_frame_stats();
