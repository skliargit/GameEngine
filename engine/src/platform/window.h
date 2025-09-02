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
            - Систему памяти memory_system_initialize()
            - Подсистему окон platform_window_initialize()
*/

#pragma once

#include <core/defines.h>
#include <core/input_types.h>

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

/*
    @brief Callback-функция, вызываемая при попытке закрытия окна.
    @note Может использоваться для подтверждения закрытия или сохранения данных.
*/
typedef void (*platform_window_on_close_callback)();

/*
    @brief Callback-функция, вызываемая при изменении размера окна.
    @param new_width Новая ширина окна в пикселях.
    @param new_height Новая высота окна в пикселях.
*/
typedef void (*platform_window_on_resize_callback)(const u32 new_width, const u32 new_height);

/*
    @brief Callback-функция, вызываемая при изменении состояния фокуса окна.
    @param focus_state true - окно получило фокус, false - окно потеряло фокус.
*/
typedef void (*platform_window_on_focus_callback)(const bool focus_state);

/*
    @brief Callback-функция для обработки нажатий и отпусканий клавиш клавиатуры.
    @param key Код клавиши.
    @param state true - клавиша нажата, false - клавиша отпущена.
*/
typedef void (*platform_window_on_key_callback)(const keyboard_key key, const bool state);

/*
    @brief Callback-функция для обработки нажатий кнопок мыши.
    @param btn Код кнопки мыши.
    @param state true - кнопка нажата, false - кнопка отпущена.
*/
typedef void (*platform_window_on_mouse_button_callback)(const mouse_button btn, const bool state);

/*
    @brief Callback-функция для обработки движения курсора мыши в пределах окна.
    @param x Координата X курсора мыши относительно левого края окна.
    @param y Координата Y курсора мыши относительно верхнего края окна.
*/
typedef void (*platform_window_on_mouse_move_callback)(const i32 x, const i32 y);

/*
    @brief Callback-функция для обработки прокрутки колеса мыши.
    @param vertical_delta Значение вертикальной прокрутки (положительное - вперед, отрицательное - назад).
    @param horizontal_delta Значение горизонтальной прокрутки (отрицательное - влево, положительное - вправо).
*/
typedef void (*platform_window_on_mouse_wheel_callback)(const i32 vertical_delta, const i32 horizontal_delta);

// TODO: Дополнительные события для будущей реализации:
// on_move - перемещение окна
// on_char - ввод символов (текстовый ввод)
// on_minimize - минимизация окна
// on_maximize - максимизация окна
// on_restore - восстановление окна после минимизации/максимизации

// @brief Конфигурация для создания нового окна.
typedef struct platform_window_config {
    // @brief Заголовок окна, отображаемый в заголовке окна.
    const char* title;
    // @brief Начальная ширина окна в пикселях.
    u32 width;
    // @brief Начальная высота окна в пикселях.
    u32 height;
    // @brief Callback-функция, вызываемая при попытке закрытия окна, может быть nullptr.
    platform_window_on_close_callback on_close;
    // @brief Callback-функция, вызываемая при изменении размера окна, может быть nullptr.
    platform_window_on_resize_callback on_resize;
    // @brief Callback-функция, вызываемая при изменении состояния фокуса окна, может быть nullptr.
    platform_window_on_focus_callback on_focus;
    // @brief Callback-функция для обработки нажатий и отпусканий клавиш клавиатуры, может быть nullptr.
    platform_window_on_key_callback on_key;
    // @brief Callback-функция для обработки нажатий кнопок мыши, может быть nullptr.
    platform_window_on_mouse_button_callback on_mouse_button;
    // @brief Callback-функция для обработки движения курсора мыши в пределах окна, может быть nullptr.
    platform_window_on_mouse_move_callback on_mouse_move;
    // @brief Callback-функция для обработки прокрутки колеса мыши, может быть nullptr.
    platform_window_on_mouse_wheel_callback on_mouse_wheel;
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
