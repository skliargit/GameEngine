#pragma once

#include <core/defines.h>
#include <vulkan/vulkan.h>

typedef struct platform_window platform_window;

// @brief Информация о поддержке цепочки обмена устройством.
typedef struct vulkan_swapchain_support {
    // @brief Базовые возможности поверхности (размеры, трансформации и т.д.).
    VkSurfaceCapabilitiesKHR capabilities;
    // @brief Количество доступных форматов пикселей поверхности.
    u32 format_count;
    // @brief Массив доступных форматов пикселей (используется darray).
    VkSurfaceFormatKHR* formats;
    // @brief Количество доступных режимов представления (presentation modes).
    u32 present_mode_count;
    // @brief Массив доступных режимов представления (используется darray).
    VkPresentModeKHR* present_modes;
} vulkan_swapchain_support;

// @brief Цепочка обмена для управления изображениями представления.
typedef struct vulkan_swapchain {
    // @brief Информация о поддержке возможностей цепочки обмена для поверхности.
    // vulkan_swapchain_support support_info;
} vulkan_swapchain;

// @brief
typedef struct vulkan_physical_device {
    // @brief Указатель на физическое устройство (GPU).
    VkPhysicalDevice handle;
    // @brief Поддерживаемые функции физического устройства.
    VkPhysicalDeviceFeatures features;
    // @brief Свойства и ограничения физического устройства.
    VkPhysicalDeviceProperties properties;
    // @brief Характеристики памяти устройства (типы памяти, размеры и т.д.).
    VkPhysicalDeviceMemoryProperties memory_properties;
    // @brief
    u32 queue_total_count;
    // @brief
    u32 queue_graphics_count;
    // @brief
    u32 queue_compute_count;
    // @brief
    u32 queue_transfer_count;
    // @brief
    u32 queue_present_count;
} vulkan_physical_device;

// @brief Описание очереди и связанных с ней ресурсов.
typedef struct vulkan_queue {
    // @brief Указатель на очередь.
    VkQueue handle;
    // @brief Указывает на выделенную очередь.
    bool dedicated;
} vulkan_queue;

// @brief Конфигурация для выбора и настройки физического устройства.
typedef struct vulkan_device_config {
    // @brief Тип физического устройства.
    VkPhysicalDeviceType device_type;
    // @brief Количество расширений устройства.
    u32 extension_count;
    // @brief Массив имен расширений устройства.
    const char** extensions;
    // @brief Поддержка анизотропной фильтрации сэмплера.
    bool use_sampler_anisotropy;
} vulkan_device_config;

// @brief Представление логического и физического устройства.
typedef struct vulkan_device {
    // @brief Указатель на физическое устройство (GPU).
    VkPhysicalDevice physical;
    // @brief Логическое устройство.
    VkDevice logical;

    // @brief Очередь для графических операций (рендеринг).
    vulkan_queue graphics_queue;
    // @brief Очередь для операций представления (отображение буфера на экране).
    vulkan_queue present_queue;
    // @brief Очередь для операций передачи данных (память -> память).
    vulkan_queue transfer_queue;
    // @brief Очередь для вычислительных операций (в настоящее время не используется).
    vulkan_queue compute_queue;

    // @brief Флаг поддержки host-visible памяти с локальным доступом от устройства.
    // bool has_host_visible_local_memory;
} vulkan_device;

// @brief Основной контекст рендерера.
typedef struct vulkan_context {
    // @brief Экземпляр Vulkan (корневой объект Vulkan API).
    VkInstance instance;
    // @brief Указатель на кастомный аллокатор памяти (может быть nullptr).
    VkAllocationCallbacks* allocator;
    // @brief Кастомный аллокатор памяти для Vulkan объектов.
    VkAllocationCallbacks custom_allocator;
    // @brief Мессенджер для отладочных сообщений Vulkan (validation layers).
    VkDebugUtilsMessengerEXT debug_messenger;
    // @brief Указывает на использование расширения для получения адресов функций отладки.
    bool debug_messenger_address_binding_report_using;
    // @brief Указатель на связанное с рендерером окно.
    platform_window* window;
    // @brief Поверхность Vulkan для отрисовки в окно.
    VkSurfaceKHR surface;
    // @brief Устройство Vulkan (GPU).
    vulkan_device device;
    // @brief Цепочка обмена для управления буферами представления.
    vulkan_swapchain swapchain;
} vulkan_context;
