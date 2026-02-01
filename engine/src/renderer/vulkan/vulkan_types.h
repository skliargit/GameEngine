#pragma once

#include <core/defines.h>
#include <vulkan/vulkan.h>
#include <renderer/types.h>

typedef struct platform_window platform_window;

// @brief Тип буфера данных.
typedef enum vulkan_buffer_type {
    // @brief Использование буфера для вершинных данных.
    VULKAN_BUFFER_TYPE_VERTEX,
    // @brief Использование буфера для индексных данных.
    VULKAN_BUFFER_TYPE_INDEX,
    // @brief Использование буфера для uniform переменных.
    VULKAN_BUFFER_TYPE_UNIFORM,
    // @brief Использование буфера для подготовки целей визуализации (из оперативной памяти в видеопамять).
    VULKAN_BUFFER_TYPE_STAGING,
    // @brief Использование буфера для чтения целей визуализации (для чтения из видеопамяти).
    VULKAN_BUFFER_TYPE_READ,
    // @brief Использование буфера для хранения данных.
    VULKAN_BUFFER_TYPE_STORAGE,
} vulkan_buffer_type;

// @brief Представляет буфер данных в Vulkan.
typedef struct vulkan_buffer {
    // @brief Тип буфера данных.
    vulkan_buffer_type type;
    // @brief Размер буфера данных в байтах.
    u64 size;
    // @brief Флаги использования буфера (кеш).
    VkBufferUsageFlagBits usage;
    // @brief Флаги свойств памяти (кеш).
    VkMemoryPropertyFlags memory_property_flags;
    // @brief Указатель на буфер.
    VkBuffer handle;
    // @brief Индекс типа памяти, используемый буфером (кеш).
    u32 memory_index;
    // @brief Требования к памяти для буфера (кеш).
    VkMemoryRequirements memory_requirements;
    // @brief Выделенная память устройства для буфера.
    VkDeviceMemory memory;
} vulkan_buffer;

// @brief Котекст экземпляра изображения в Vulkan.
typedef struct vulkan_image {
    // @brief Указатель на изображение.
    VkImage handle;
    // @brief Вид изображения, используется для доступа к изображению.
    VkImageView view;
    // @brief Память изображения (GPU сторона).
    VkDeviceMemory memory;
    // @brief Требования к памяти GPU.
    VkMemoryRequirements memory_requirements;
    // @brief Флаги свойств памяти (кеш).
    VkMemoryPropertyFlags memory_property_flags;
    // @brief Ширина изображения.
    u32 width;
    // @brief Высота изображения.
    u32 height;
    // @brief Флаг указывающий на использование памяти GPU.
    bool use_device_local;
} vulkan_image;

// @brief Цепочка обмена для управления изображениями представления.
typedef struct vulkan_swapchain {
    // @brief Указатель на цепочку обмена.
    VkSwapchainKHR handle;
    // @brief Размеры кадра цепочки обмена.
    VkExtent2D image_extent;
    // @brief Количество кадров цепочки обмена.
    u32 image_count;
    // @brief Формат пикселей изображения.
    VkSurfaceFormatKHR image_format;
    // @brief Преображования к изображениям цепочки обмена.
    VkSurfaceTransformFlagBitsKHR image_transform;
    // @brief Режим представления изображений.
    VkPresentModeKHR present_mode;
    // @brief Изображения цепочки обмена (используется darray).
    VkImage* images;
    // @brief Представления изображений (используется darray).
    VkImageView* image_views;
    // @brief Формат буфера глубины.
    VkFormat depth_format;
    // @brief Количество каналов буфера глубины.
    u8 depth_channel_count;
    // @brief Буфер глубины (один для всех изображений, кадров).
    vulkan_image depth_image;
    // @brief Индекс текущего изображения.
    u32 image_index;
    // @brief Номер текущего кадра.
    u32 current_frame;
    // @brief Максимальное количество кадров (2 - двойная, 3 - тройная буферезация, а больше и не потребуется!).
    u8 max_frames_in_flight;
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
    // @brief Индекс семейства очередей.
    u32 family_index;
    // @brief Указатель на очередь.
    VkQueue handle;
    // @brief Указывает на выделенную очередь.
    bool dedicated;
    // @brief Указатель на пул команд.
    VkCommandPool command_pool;
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

    // @brief Характеристики памяти устройства (типы памяти, размеры и т.д.).
    VkPhysicalDeviceMemoryProperties memory_properties;
    // @brief Флаг поддержки host-visible памяти с локальным доступом от устройства.
    bool supports_host_local_memory;
} vulkan_device;

// TODO: Изменить с учетом других шейдеров.
#define MAX_SHADER_STAGES 2

// @brief Представляет шейдер.
typedef struct vulkan_shader {
    // @brief Макет дескрипторных наборов.
    VkDescriptorSetLayout descriptor_set_layout;
    // @brief Макет конвейера, описывающий размещение дескрипторных наборов и push-констант.
    VkPipelineLayout pipeline_layout;
    // @brief Указатель на графический конвейер, содержащий компилированные шейдерные модули и другие функций.
    VkPipeline pipeline;
    // @brief Пул дескрипторов.
    VkDescriptorPool descriptor_pool;
    // @brief Массив дескрипторных наборов (на кадр). Зарезервировано 3, т.к. кол-во кадров в обработке 2-3 (см. max_frames_in_flight).
    VkDescriptorSet descriptor_sets[3];
    // @brief Буфер uniform переменных.
    vulkan_buffer uniform_buffer;
} vulkan_shader;

// @brief Основной контекст рендерера.
typedef struct vulkan_context {
    // @brief Ширина окна, запланированная к применению (ожидающая обработки).
    u32 frame_pending_width;
    // @brief Высота окна, запланированная к применению (ожидающая обработки).
    u32 frame_pending_height;
    // @brief Версия ожидающих изменений размера. Увеличивается при каждом новом запросе на изменение размера.
    u32 frame_pending_generation;
    // @brief Текущая ширина окна, используемая для рендеринга.
    u32 frame_width;
    // @brief Текущая высота окна, используемая для рендеринга.
    u32 frame_height;
    // @brief Версия примененных изменений размера. Обновляется после успешного пересоздания цепочки обмена.
    u32 frame_generation;

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

    // @brief Массив объектов синхронизации указывающих на готовность выполнения операций с изображением (на кард).
    VkSemaphore* image_available_semaphores;
    // @brief Массив объектов синхронизации указывающих на готовность записи командного буфера (на кадр).
    VkFence* in_flight_fences;
    // @brief Массив объектов синхронизации указывающих на готовность представления изображения на экран (на изображение).
    VkSemaphore* image_complete_semaphores;
    // @brief Массив указателей на объекты синхронизации ... (на изображение).
    VkFence* images_in_flight;

    // @brief Буферы команд для графических операций (на кадр).
    VkCommandBuffer* graphics_command_buffers;

    // TODO: Временно!
    vulkan_shader world_shader;

    // TODO: Временно!
    u64 vertex_buffer_offset;
    vulkan_buffer vertex_buffer;
    u64 index_buffer_offset;
    vulkan_buffer index_buffer;

    // TODO: Временно!
    renderer_camera camera;

} vulkan_context;
