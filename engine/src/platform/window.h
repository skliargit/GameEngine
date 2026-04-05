/*
    @file window.h
    @brief Кросс-платформенный интерфейс для работы с окнами.
    @author Дмитрий Скляр.
    @version 1.1
    @date 04-02-2026

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
#include <platform/window_types.h>

/*
    @brief Инициализирует подсистему для работы с окнами.
    @brief type Тип используемого оконного бэкенда.
    @return true - инициализация успешна, false - произошла ошибка.

    @warning Не thread-safe. Должна вызываться из основного потока.
    @note Должна быть вызвана один раз перед использованием любых других функций работы с окнами.
*/
bool platform_window_initialize(platform_window_backend_t type);

/*
    @brief Завершает работу подсистемы для работы с окнами.

    @warning Не thread-safe. Должна вызываться из основного потока.
    @note Должна быть вызвана при завершении приложения, после уничтожения всех окон.
*/
void platform_window_shutdown();

/*
    @brief Проверяет, была ли инициализирована подсистема для работы с окнами.
    @return true - подсистема инициализирована и готова к работе, false - подсистема не инициализирована.

    @warning Не thread-safe. Должна вызываться из того же потока, что и инициализация/завершение.
    @note Может использоваться для проверки состояния подсистемы перед вызовом других функций.
*/
CORE_API bool platform_window_is_initialized();

/*
    @brief Обрабатывает оконные сообщения/события.
    @return true - оконные сообщения обработаны успешно, false - произошла ошибка.
*/
bool platform_window_poll_events();

/*
    @brief Создает новое окно с указанной конфигурацией.
    @param config Указатель на структуру с параметрами окна.
    @return Указатель на контекст созданного окна в случае успеха, nullptr при ошибке.
*/
CORE_API platform_window_t* platform_window_create(const platform_window_config_t* config);

/*
    @brief Уничтожает созданное окно и освобождает ресурсы.
    @param window Указатель на контекст окна для уничтожения.
*/
CORE_API void platform_window_destroy(platform_window_t* window);

/*
    @brief Получает текущий заголовок окна.
    @param window Указатель на контекст окна для получения заголовка.
    @return Указатель на строку с заголовком окна.
*/
CORE_API const char* platform_window_get_title(platform_window_t* window);

/*
    @brief Получает размеры клиентской области окна в пикселях.
    @param window Указатель на контекст окна для получения размеров.
    @param width Указатель на переменную для сохранения ширина окна.
    @param height Указатель на переменную для сохранения высоты окна.
*/
CORE_API void platform_window_get_resolution(platform_window_t* window, u32* width, u32* height);

/*
    @brief Устанавливает функцию для обработки указанного события окна.
    @param window Указатель на контекст окна для установки функции-обработчика.
    @param event Тип события для подписки.
    @param callback Функция-обработчик (может быть nullptr для удаления обработчика).
    @param user_data Пользовательские данные, передаваемые в функцию-обработчик (может быть nullptr).
*/
CORE_API void platform_window_set_event_callback(platform_window_t* window, platform_window_event_t event, platform_window_event_callback callback, void* user_data);

/*
*/
CORE_API void platform_window_hide_cursor(platform_window_t* window);

/*
*/
CORE_API void platform_window_show_cursor(platform_window_t* window);

/*
*/
CORE_API void platform_window_lock_cursor(platform_window_t* window);

/*
*/
CORE_API void platform_window_unlock_cursor(platform_window_t* window);
