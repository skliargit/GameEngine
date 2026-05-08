#pragma once

#include <core/defines.h>
#include <vulkan/vulkan.h>
#include <renderer/types.h>

typedef struct platform_window platform_window;

/**
    @brief Представляет буфер данных в Vulkan.
*/
typedef struct vulkan_buffer {
    VkBufferUsageFlagBits usage;                         /**< Флаги использования буфера (кеш).               */
    VkMemoryPropertyFlags memory_property_flags;         /**< Флаги свойств памяти (кеш).                     */
    VkBuffer handle;                                     /**< Указатель на буфер.                             */
    u32 memory_index;                                    /**< Индекс типа памяти, используемый буфером (кеш). */
    VkMemoryRequirements memory_requirements;            /**< Требования к памяти для буфера (кеш).           */
    VkDeviceMemory memory;                               /**< Выделенная память устройства для буфера.        */
} vulkan_buffer_t;

/**
    @brief Контекст экземпляра изображения в Vulkan.
*/
typedef struct vulkan_image {
    VkImage handle;                                      /**< Указатель на изображение.                                */
    VkImageView view;                                    /**< Вид изображения, используется для доступа к изображению. */
    VkDeviceMemory memory;                               /**< Память изображения (GPU сторона).                        */
    VkMemoryRequirements memory_requirements;            /**< Требования к памяти GPU.                                 */
    VkMemoryPropertyFlags memory_property_flags;         /**< Флаги свойств памяти (кеш).                              */
    u32 width;                                           /**< Ширина изображения в пикселях.                           */
    u32 height;                                          /**< Высота изображения в пикселях.                           */
    bool use_device_local;                               /**< Флаг указывающий на использование памяти GPU.            */
} vulkan_image;

/**
    @brief Комбинации перевода layout-во изображений.
*/
typedef enum vulkan_image_transition {
    VULKAN_IMAGE_TRANSITION_UNDEFINED_TO_TRANSFER_DST,   /**< Подготавливает буфер цвета для записи в него данных.              */
    VULKAN_IMAGE_TRANSITION_TRANSFER_DST_TO_SHADER_READ, /**< Подготавливает буфер цвета для чтения шейдером.                   */
    VULKAN_IMAGE_TRANSITION_ATTACHMENTS_TO_RENDERING,    /**< Подготавливает буфер цвета и глубины для выполнения рендеринга.   */
    VULKAN_IMAGE_TRANSITION_ATTACHMENT_TO_PRESENT,       /**< Подготавливает буфер цвета для отображения на экран.              */
    VULKAN_IMAGE_TRANSITION_CUSTOM                       /**< Подготавливает указанные буферы по указанным описаниям переходов. */
} vulkan_image_transition_t;

/**
    @brief Описания пользовательского перевода layout-а изображения.
*/
typedef struct vulkan_image_transition_custom {
    VkImageLayout old_layout;                            /**< ... */
    VkImageLayout new_layout;                            /**< ... */
    VkPipelineStageFlags src_stage;                      /**< ... */
    VkAccessFlags src_access;                            /**< ... */
    VkPipelineStageFlags dst_stage;                      /**< ... */
    VkAccessFlags dst_access;                            /**< ... */
} vulkan_image_transition_custom_t;

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

/**
    @brief Контекст менеджера команд.
*/
typedef struct vulkan_command_manager {
    VkDevice device;                              /**< Используемое логическое устройство.   */
    VkQueue queue;                                /**< Очередь исполнения командных буферов. */
    VkCommandPool pool;                           /**< Пул для выделения командных буферов.  */
} vulkan_command_manager_t;

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

/**
    @brief Представляет шейдер.
*/
typedef struct vulkan_shader {
    VkPipelineLayout pipeline_layout;               /**< Макет конвейера (размещение дескрипторных наборов и push-констант).    */
    VkDescriptorSetLayout descriptor_set_layout;    /**< Макет дескрипторных наборов.                                           */
    VkPipeline pipeline;                            /**< Указатель на графический конвейер (скомпилированные шейдерные модули). */
    VkDescriptorPool descriptor_pool;               /**< Пул дескрипторов.                                                      */
    VkDescriptorSet descriptor_sets[3];             /**< Массив дескрипторных наборов (на кадр).                                */
    bool descriptor_set_updated[3];                 /**< Флаги обновления дескрипторных наборов (на дескриптор, на кадр).       */
} vulkan_shader_t;

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

    // @brief Менеджер графических команд.
    vulkan_command_manager_t graphics_command_manager;

    // @brief Буферы команд для графических операций (на кадр).
    VkCommandBuffer* graphics_command_buffers;
} vulkan_context;

// TODO: Временно.
typedef struct vulkan_texture_map {
    vulkan_image image;    // Сырые текстурные данные.
    VkSampler sampler;     // Карта использования текстурных данных + фильтрация.
} vulkan_texture_map_t;
