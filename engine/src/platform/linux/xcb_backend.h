/*
    @file xcb_backend.h
    @brief XCB бэкенд для системы управления окнами на платформе Linux.
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
            - Инициализацию и завершение работы XCB бэкенда
            - Обработку событий оконной системы через XCB
            - Создание и уничтожение окон через XCB протокол
            - Проверку поддержки XCB на целевой системе
            - Управление требованиями к памяти для XCB контекста

    @dependencies Требует XCB библиотеки для работы с X Window System
*/

#pragma once

#include <core/defines.h>
#include <platform/window.h>

/*
    @brief Инициализирует XCB бэкенд для работы с оконной системой.
    @note Должна быть вызвана перед использованием других функций бэкенда.
    @param internal_data Указатель на внутренние данные бэкенда.
    @return true если инициализация прошла успешно, false в случае ошибки.
*/
bool xcb_backend_initialize(void* internal_data);

/*
    @brief Завершает работу XCB бэкенда и освобождает ресурсы.
    @note Должна быть вызвана после завершения работы с бэкендом.
    @param internal_data Указатель на внутренние данные бэкенда.
*/
void xcb_backend_shutdown(void* internal_data);

/*
    @brief Обрабатывает события оконной системы через XCB.
    @note Должна вызываться каждый кадр для обработки входящих событий.
    @param internal_data Указатель на внутренние данные бэкенда.
    @return true если обработка событий прошла успешно, false в случае ошибки.
*/
bool xcb_backend_poll_events(void* internal_data);

/*
    @brief Проверяет поддержку XCB на текущей системе.
    @note Может использоваться для выбора подходящего бэкенда во время выполнения.
    @return true если XCB поддерживается, false в противном случае.
*/
bool xcb_backend_is_supported();

/*
    @brief Возвращает требования к памяти для XCB бэкенда.
    @note Используется для выделения достаточного объема памяти перед инициализацией.
    @return Количество байт памяти, необходимое для работы бэкенда.
*/
u64  xcb_backend_memory_requirement();

/*
    @brief Создает новое окно через XCB бэкенд.
    @note Окно создается с параметрами, указанными в конфигурации.
    @param config Конфигурация создаваемого окна.
    @param internal_data Указатель на внутренние данные бэкенда.
    @return Указатель на созданное окно или nullptr в случае ошибки.
*/
platform_window* xcb_backend_window_create(const platform_window_config* config, void* internal_data);

/*
    @brief Уничтожает окно, созданное через XCB бэкенд.
    @note Освобождает все ресурсы, связанные с окном.
    @param window Указатель на уничтожаемое окно.
    @param internal_data Указатель на внутренние данные бэкенда.
*/
void xcb_backend_window_destroy(platform_window* window, void* internal_data);

/*
    @brief Получает текущий заголовок окна.
    @param window Указатель на контекст окна для получения заголовка.
    @return Указатель на строку с заголовком окна.
*/
const char* xcb_backend_window_get_title(platform_window* window);

/*
    @brief Получает размеры клиентской области окна в пикселях.
    @param window Указатель на контекст окна для получения размеров.
    @param width Указатель на переменную для сохранения ширина окна.
    @param height Указатель на переменную для сохранения высоты окна.
*/
void xcb_backend_window_get_resolution(platform_window* window, u32* width, u32* height);

/*
    @brief Перечисляет расширения Vulkan, необходимые для создания поверхности окна на текущей платформе.
    @note Для получения расширения используется двухэтапный подход: первый вызов (out_extensions = nullptr)
          возвращает количество расширений, второй вызов сохраняет имена расширений в предоставленный буфер.
    @param extension_count Указатель на переменную для сохранения количества расширений доступных при
           (out_extensions = nullptr), или размер буфера out_extensions для записи расширений.
    @param out_extensions Указатель на массив строк, который будет заполнен именами расширений, или
           nullptr для получения количество расширений через extension_count.
*/
void xcb_backend_enumerate_vulkan_extensions(u32* extension_count, const char** out_extensions);

/*
    @brief Создает поверхность Vulkan для указанного окна.
    @note Поверхность должна быть уничтожена с помощью platform_window_destroy_vulkan_surface().
    @param window Указатель на платформенное окно.
    @param vulkan_instance Указатель на экземпляр Vulkan (VkInstance).
    @param vulkan_allocator Указатель на аллокатор Vulkan (VkAllocationCallbacks) или nullptr.
    @param out_vulkan_surface Указатель для получения созданной поверхности (VkSurfaceKHR).
    @return Код результата VK_SUCCESS при успехе (VkResult).
*/
u32 xcb_backend_create_vulkan_surface(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void** out_vulkan_surface);

/*
    @brief Уничтожает поверхность Vulkan для указанного окна.
    @note Должна использоваться для поверхностей, созданных через platform_window_create_vulkan_surface().
    @param window Указатель на платформенное окно.
    @param vulkan_instance Указатель на экземпляр Vulkan (VkInstance).
    @param vulkan_allocator Указатель на аллокатор Vulkan (VkAllocationCallbacks) или nullptr.
    @param vulkan_surface Поверхность Vulkan для уничтожения (VkSurfaceKHR).
*/
void xcb_backend_destroy_vulkan_surface(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void* vulkan_surface);

/*
    @brief Проверяет поддержку представления Vulkan для конкретного устройства и семейства очередей.
    @note Используется для выбора подходящего семейства очередей для представления.
    @param window Указатель на платформенное окно.
    @param vulkan_pyhical_device Указатель на физическое устройство Vulkan (VkPhysicalDevice).
    @param queue_family_index Индекс семейства очередей для проверки.
    @return VK_TRUE если представление поддерживается, VK_FALSE в противном случае (VkBool32).
*/
u32 xcb_backend_supports_vulkan_presentation(platform_window* window, void* vulkan_pyhical_device, u32 queue_family_index);
