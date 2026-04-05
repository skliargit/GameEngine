#pragma once

#include <core/defines.h>
#include <core/input_types.h>

/**
    @brief Контекст окна (представляет окно платформы).
*/
typedef struct platform_window platform_window_t;

/**
    @brief Конфигурация для создания нового окна.
*/
typedef struct {
    const char* title;                         /**< @brief Заголовок окна.                                         */
    i32 x;                                     /**< @brief Позиция окна по X (-1 для центрирования).               */ // TODO
    i32 y;                                     /**< @brief Позиция окна по Y (-1 для центрирования).               */ // TODO
    u32 width;                                 /**< @brief Ширина окна в пикселях.                                 */
    u32 height;                                /**< @brief Высота окна в пикселях.                                 */
    bool fullscreen;                           /**< @brief Создание окна в полноэкранном режиме.                   */ // TODO
    bool borderless;                           /**< @brief Создание окна без рамки (если поддерживается).          */ // TODO
    bool transparent;                          /**< @brief Создание окна с прозрачностью (если поддерживается).    */ // TODO
    bool always_on_top;                        /**< @brief Создание окна поверх других окон (если поддерживается). */ // TODO
    bool vsync;                                /**< @brief Запрос вертикальной синхронизации (vsync).              */ // TODO
} platform_window_config_t;

/**
    @brief Тип оконного бэкенда.
*/
typedef enum {
    PLATFORM_WINDOW_BACKEND_DEFAULT,           /**< @brief Автоматический выбор оконной системы (по умолчанию).    */
    PLATFORM_WINDOW_BACKEND_WAYLAND,           /**< @brief Wayland - современный протокол отображения Linux.       */
    PLATFORM_WINDOW_BACKEND_XCB,               /**< @brief XCB (X11/Xorg) - традиционная оконная система Linux.    */
    PLATFORM_WINDOW_BACKEND_WIN32,             /**< @brief Classic Window Manager - оконная система Windows.       */
    PLATFORM_WINDOW_BACKEND_COUNT              /**< @brief Количество типов бэкендов (не является реальным типом). */
} platform_window_backend_t;

/**
    @brief Типы событий окна.
*/
typedef enum {
    PLATFORM_WINDOW_EVENT_SHOULD_CLOSE,        /**< @brief Запрос на закрытие окна (для отмены вернуть false из callback).       */
    PLATFORM_WINDOW_EVENT_DESTROY,             /**< @brief Уничтожение окна (нельзя отменить).                                   */
    PLATFORM_WINDOW_EVENT_RESIZE,              /**< @brief Изменение размера окна (содержит новые размеры).                      */
    PLATFORM_WINDOW_EVENT_FOCUS,               /**< @brief Изменение фокуса окна (state = true - окно получило фокус).           */
    PLATFORM_WINDOW_EVENT_MOVE,                /**< @brief Перемещение окна (содержит новые координаты).                         */ // TODO
    PLATFORM_WINDOW_EVENT_MINIMIZE,            /**< @brief Окно минимизировано (свернуто).                                       */ // TODO
    PLATFORM_WINDOW_EVENT_MAXIMIZE,            /**< @brief Окно максимизировано (развернуто).                                    */ // TODO
    PLATFORM_WINDOW_EVENT_FULLSCREEN,          /**< @brief Окно перешло в полноэкранный режим.                                   */ // TODO
    PLATFORM_WINDOW_EVENT_RESTORE,             /**< @brief Окно восстановлено из минимизированного/максимизированного состояния. */ // TODO
    PLATFORM_WINDOW_EVENT_KEYBOARD_KEY,        /**< @brief Изменение состояния клавиши (нажата/отпущена).                        */
    PLATFORM_WINDOW_EVENT_KEYBOARD_CHAR,       /**< @brief Ввод символа (текстовый ввод).                                        */ // TODO
    PLATFORM_WINDOW_EVENT_KEYBOARD_ENTER,      /**< @brief Клавиатура получила фокус ввода (state = true).                       */
    PLATFORM_WINDOW_EVENT_KEYBOARD_LEAVE,      /**< @brief Клавиатура потеряла фокус ввода (state = false).                      */
    PLATFORM_WINDOW_EVENT_MOUSE_BUTTON,        /**< @brief Изменение состояния кнопки мыши (нажата/отпущена).                    */
    PLATFORM_WINDOW_EVENT_MOUSE_MOVE,          /**< @brief Абсолютное перемещение курсора мыши в окне.                           */
    PLATFORM_WINDOW_EVENT_MOUSE_MOVE_RELATIVE, /**< @brief Относительное перемещение курсора мыши в окне.                        */
    PLATFORM_WINDOW_EVENT_MOUSE_WHEEL,         /**< @brief Прокрутка колеса мыши (вертикальная и горизонтальная).                */
    PLATFORM_WINDOW_EVENT_MOUSE_DRAG,          /**< @brief Перетаскивание (в процессе).                                          */ // TODO
    PLATFORM_WINDOW_EVENT_MOUSE_DROP,          /**< @brief Конец перетаскивания.                                                 */ // TODO
    PLATFORM_WINDOW_EVENT_MOUSE_ENTER,         /**< @brief Курсор вошел в область окна (state = true).                           */
    PLATFORM_WINDOW_EVENT_MOUSE_LEAVE,         /**< @brief Курсор покинул область окна (state = false).                          */
    PLATFORM_WINDOW_EVENT_REFRESH,             /**< @brief Требуется перерисовка содержимого окна.                               */ // TODO
    PLATFORM_WINDOW_EVENT_COUNT                /**< @brief Количество событий (не является реальным событием).                   */
} platform_window_event_t;

