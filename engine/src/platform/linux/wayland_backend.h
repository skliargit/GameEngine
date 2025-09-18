/*
    @file wayland_backend.h
    @brief Wayland бэкенд для системы управления окнами на платформе Linux.
    @author Дмитрий Скляр.
    @version 1.0
    @date 27-08-2025

    @license Лицензия Apache, версия 2.0 («Лицензия»);
          Вы не имеете права использовать этот файл без соблюдения условий Лицензии.
          Копию Лицензии можно получить по адресу http://www.apache.org/licenses/LICENSE-2.0
          Если иное не предусмотрено действующим законодательством или не согласовано в письменной форме,
          программное обеспечение, распространяемое по Лицензии, распространяется на условиях «КАК ЕСТЬ»,
          БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ ИЛИ УСЛОВИЙ, явных или подразумеваемых. См. Лицензию для получения
          информации о конкретных языках, регулирующих разрешения и ограничения по Лицензии.

    @note Предоставляет:
            - Инициализацию и завершение работы Wayland бэкенда
            - Обработку событий через Wayland протокол
            - Создание и уничтожение окон через Wayland
            - Проверку поддержки Wayland на целевой системе
            - Управление требованиями к памяти для Wayland контекста

    @dependencies Требует Wayland библиотеки для работы с Wayland композитором
*/

#pragma once

#include <core/defines.h>
#include <platform/window.h>

/*
    @brief Инициализирует Wayland бэкенд для работы с оконной системой.
    @note Должна быть вызвана перед использованием других функций бэкенда.
    @param internal_data Указатель на внутренние данные бэкенда.
    @return true если инициализация прошла успешно, false в случае ошибки.
*/
bool wayland_backend_initialize(void* internal_data);

/*
    @brief Завершает работу Wayland бэкенда и освобождает ресурсы.
    @note Должна быть вызвана после завершения работы с бэкендом.
    @param internal_data Указатель на внутренние данные бэкенда.
*/
void wayland_backend_shutdown(void* internal_data);

/*
    @brief Обрабатывает события оконной системы через Wayland.
    @note Должна вызываться каждый кадр для обработки входящих событий.
    @param internal_data Указатель на внутренние данные бэкенда.
    @return true если обработка событий прошла успешно, false в случае ошибки.
*/
bool wayland_backend_poll_events(void* internal_data);

/*
    @brief Проверяет поддержку Wayland на текущей системе.
    @note Может использоваться для выбора подходящего бэкенда во время выполнения.
    @return true если Wayland поддерживается, false в противном случае.
*/
bool wayland_backend_is_supported();

/*
    @brief Возвращает требования к памяти для Wayland бэкенда.
    @note Используется для выделения достаточного объема памяти перед инициализацией.
    @return Количество байт памяти, необходимое для работы бэкенда.
*/
u64 wayland_backend_memory_requirement();

/*
    @brief Создает новое окно через Wayland бэкенд.
    @note Окно создается с параметрами, указанными в конфигурации.
    @param config Конфигурация создаваемого окна.
    @param internal_data Указатель на внутренние данные бэкенда.
    @return Указатель на созданное окно или nullptr в случае ошибки.
*/
platform_window* wayland_backend_window_create(const platform_window_config* config, void* internal_data);

/*
    @brief Уничтожает окно, созданное через Wayland бэкенд.
    @note Освобождает все ресурсы, связанные с окном.
    @param window Указатель на уничтожаемое окно.
    @param internal_data Указатель на внутренние данные бэкенда.
*/
void wayland_backend_window_destroy(platform_window* window, void* internal_data);

/*
    @brief Перечисляет расширения Vulkan, необходимые для создания поверхности окна на текущей платформе.
    @note Для получения расширения используется двухэтапный подход: первый вызов (out_extentions = nullptr)
          возвращает количество расширений, второй вызов сохраняет имена расширений в предоставленный буфер.
    @param extention_count Указатель на переменную для сохранения количества расширений доступных при
           (out_extentions = nullptr), или размер буфера out_extentions для записи расширений.
    @param out_extentions Указатель на массив строк, который будет заполнен именами расширений, или
           nullptr для получения количество расширений через extention_count.
*/
void wayland_backend_enumerate_vulkan_extentions(u32* extention_count, const char** out_extentions);

/*
    @brief Создает поверхность Vulkan для указанного окна.
    @note Поверхность должна быть уничтожена с помощью platform_window_destroy_vulkan_surface().
    @param window Указатель на платформенное окно.
    @param vulkan_instance Указатель на экземпляр Vulkan (VkInstance).
    @param vulkan_allocator Указатель на аллокатор Vulkan (VkAllocationCallbacks) или nullptr.
    @param out_vulkan_surface Указатель для получения созданной поверхности (VkSurfaceKHR).
    @return Код результата VK_SUCCESS при успехе (VkResult).
*/
u32 wayland_backend_create_vulkan_surface(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void** out_vulkan_surface);

/*
    @brief Уничтожает поверхность Vulkan для указанного окна.
    @note Должна использоваться для поверхностей, созданных через platform_window_create_vulkan_surface().
    @param window Указатель на платформенное окно.
    @param vulkan_instance Указатель на экземпляр Vulkan (VkInstance).
    @param vulkan_allocator Указатель на аллокатор Vulkan (VkAllocationCallbacks) или nullptr.
    @param vulkan_surface Поверхность Vulkan для уничтожения (VkSurfaceKHR).
*/
void wayland_backend_destroy_vulkan_surface(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void* vulkan_surface);

/*
    @brief Проверяет поддержку представления Vulkan для конкретного устройства и семейства очередей.
    @note Используется для выбора подходящего семейства очередей для представления.
    @param window Указатель на платформенное окно.
    @param vulkan_pyhical_device Указатель на физическое устройство Vulkan (VkPhysicalDevice).
    @param queue_family_index Индекс семейства очередей для проверки.
    @return VK_TRUE если представление поддерживается, VK_FALSE в противном случае (VkBool32).
*/
u32 wayland_backend_supports_vulkan_presentation(platform_window* window, void* vulkan_pyhical_device, u32 queue_family_index);
