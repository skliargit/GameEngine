/*
    @file window.h
    @brief Кросс-платформенный интерфейс для работы с окнами.
    @author Дмитрий Скляр.
    @version 1.0
    @date 22-08-2025

    @license Лицензия Apache, версия 2.0 («Лицензия»);
          Вы не имеете права использовать этот файл без соблюдения условий Лицензии.
          Копию Лицензии можно получить по адресу http://www.apache.org/licenses/LICENSE-2.0
          Если иное не предусмотрено действующим законодательством или не согласовано в письменной форме,
          программное обеспечение, распространяемое по Лицензии, распространяется на условиях «КАК ЕСТЬ»,
          БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ ИЛИ УСЛОВИЙ, явных или подразумеваемых. См. Лицензию для получения
          информации о конкретных языках, регулирующих разрешения и ограничения по Лицензии.

    @note Реализации функций являются платформозависимыми и находятся в соответствующих
          platform/ модулях (windows, linux и т.д.).

    @note Для корректной работы необходимо предварительно инициализировать (в указанном порядке):
            - Подсистему консоли platform_console_initialize()
            - Подсистему памяти platform_memory_initialize()
            - Систему менеджмента и контроля памяти memory_system_initialize() из core/memory.h
            - Подсистему окон platform_window_initialize()
*/

#pragma once

#include <core/defines.h>

// @brief Контекст окна (представляет окно платформы).
typedef struct platform_window platform_window;

// @brief Тип оконной системы/бэкенда.
typedef enum platform_window_backend_type {
    // @brief Автоматический выбор оконной системы (по умолчанию).
    PLATFORM_WINDOW_BACKEND_TYPE_AUTO,
    // @brief Wayland - современный протокол отображения Linux.
    PLATFORM_WINDOW_BACKEND_TYPE_WAYLAND,
    // @brief XCB (X11/Xorg) - традиционная оконная система Linux.
    PLATFORM_WINDOW_BACKEND_TYPE_XCB,
    // @brief Classic Window Manager - оконная система Windows.
    PLATFORM_WINDOW_BACKEND_TYPE_WIN32,
    // @brief Количество доступных типов оконных систем (не является реальным типом).
    PLATFORM_WINDOW_BACKEND_TYPE_COUNT
} platform_window_backend_type;

// @brief Конфигурация для создания нового окна.
typedef struct platform_window_config {
    // @brief Заголовок окна.
    const char* title;
    // @brief Начальная ширина окна в пикселях.
    u32 width;
    // @brief Начальная высота окна в пикселях.
    u32 height;
} platform_window_config;

/*
    @brief Инициализирует подсистему для работы с окнами.
    @note Должна быть вызвана один раз при старте приложения, перед использованием любых других функций работы с окнами.
    @warning Не thread-safe. Должна вызываться из основного потока.
    @brief type Тип используемого оконного бэкенда.
    @return true - инициализация успешна, false - произошла ошибка.
*/
API bool platform_window_initialize(platform_window_backend_type type);

/*
    @brief Завершает работу подсистемы для работы с окнами.
    @note Должна быть вызвана при завершении приложения, после уничтожения всех окон.
    @warning Не thread-safe. Должна вызываться из основного потока.
*/
API void platform_window_shutdown();

/*
    @brief Проверяет, была ли инициализирована подсистема для работы с окнами.
    @note Может использоваться для проверки состояния подсистемы перед вызовом других функций.
    @warning Не thread-safe. Должна вызываться из того же потока, что и инициализация/завершение.
    @return true - подсистема инициализирована и готова к работе, false - подсистема не инициализирована.
*/
API bool platform_window_is_initialized();

/*
    @brief Обрабатывает оконные сообщения/события.
    @return true - оконные сообщения обработаны успешно, false - произошла ошибка.
*/
API bool platform_window_poll_events();

/*
    @brief Создает новое окно с указанной конфигурацией.
    @param config Указатель на структуру с параметрами окна.
    @return Указатель на контекст созданного окна в случае успеха, nullptr при ошибке.
*/
API platform_window* platform_window_create(const platform_window_config* config);

/*
    @brief Уничтожает созданное окно и освобождает ресурсы.
    @param window Указатель на контекст окна для уничтожения.
*/
API void platform_window_destroy(platform_window* window);
