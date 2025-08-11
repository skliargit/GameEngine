#pragma once

#include <core/defines.h>

// @brief Контекст окна (представляет окно платформы).
typedef struct platform_window platform_window;

// @brief Тип оконной системы/бэкенда.
typedef enum platform_window_backend_type {
    // @brief Автоматический выбор оконной системы (по умолчанию).
    PLATFORM_WINDOW_BACKEND_TYPE_AUTO,

#if PLATFORM_LINUX_FLAG
    // @brief Wayland - современный протокол отображения Linux.
    PLATFORM_WINDOW_BACKEND_TYPE_WAYLAND,
    // @brief XCB (X11/Xorg) - традиционная оконная система Linux.
    PLATFORM_WINDOW_BACKEND_TYPE_XCB,

#elif PLATFORM_WINDOWS_FLAG
    // @brief Classic Window Manager - оконная система Windows.
    PLATFORM_WINDOW_BACKEND_TYPE_WM,
#endif

    // @brief Количество доступных типов оконных систем (не является реальным типом).
    PLATFORM_WINDOW_BACKEND_TYPE_COUNT
} platform_window_backend_type;

// @brief Конфигурация для создания нового окна.
typedef struct platform_window_config {
    // @brief Тип используемого оконного бэкенда.
    platform_window_backend_type backend_type;
    // @brief Заголовок окна.
    const char* title;
    // @brief Начальная ширина окна в пикселях.
    u16 width;
    // @brief Начальная высота окна в пикселях.
    u16 height;
} platform_window_config;

/*
    @brief Создает новое окно с указанной конфигурацией.
    @param config Указатель на структуру с параметрами окна.
    @param out_window Указатель, по которому будет записан созданный контекст окна. При вызове должен быть nullptr!
    @return true в случае успеха, false при ошибке.

    @example Пример корректного использования:
        platform_window* window = nullptr;
        platform_window_config config = {...};
        if(!platform_window_create(&config, &window)) {
            // Обработка ошибки
        }
*/
API bool platform_window_create(const platform_window_config* config, platform_window** out_window);

/*
    @brief Уничтожает созданное окно и освобождает ресурсы.
    @param window Указатель на контекст окна для уничтожения.
*/
API void platform_window_destroy(platform_window* window);

/*
    @brief Обрабатывает оконные сообщения/события.
    @param window Указатель на контекст окна.
    @return true если приложение должно продолжать работу, false для завершения.
*/
API bool platform_window_poll_events(platform_window* window);
