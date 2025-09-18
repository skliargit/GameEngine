/*
    @file vulkan_window.h
    @brief Утилиты для интеграции платформенного окна с Vulkan.
    @author Дмитрий Скляр.
    @version 1.0
    @date 18-09-2025

    @license Лицензия Apache, версия 2.0 («Лицензия»);
          Вы не имеете права использовать этот файл без соблюдения условий Лицензии.
          Копию Лицензии можно получить по адресу http://www.apache.org/licenses/LICENSE-2.0
          Если иное не предусмотрено действующим законодательством или не согласовано в письменной форме,
          программное обеспечение, распространяемое по Лицензии, распространяется на условиях «КАК ЕСТЬ»,
          БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ ИЛИ УСЛОВИЙ, явных или подразумеваемых. См. Лицензию для получения
          информации о конкретных языках, регулирующих разрешения и ограничения по Лицензии.

    @note Этот файл предоставляет функции для создания и управления Vulkan поверхностями для платформенных окон.
*/

#pragma once

#include <core/defines.h>
#include <vulkan/vulkan.h>
#include <platform/window.h>

/*
    @brief Перечисляет расширения Vulkan, необходимые для создания поверхности окна на текущей платформе.
    @note Для получения расширения используется двухэтапный подход: первый вызов (out_extensions = nullptr)
          возвращает количество расширений, второй вызов сохраняет имена расширений в предоставленный буфер.
    @param extension_count Указатель на переменную для сохранения количества расширений доступных при
           (out_extensions = nullptr), или размер буфера out_extensions для записи расширений.
    @param out_extensions Указатель на массив строк, который будет заполнен именами расширений, или
           nullptr для получения количество расширений через extension_count.
*/
void platform_window_enumerate_vulkan_extensions(u32* extension_count, const char** out_extensions);

/*
    @brief Создает поверхность Vulkan для указанного окна.
    @note Поверхность должна быть уничтожена с помощью platform_window_destroy_vulkan_surface().
    @param window Указатель на платформенное окно.
    @param vulkan_instance Указатель на экземпляр Vulkan (VkInstance).
    @param vulkan_allocator Указатель на аллокатор Vulkan (VkAllocationCallbacks) или nullptr.
    @param out_vulkan_surface Указатель для получения созданной поверхности (VkSurfaceKHR).
    @return Код результата VK_SUCCESS при успехе (VkResult).
*/
u32 platform_window_create_vulkan_surface(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void** out_vulkan_surface);

/*
    @brief Уничтожает поверхность Vulkan для указанного окна.
    @note Должна использоваться для поверхностей, созданных через platform_window_create_vulkan_surface().
    @param window Указатель на платформенное окно.
    @param vulkan_instance Указатель на экземпляр Vulkan (VkInstance).
    @param vulkan_allocator Указатель на аллокатор Vulkan (VkAllocationCallbacks) или nullptr.
    @param vulkan_surface Поверхность Vulkan для уничтожения (VkSurfaceKHR).
*/
void platform_window_destroy_vulkan_surface(platform_window* window, void* vulkan_instance, void* vulkan_allocator, void* vulkan_surface);

/*
    @brief Проверяет поддержку представления Vulkan для конкретного устройства и семейства очередей.
    @note Используется для выбора подходящего семейства очередей для представления.
    @param window Указатель на платформенное окно.
    @param vulkan_pyhical_device Указатель на физическое устройство Vulkan (VkPhysicalDevice).
    @param queue_family_index Индекс семейства очередей для проверки.
    @return VK_TRUE если представление поддерживается, VK_FALSE в противном случае (VkBool32).
*/
u32 platform_window_supports_vulkan_presentation(platform_window* window, void* vulkan_pyhical_device, u32 queue_family_index);