// @brief Контекст события.
typedef struct {
    // @brief Окно, в котором произошло событие.
    platform_window_t* window;
    // @brief Тип события.
    platform_window_event_t type;
    // @brief Пользовательские данные.
    void* user_data;

    union {
        // @brief Событие изменения размеров окна.
        struct {
            // @brief Текущая ширина окна в пикселях.
            u32 to_width;
            // @brief Текушая высота окна в пикселях.
            u32 to_height;
            // @brief Окно свернуто.
            // bool minimized;
            // @brife Окно развернуто.
            // bool maximized;
            // @brief Окна занимает весь экран.
            // bool fullscreen;
            // @brief Состояние события (true - в процессе, false - завершено).
            bool state;
        } window_resize;

        // @brief Событие фокуса окна.
        struct {
            // @brief Состояние события (true окно получило фокус, false окно потеряло фокус).
            bool state;
        } window_focus;

        // @brief Событие перемещения.
        struct {
            // @brief Текущая позиция окна по горизонтали в пикселях.
            i32 to_x;
            // @brief Текущая позиция окна по вертикали в пикселях.
            i32 to_y;
            // @brife Состояние перемещения (true - в процессе, false - завершено).
            bool state;
        } window_move;

        // @brief Событие клавиатуры.
        struct {
            // @brief Состояние фокуса клавиатуры (true захвачена окном, false освобождена).
            bool state;
        } keyboard_focus;

        // @brief Событие клавиатуры.
        struct {
            // @brief Нормализованный код клавиши (см. input_types).
            keyboard_key code;
            // @brief Состояние клавиши (true нажата, false отпущена).
            bool state;
        } keyboard_key;

        // TODO: Событие ввода символа.
        // struct {
        //     // @brief Код символа Unicode.
        //     u32 codepoint;
        // } character;

        // @brief Событие мыши.
        struct {
            // @brief Состояние фокуса мыши (true находится в окне, false за пределами).
            bool state;
        } mouse_focus;

        // @brief Событие мыши.
        struct {
            // @brief Нормализованный код кнопки (см. input_types).
            mouse_button code;
            // @brief Состояние кнопки (true нажата, false отпущена).
            bool state;
        } mouse_button;

        // @brief Событие мыши.
        struct {
            // @brief Текущая позиция мыши по горизонтали в пикселях (не доступно в режиме: PLATFORM_WINDOW_CURSOR_MODE_DISABLE).
            i32 to_x;
            // @brief Текущая позиция мыши по вертикали в пикселях (не доступно в режиме: PLATFORM_WINDOW_CURSOR_MODE_DISABLE).
            i32 to_y;
        } mouse_move;

        // @brief Событие мыши.
        struct {
            // @brief Смещение позиции мыши по горизонтали в пикселях (только режим: PLATFORM_WINDOW_CURSOR_MODE_DISABLE).
            i32 dx;
            // @brief Смещение позиции мыши по вертикали в пикселях (только режим: PLATFORM_WINDOW_CURSOR_MODE_DISABLE).
            i32 dy;
        } mouse_move_relative;

        // @brief Событие мыши.
        struct {
            // @brief Текущее значение вертикальной прокрутки (положительное - вперед, отрицательное - назад).
            i32 delta_vert;
            // @brief Текущее значение горизонтальной прокрутки (отрицательное - влево, положительное - вправо).
            i32 delta_horz;
        } mouse_wheel;
    };
} platform_window_event_context_t;

/*
    @brief Callback-функция для обработки событий окна с возможностью отмены.
    @param event Передаваемые данные события (контекст).
*/
typedef bool (*platform_window_event_callback)(platform_window_event_context_t* event);

// @brief Структура, описывающая зарегистрированный обработчик событий окна.
typedef struct {
    // @brief Функция-обработчик, вызываемая при наступлении события.
    platform_window_event_callback callback;
    // @brief Пользовательские данные, передаваемые в функцию-обработчик.
    void* user_data;
} platform_window_event_listener_t;
