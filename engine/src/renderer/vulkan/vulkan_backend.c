#include "renderer/vulkan/vulkan_backend.h"
#include "renderer/vulkan/vulkan_types.h"
#include "renderer/vulkan/vulkan_result.h"
#include "renderer/vulkan/vulkan_window.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_shader.h"
#include "renderer/vulkan/vulkan_buffer.h"

#include "debug/assert.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/string.h"
#include "core/containers/darray.h"
#include "math/types.h"
#include "math/matrix.h"

static vulkan_context* context = nullptr;

static VkResult instance_create()
{
    // Проверка требований к версии Vulkan API.
    u32 instance_version;
    vkEnumerateInstanceVersion(&instance_version);

    u32 min_required_version = VK_API_VERSION_1_3;
    if(instance_version < min_required_version)
    {
        LOG_ERROR("Vulkan version %u.%u.%u is required, but only %u.%u.%u is available.",
             VK_VERSION_MAJOR(min_required_version), VK_VERSION_MINOR(min_required_version), VK_VERSION_PATCH(min_required_version),
             VK_VERSION_MAJOR(instance_version), VK_VERSION_MINOR(instance_version), VK_VERSION_PATCH(instance_version)
        );
        return false;
    }

    LOG_TRACE("Latest Vulkan API: %u.%u.%u",
        VK_VERSION_MAJOR(instance_version), VK_VERSION_MINOR(instance_version), VK_VERSION_PATCH(instance_version)
    );

    // TODO: Предпочтительно выбирать более новую версию, если доступно 1.3+, но если не, то 1.0-1.3 + необходимые расширения!
    LOG_TRACE("Chosen Vulkan API: %u.%u.%u",
        VK_VERSION_MAJOR(min_required_version), VK_VERSION_MINOR(min_required_version), VK_VERSION_PATCH(min_required_version)
    );

    // Определение количества и значений слоев проверки и расширений.
    u32 layer_count = 0;
    const char** layers = nullptr;

#ifdef DEBUG_FLAG
    // Получение списка необязательных слоев.
    layers = darray_create(const char*);
    darray_push(layers, &"VK_LAYER_KHRONOS_validation"); // Проверяет правильность использования Vulkan API.
    // NOTE: Раскомментировать для более детальной отладки, если необходимо.
    // darray_push(layers, &"VK_LAYER_LUNARG_api_dump");    // Выводит на консоль вызовы и передаваемые параметры.
    layer_count = darray_length(layers);

    // Получение списка доступных слоев.
    u32 available_layer_count;
    vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);
    VkLayerProperties* available_layers = darray_create_custom(VkLayerProperties, available_layer_count);
    vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers);

    // Проверка необязательных слоев.
    LOG_TRACE("Vulkan instance optional layers:");
    for(u32 i = 0; i < layer_count; ++i)
    {
        bool found = false;
        for(u32 j = 0; j < available_layer_count; ++j)
        {
            if(string_equal(layers[i], available_layers[j].layerName))
            {
                found = true;
                break;
            }
        }

        LOG_TRACE(" %s %s", found ? "+" : "-", layers[i]);
        if(!found)
        {
            darray_remove(layers, i, nullptr);
            layer_count--;
            i--;
        }
    }

    // Освобождение списка доступных слоев.
    darray_destroy(available_layers);

    // Обновление количества необязательных слоев.
    layer_count = darray_length(layers);
#endif

    // Получение списка обязательных расширений.
    u32 extension_count = 0;
    platform_window_enumerate_vulkan_extensions(&extension_count, nullptr);        // Получение количества расширений.
    const char** extensions = darray_create_custom(const char*, extension_count);
    platform_window_enumerate_vulkan_extensions(&extension_count, extensions);     // Платформо-зависимые расширения окна.
    darray_set_length(extensions, extension_count);                                // Обновление размеров массива.

    // Получение списка доступных расширений.
    u32 available_extension_count;
    vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr);
    VkExtensionProperties* available_extensions = darray_create_custom(VkExtensionProperties, available_extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, available_extensions);

    // Проверка обязательных расширений.
    LOG_TRACE("Vulkan instance required extensions:");
    for(u32 i = 0; i < extension_count; ++i)
    {
        bool found = false;
        for(u32 j = 0; j < available_extension_count; ++j)
        {
            if(string_equal(extensions[i], available_extensions[j].extensionName))
            {
                found = true;
                break;
            }
        }

        LOG_TRACE(" %s %s", found ? "+" : "-", extensions[i]);
        if(!found)
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

#ifdef DEBUG_FLAG
    // HACK: При отсутствии расшенения доп. сообщения должны быть выключены.
    context->debug_messenger_address_binding_report_using = true;

    // Добавление в список необязательных расширений.
    darray_push(extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);                   // Расширение для получения debug messenger.
    darray_push(extensions, &VK_EXT_DEVICE_ADDRESS_BINDING_REPORT_EXTENSION_NAME); // Расширение для доп. сообщений debug messenger.
    u32 prev_extension_count = extension_count;
    extension_count = darray_length(extensions);                                   // Обновление размеров массива.

    // Проверка необязательных расширений.
    LOG_TRACE("Vulkan instance optional extensions:");
    for(u32 i = prev_extension_count; i < extension_count; ++i)
    {
        bool found = false;
        for(u32 j = 0; j < available_extension_count; ++j)
        {
            if(string_equal(extensions[i], available_extensions[j].extensionName))
            {
                found = true;
                break;
            }
        }

        LOG_TRACE(" %s %s", found ? "+" : "-", extensions[i]);
        if(!found)
        {
            if(string_equal(extensions[i], VK_EXT_DEVICE_ADDRESS_BINDING_REPORT_EXTENSION_NAME))
            {
                context->debug_messenger_address_binding_report_using = false;
            }

            darray_remove(extensions, i, nullptr);
            extension_count--;
            i--;
        }
    }
#endif

    // Освобождение списка доступных расширений.
    darray_destroy(available_extensions);

    // Обновление количества обязательных + необяхательных расширений.
    extension_count = darray_length(extensions);

    // Создание экземпляра Vulkan.
    VkApplicationInfo application_info = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = platform_window_get_title(context->window),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "GameEngine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = min_required_version
    };

    VkInstanceCreateInfo instance_info = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &application_info,
        .enabledExtensionCount   = extension_count,
        .ppEnabledExtensionNames = extensions,
        .enabledLayerCount       = layer_count,
        .ppEnabledLayerNames     = layers
    };

    VkResult result = vkCreateInstance(&instance_info, context->allocator, &context->instance);

    // Освобождение списков слоев и расширений.
    if(layers)
    {
        darray_destroy(layers);
    }

    if(extensions)
    {
        darray_destroy(extensions);
    }

    return result;
}

static void instance_destroy()
{
    vkDestroyInstance(context->instance, context->allocator);
    context->instance = nullptr;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_handler(
    VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
    VkDebugUtilsMessageTypeFlagsEXT             message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*                                       user_data
)
{
    UNUSED(user_data);

    // Определение типа сообщения.
    const char* message_type = "UNKNOWN";
    if(message_types & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
    {
        message_type = "GENERAL";
    }
    else if(message_types & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
    {
        message_type = "VALIDATION";
    }
    else if(message_types & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
    {
        message_type = "PERFORMANCE";
    }
    else if(message_types & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
    {
        message_type = "DEVICE ADDRESS BINDING";
    }

    // Определение уровня важности.
    switch(message_severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            LOG_TRACE("[Vulkan %s]: %s.", message_type, callback_data->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            LOG_INFO("[Vulkan %s]: %s.", message_type, callback_data->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            LOG_WARN("[Vulkan %s]: %s.", message_type, callback_data->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: default:
            LOG_ERROR("[Vulkan %s]: %s.", message_type, callback_data->pMessage);

            // Вывод дополнительной информации по памяти.
            const char* memory_info = memory_system_usage_str();
            LOG_WARN(memory_info);
            string_free(memory_info);

            DEBUG_BREAK();
            return VK_TRUE; // Указывает прервать работу.
    }

    return VK_FALSE; // Указывает продолжать работу.
}

static VkResult debug_messenger_create()
{
    VkDebugUtilsMessengerCreateInfoEXT messenger_info = {
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
                         // NOTE: Раскомментировать для получения более детального вывода сообщений.
                         // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                         // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        .pfnUserCallback = debug_messenger_handler,
        .pUserData       = nullptr
    };

    // При наличии доп. расширения включить его сообщения.
    if(context->debug_messenger_address_binding_report_using)
    {
        messenger_info.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
    }

    PFN_vkCreateDebugUtilsMessengerEXT debug_messenger_create =
        (void*)vkGetInstanceProcAddr(context->instance, "vkCreateDebugUtilsMessengerEXT");

    if(!debug_messenger_create)
    {
        LOG_ERROR("Failed to get vkCreateDebugUtilsMessengerEXT function pointer.");
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    return debug_messenger_create(context->instance, &messenger_info, context->allocator, &context->debug_messenger);
}

static void debug_messenger_destroy()
{
    PFN_vkDestroyDebugUtilsMessengerEXT debug_messenger_destroy =
        (void*)vkGetInstanceProcAddr(context->instance, "vkDestroyDebugUtilsMessengerEXT");

    if(!debug_messenger_destroy)
    {
        LOG_ERROR("Failed to get vkDestroyDebugUtilsMessengerEXT function pointer.");
    }

    debug_messenger_destroy(context->instance, context->debug_messenger, context->allocator);
    context->debug_messenger = nullptr;
}

static bool device_create()
{
    // Получение физических устройств.
    u32 physical_device_count = 0;
    vulkan_device_enumerate_physical_devices(context, &physical_device_count, nullptr);
    vulkan_physical_device* physical_devices = darray_create_custom(vulkan_physical_device, physical_device_count);
    vulkan_device_enumerate_physical_devices(context, &physical_device_count, physical_devices);
    darray_set_length(physical_devices, physical_device_count);

    // Вывод информации о доступных устройствах.
    LOG_TRACE("----------------------------------------------------------");
    LOG_TRACE("Available vulkan physical devices (count %u):", physical_device_count);
    LOG_TRACE("----------------------------------------------------------");

    for(u32 i = 0; i < physical_device_count; ++i)
    {
        vulkan_physical_device* physical_device = &physical_devices[i];

        u32 api_major = VK_API_VERSION_MAJOR(physical_device->properties.apiVersion);
        u32 api_minor = VK_API_VERSION_MINOR(physical_device->properties.apiVersion);
        u32 api_patch = VK_API_VERSION_PATCH(physical_device->properties.apiVersion);
        u32 drv_major = VK_VERSION_MAJOR(physical_device->properties.driverVersion);
        u32 drv_minor = VK_VERSION_MINOR(physical_device->properties.driverVersion);
        u32 drv_patch = VK_VERSION_PATCH(physical_device->properties.driverVersion);

        LOG_TRACE("  Device type    : %s", vulkan_device_get_physical_device_type_str(physical_device->properties.deviceType));
        LOG_TRACE("  Device name    : %s", physical_device->properties.deviceName);
        LOG_TRACE("  Version api    : %u.%u.%u", api_major, api_minor, api_patch);
        LOG_TRACE("  Version driver : %u.%u.%u", drv_major, drv_minor, drv_patch);
        LOG_TRACE("  Vendor id      : 0x%x", physical_device->properties.vendorID);
        LOG_TRACE("  Device id      : 0x%x", physical_device->properties.deviceID);
        LOG_TRACE("  Graphics queue : count %u", physical_device->queue_graphics_count);
        LOG_TRACE("  Compute  queue : count %u", physical_device->queue_compute_count);
        LOG_TRACE("  Transfer queue : count %u", physical_device->queue_transfer_count);
        LOG_TRACE("  Present  queue : count %u", physical_device->queue_present_count);

        u32 memory_heap_count = physical_device->memory_properties.memoryHeapCount;
        for(u32 j = 0; j < memory_heap_count; ++j)
        {
            memory_format heap_fm;
            memory_get_format(physical_device->memory_properties.memoryHeaps[j].size, &heap_fm);

            if(physical_device->memory_properties.memoryHeaps[j].flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            {
                LOG_TRACE("  Local memory   : %.2f%s", heap_fm.amount, heap_fm.unit);
            }
            else
            {
                LOG_TRACE("  Shared memory  : %.2f%s", heap_fm.amount, heap_fm.unit);
            }
        }

        LOG_TRACE("----------------------------------------------------------");
    }

    vulkan_physical_device* physical_device_selected = nullptr;

    // TODO: Выбор устройства (+ сценарии выбора).
    for(u32 i = 0; i < physical_device_count; ++i)
    {
        // Предпочтительнее дискретная.
        if(physical_devices[i].properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
        || (physical_devices[i].properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && physical_device_selected == nullptr))
        {
            physical_device_selected = &physical_devices[i];
        }
    }

    if(physical_device_selected == nullptr)
    {
        LOG_ERROR("No physical devices were found.");
        darray_destroy(physical_devices);
        return false;
    }
    LOG_TRACE("Selected physical device named: %s.", physical_device_selected->properties.deviceName);

    // Создание списка расширений устройства.
    const char** device_extensions = darray_create(const char*);
    darray_push(device_extensions, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    // TODO: Определить когда включать! Но похоже необходим для версии ниже 1.3 + необходимо получать указатели на функции!
    // darray_push(device_extensions, &VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    // darray_push(device_extensions, &VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);

    // Конфигурация устройства.
    // TODO: Настраиваемое.
    vulkan_device_config device_cfg = {
        .device_type = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
        .extension_count = darray_length(device_extensions),
        .extensions = device_extensions,
        .use_sampler_anisotropy = true
    };

    bool result = vulkan_device_create(context, physical_device_selected, &device_cfg, &context->device);

    // Оcвобождение временного списка устройств и расширений устройства.
    darray_destroy(physical_devices);
    darray_destroy(device_extensions);

    return result;
}

static void device_destroy()
{
    if(context->device.logical == nullptr)
    {
        LOG_WARN("Some resources were not allocated, skipped...");
        return;
    }

    vulkan_device_destroy(context, &context->device);
}

// TODO: Количество изображений цепочки обмена может измениться, а потому реализовать пересоздание.
static bool command_buffers_create()
{
    context->graphics_command_buffers = darray_create_custom(VkCommandBuffer, context->swapchain.max_frames_in_flight);

    VkCommandBufferAllocateInfo allocate_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = context->device.graphics_queue.command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = context->swapchain.max_frames_in_flight
    };

    VkResult result = vkAllocateCommandBuffers(context->device.logical, &allocate_info, context->graphics_command_buffers);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to allocate command buffers: %s.", vulkan_result_get_string(result));
        return false;
    }

    return true;
}

static void command_buffers_destroy()
{
    if(!context->graphics_command_buffers)
    {
        return;
    }

    vkFreeCommandBuffers(
        context->device.logical, context->device.graphics_queue.command_pool, context->swapchain.max_frames_in_flight, context->graphics_command_buffers
    );

    darray_destroy(context->graphics_command_buffers);
    context->graphics_command_buffers = nullptr;
}

static bool sync_objects_create()
{
    VkDevice logical = context->device.logical;
    u32 image_count = context->swapchain.image_count;
    u8 max_frames_in_flight = context->swapchain.max_frames_in_flight;

    // NOTE: Fence используются для сообщения программе о завершении отрисовки кадра в изображение (GPU->CPU).
    //       Semaphore используются для синхронизации между командами в командном буфере (только на GPU стороне).

    // Выделение памяти под семафоры и барьеры.
    context->image_available_semaphores = darray_create_custom(VkSemaphore, max_frames_in_flight);
    context->in_flight_fences           = darray_create_custom(VkFence, max_frames_in_flight);

    // TODO: Так как цепочка обмена может изменить количество изображений, организовать пересоздание массивов.
    context->image_complete_semaphores  = darray_create_custom(VkSemaphore, image_count);
    context->images_in_flight           = darray_create_custom(VkFence, image_count);

    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    // NOTE: По умолчанию создается в сигнальном состоянии, что будет говорить о том,
    //       что изображения цепочки обмена в данный момент свободны и готовы.
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for(u8 i = 0; i < max_frames_in_flight; ++i)
    {
        VkResult result = vkCreateSemaphore(logical, &semaphore_info, context->allocator, &context->image_available_semaphores[i]);
        if(!vulkan_result_is_success(result))
        {
            LOG_ERROR("Failed to create 'image available semaphore %u': %s.", i, vulkan_result_get_string(result));
            return false;
        }

        result = vkCreateFence(logical, &fence_info, context->allocator, &context->in_flight_fences[i]);
        if(!vulkan_result_is_success(result))
        {
            LOG_ERROR("Failed to create 'in flight fence %u': %s.", i, vulkan_result_get_string(result));
            return false;
        }
    }

    for(u32 i = 0; i < image_count; ++i)
    {
        VkResult result = vkCreateSemaphore(logical, &semaphore_info, context->allocator, &context->image_complete_semaphores[i]);
        if(!vulkan_result_is_success(result))
        {
            LOG_ERROR("Failed to create 'image complete semaphore %u': %s.", i, vulkan_result_get_string(result));
            return false;
        }
    }

    mzero(context->images_in_flight, sizeof(VkFence) * image_count);
    return true;
}

static void sync_objects_destroy()
{
    if(!context->image_available_semaphores || !context->image_complete_semaphores || !context->in_flight_fences || !context->images_in_flight)
    {
        LOG_WARN("Some resources were not allocated, there may be errors!!!");
    }

    VkDevice logical = context->device.logical;
    u32 image_count = context->swapchain.image_count;
    u8 max_frames_in_flight = context->swapchain.max_frames_in_flight;

    for(u8 i = 0; i < max_frames_in_flight; ++i)
    {
        vkDestroySemaphore(logical, context->image_available_semaphores[i], context->allocator);
        vkDestroyFence(logical, context->in_flight_fences[i], context->allocator);
    }

    for(u32 i = 0; i < image_count; ++i)
    {
        vkDestroySemaphore(logical, context->image_complete_semaphores[i], context->allocator);
    }

    darray_destroy(context->image_available_semaphores);
    darray_destroy(context->image_complete_semaphores);
    darray_destroy(context->in_flight_fences);
    darray_destroy(context->images_in_flight);
    context->image_available_semaphores = nullptr;
    context->image_complete_semaphores  = nullptr;
    context->in_flight_fences = nullptr;
    context->images_in_flight = nullptr;
}

bool vertex_buffers_create()
{
    const u64 vertex_buffer_size  = sizeof(vertex3d) * 1000000;
    const u64 index_buffer_size   = sizeof(u32) * 1000000;
    context->vertex_buffer_offset = 0;
    context->index_buffer_offset  = 0;

    if(!vulkan_buffer_create(context, VULKAN_BUFFER_TYPE_VERTEX, vertex_buffer_size, &context->vertex_buffer))
    {
        LOG_ERROR("Failed to create vertex buffer.");
        return false;
    }

    if(!vulkan_buffer_create(context, VULKAN_BUFFER_TYPE_INDEX, index_buffer_size, &context->index_buffer))
    {
        LOG_ERROR("Failed to create index buffer.");
        return false;
    }

    return true;
}

void vertex_buffers_destroy()
{
    vulkan_buffer_destroy(context, &context->vertex_buffer);
    vulkan_buffer_destroy(context, &context->index_buffer);
}

bool vulkan_backend_initialize(platform_window* window)
{
    ASSERT(context == nullptr, "Vulkan backend is already initialized.");

    context = mallocate(sizeof(vulkan_context), MEMORY_TAG_RENDERER);
    if(!context)
    {
        LOG_ERROR("Failed to allocate memory for vulkan context.");
        return false;
    }
    mzero(context, sizeof(vulkan_context));

    // TODO: Реализовать кастомный аллокатор.
    context->allocator = nullptr;

    // Сохранение контекста связанного окна.
    context->window = window;
    u32 framebuffer_width = 0;
    u32 framebuffer_height = 0;

    // TODO: Проблема перед созданием рендерера, wayland окно появляется только когда создан буфер
    //       и из-за особенностей системы (в данном случае стековой, размер выбирается автоматически другой).
    platform_window_get_resolution(window, &framebuffer_width, &framebuffer_height);
    context->frame_width = framebuffer_width > 0 ? framebuffer_width : 1280;
    context->frame_height = framebuffer_height > 0 ? framebuffer_height : 768;
    context->frame_generation = 0;
    context->frame_pending_width = context->frame_width;
    context->frame_pending_height = context->frame_height;
    context->frame_pending_generation = context->frame_generation;

    VkResult result = instance_create();
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create vulkan instance: %s.", vulkan_result_get_string(result));
        return false;
    }
    LOG_TRACE("Vulkan instance created successfully.");

#ifdef DEBUG_FLAG
    result = debug_messenger_create();
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create vulkan debug messenger: %s.", vulkan_result_get_string(result));
        return false;
    }
    LOG_TRACE("Vulkan debug messenger created successfully.");
#endif

    result = platform_window_create_vulkan_surface(context->window, context->instance, context->allocator, (void**)&context->surface);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create vulkan surface: %s.", vulkan_result_get_string(result));
        return false;
    }
    LOG_TRACE("Vulkan surfcae created successfully.");

    if(!device_create())
    {
        LOG_ERROR("Failed to create vulkan device.");
        return false;
    }
    LOG_TRACE("Vulkan device created successfully.");

    if(!vulkan_swapchain_create(context, context->frame_width, context->frame_height, &context->swapchain))
    {
        LOG_ERROR("Failed to create vulkan swapchain.");
        return false;
    }
    LOG_TRACE("Vulkan swapchain created successfully.");

    if(!sync_objects_create())
    {
        LOG_ERROR("Failed to create synchronization objects.");
        return false;
    }
    LOG_TRACE("Vulkan synchronization objects created successfully.");

    if(!command_buffers_create())
    {
        LOG_ERROR("Failed to create graphics command buffer.");
        return false;
    }
    LOG_TRACE("Vulkan graphics command buffers created successfully.");

    // TODO: Временно!
    if(!vulkan_shader_create(context, &context->world_shader))
    {
        LOG_ERROR("Failed to load world shader.");
        return false;
    }
    LOG_TRACE("Vulkan world shader created successfully.");

    // TODO: Временно!
    if(!vertex_buffers_create())
    {
        LOG_ERROR("Failed to create vertex buffers.");
        return false;
    }
    LOG_TRACE("Vulkan buffers created successfully.");

    // TODO: Временно!
    vertex3d verts[] = {
        { {{ -0.5, -0.5, 0.0 }}, {{ 1.0, 0.0, 0.0, 1.0 }} }, // 0
        { {{  0.5,  0.5, 0.0 }}, {{ 0.0, 1.0, 0.0, 1.0 }} }, // 1
        { {{ -0.5,  0.5, 0.0 }}, {{ 0.0, 0.0, 1.0, 1.0 }} }, // 2
        { {{  0.5, -0.5, 0.0 }}, {{ 1.0, 1.0, 0.0, 1.0 }} }, // 3
    };
    if(!vulkan_buffer_load_data(context, &context->vertex_buffer, 0, sizeof(verts), verts))
    {
        LOG_ERROR("Failed to load verts data.");
        return false;
    }

    u32 indices[] = {0, 1, 2, 0, 3, 1};
    if(!vulkan_buffer_load_data(context, &context->index_buffer, 0, sizeof(indices), indices))
    {
        LOG_ERROR("Failed to load indices data.");
        return false;
    }

    // TODO: Временно!
    const f32 fov = math_deg_to_rad(60);
    const f32 aspect = (f32)context->frame_width / context->frame_height;
    context->camera.proj = mat4_perspective(fov, aspect, 0.1f, 1000.0f);

    // NOTE: Это не позиция самой камеры, а дополнительное смещение всех вершин мира,
    //       что можно трактовать как перемещение камеры от центра мира.
    context->camera.view = mat4_translation(vec3_forward());

    LOG_TRACE("Vulkan backend initialized successfully.");
    return true;
}

void vulkan_backend_shutdown()
{
    ASSERT(context != nullptr, "Vulkan backend should be initialized.");

    VkResult result = vkDeviceWaitIdle(context->device.logical);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to wait device operations.");
    }

    // TODO: Временно!
    vertex_buffers_destroy();
    LOG_TRACE("Vulkan vertex buffers destroy complete.");

    // TODO: Временно!
    vulkan_shader_destroy(context, &context->world_shader);
    LOG_TRACE("Vulkan world shader destroy complete.");

    command_buffers_destroy();
    LOG_TRACE("Vulkan graphics command buffers destroy complete.");

    sync_objects_destroy();
    LOG_TRACE("Vulkan synchronization objects destroy complete.");

    vulkan_swapchain_destroy(context, &context->swapchain);
    LOG_TRACE("Vulkan swapchain destroy complete.");

    device_destroy();
    LOG_TRACE("Vulkan device destroy complete.");

    if(context->surface)
    {
        platform_window_destroy_vulkan_surface(context->window, context->instance, context->allocator, context->surface);
        context->surface = nullptr;
        LOG_TRACE("Vulkan surface destroy complete.");
    }

    if(context->debug_messenger)
    {
        debug_messenger_destroy();
        LOG_TRACE("Vulkan debug messenger destroy complete.");
    }

    if(context->instance)
    {
        instance_destroy();
        LOG_TRACE("Vulkan instance destroy complete.");
    }

    mfree(context, sizeof(vulkan_context), MEMORY_TAG_RENDERER);
    context = nullptr;

    LOG_TRACE("Vulkan backend shutdown complete.");
}

bool vulkan_backend_is_supported()
{
    u32 instance_version;
    VkResult result = vkEnumerateInstanceVersion(&instance_version);
    if(!vulkan_result_is_success(result))
    {
        LOG_TRACE("Vulkan is not supported by the system: %s.", vulkan_result_get_string(result));
        return false;
    }

    VkApplicationInfo application_info = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "Vulkan Support Check",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = instance_version
    };

    VkInstanceCreateInfo instance_info = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &application_info,
        .enabledExtensionCount   = 0,
        .ppEnabledExtensionNames = nullptr,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = nullptr
    };

    VkInstance instance;
    result = vkCreateInstance(&instance_info, nullptr, &instance);
    if(!vulkan_result_is_success(result))
    {
        LOG_TRACE("Failed to create Vulkan instance: %s.", vulkan_result_get_string(result));
        return false;
    }

    u32 device_count = 0;
    result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if(!vulkan_result_is_success(result))
    {
        LOG_TRACE("Failed to enumerate physical devices: %s.", vulkan_result_get_string(result));
        vkDestroyInstance(instance, nullptr);
        return false;
    }

    vkDestroyInstance(instance, nullptr);

    if(device_count == 0)
    {
        LOG_TRACE("No Vulkan compatible graphics cards found.");
        return false;
    }

    return true;
}

void vulkan_backend_resize(const u32 width, const u32 height)
{
    context->frame_pending_width = width;
    context->frame_pending_height = height;
    context->frame_pending_generation++;
    LOG_TRACE("Vulkan resize event to %ux%u, generation: %u.", width, height, context->frame_pending_generation);

    // TODO: Временно для камеры!
    const f32 aspect = (f32)width / height;
    mat4_perspective_update_aspect(&context->camera.proj, aspect);
}

bool vulkan_backend_frame_begin()
{
    // Обнаружение изменения размеров окна.
    if(context->frame_generation != context->frame_pending_generation)
    {
        if(context->frame_pending_width < 1 || context->frame_pending_height < 1)
        {
            LOG_ERROR("%s called when the window size is less than 1.", __func__);
            return false;
        }

        if(!vulkan_swapchain_recreate(context, context->frame_pending_width, context->frame_pending_height, &context->swapchain))
        {
            LOG_ERROR("Failed to recreate swapchain.");
            return false;
        }

        // Освобождение изображений.
        mzero(context->images_in_flight, sizeof(VkFence) * context->swapchain.image_count);

        // Применение изменений цепочки обмена.
        context->frame_width = context->frame_pending_width;
        context->frame_height = context->frame_pending_height;
        context->frame_generation = context->frame_pending_generation;

        LOG_DEBUG("Swapchain recreate complete.");
        return false;
    }

    // Ожидание завершения предыдущего кадра (чтобы можно было переиспользовать его командный буфер).
    u32 current_frame = context->swapchain.current_frame;
    VkResult result = vkWaitForFences(context->device.logical, 1, &context->in_flight_fences[current_frame], VK_TRUE, U64_MAX);
    if(!vulkan_result_is_success(result))
    {
        LOG_FATAL("Failed to wait in-flight fence: %s.", vulkan_result_get_string(result));
    }

    // Получение индекса сделующего изображения из цепочки обмена.
    // NOTE: Сохранение индекса в context->swapchain.image_index переменную необходимо, для **end_frame!
    // NOTE: То что семафор image_available_semaphore[current_frame] свободен для использования гарантируется
    //       благодаря in_flight_fences[current_frame], который проверяется выше.
    if(!vulkan_swapchain_acquire_next_image_index(
        context, &context->swapchain, context->image_available_semaphores[current_frame], nullptr, U64_MAX, &context->swapchain.image_index
    ))
    {
        LOG_ERROR("Failed to acquire next image index.");
        return false;
    }
    u32 image_index = context->swapchain.image_index;

    // Получение командного буфера.
    // NOTE: То что командный буфер graphics_command_buffers[current_frame] свободен для использования гарантируется
    //       благодаря in_flight_fences[current_frame], который проверяется выше.
    VkCommandBuffer cmdbuf = context->graphics_command_buffers[current_frame];

    // До того, как будет использоваться командный буфер, его нужно вернуть в изначальное состояние.
    result = vkResetCommandBuffer(cmdbuf, 0);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to reset command buffer: %s (current index: %u).", vulkan_result_get_string(result), current_frame);
        return false;
    }

    // Начало записи команд в текущий буфер команд кадра.
    VkCommandBufferBeginInfo cmdbuf_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0 // Указывает на использование командного буфера более одного раза.
    };

    result = vkBeginCommandBuffer(cmdbuf, &cmdbuf_begin_info);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to start recording to the command buffer: %s (current index: %u).", vulkan_result_get_string(result), current_frame);
        return false;
    }

    // TODO: При использовании graphics совместно с present вполне рабочий способ, однако как действовать,
    //       когда graphics и present работают параллельно?

    // Перевод layout изображения в COLOR_ATTACHMENT_OPTIMAL.
    VkImageMemoryBarrier color_barrier = {
        .sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask                   = 0,
        .dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .image                           = context->swapchain.images[image_index],
        .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel   = 0,
        .subresourceRange.levelCount     = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount     = 1
    };

    // TODO: Перенести изменение лайаутов для буфера глубины только при создании/пересоздании цепочки обмена!
    // Перевод layout буфера глубины в DEPTH_ATTACHMENT_OPTIMAL.
    VkImageMemoryBarrier depth_barrier = {
        .sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask                   = 0,
        .dstAccessMask                   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout                       = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .image                           = context->swapchain.depth_image.handle,
        .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
        .subresourceRange.baseMipLevel   = 0,
        .subresourceRange.levelCount     = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount     = 1
    };

    VkImageMemoryBarrier image_barriers[] = {
        color_barrier,
        depth_barrier
    };

    const u32 image_barrier_count = ARRAY_SIZE(image_barriers);

    // NOTE: Перед рендерингом в изображение swapchain'а и выполнения теста глубины нужно перевести их в правильные
    //       layout'ы. Для изображения цепиочки обмена и буфера глубины записывается команда барьера памяти, которая
    //       гарантирует, что все команды, которые используют эти изображения и записаны ПОСЛЕ барьера, будут ждать
    //       на указанных стадиях (для каждоый команды своя стадия COLOR_ATTACHMENT_OUTPUT | EARLY_FRAGMENT_TESTS),
    //       пока команды ДО барьера, которые также используют эти же изображения, не завершат стадию TOP_OF_PIPE
    //       и/или не будут переведены layout'ы изображений (oldLayout -> newLayout).
    // NOTE: В данном случае, команд ДО барьера работающих с этими изображениями нет, а потому команды ПОСЛЕ барьера
    //       будут ожидать только изменения layout'ов.
    vkCmdPipelineBarrier(
        cmdbuf,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        image_barrier_count, image_barriers
    );

    VkClearValue color_clear_value = { .color = {{0.01f, 0.01f, 0.01f, 1.0f }} };
    VkClearValue depth_clear_color = { .depthStencil = { 1.0f, 0 } };

    VkRenderingAttachmentInfo color_attachment = {
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView   = context->swapchain.image_views[image_index],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue  = color_clear_value
    };

    VkRenderingAttachmentInfo depth_attachment = {
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView   = context->swapchain.depth_image.view,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue  = depth_clear_color
    };

    VkRenderingInfo rendering_info = {
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea           = { {0, 0}, {context->frame_width, context->frame_height} },
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &color_attachment,
        .pDepthAttachment     = &depth_attachment
    };

    // Начало динамического рендеринга.
    vkCmdBeginRendering(cmdbuf, &rendering_info);

    // TODO: Временно. Начало.

    // Установка viewport и scissor.
    // NOTE: Это переворачивает направление оси Y вверх.
    VkViewport viewport = {
        .x = 0.0f,
        .y = (f32)context->frame_height,
        .width = (f32)context->frame_width,
        .height = -(f32)context->frame_height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = {context->frame_width, context->frame_height}
    };

    // NOTE: При установке количества viewport и scissor их количества должны точно соответствовать заданным для конвейера!
    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

    // Использование конвейера.
    vulkan_shader_use(context, &context->world_shader);

    // Обновление данных камеры и применение их.
    vulkan_shader_update_camera(context, &context->world_shader, &context->camera);

    // Использование буферов для рисования.
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmdbuf, 0, 1, &context->vertex_buffer.handle, &offset);
    vkCmdBindIndexBuffer(cmdbuf, context->index_buffer.handle, offset, VK_INDEX_TYPE_UINT32);

    // vkCmdDraw(cmdbuf, 6, 1, 0, 0);
    vkCmdDrawIndexed(cmdbuf, 6, 1, 0, 0, 0);

    // TODO: Временно. Завершение.

    return true;
}

bool vulkan_backend_frame_end()
{
    VkDevice logical = context->device.logical;
    u32 image_index = context->swapchain.image_index;
    u32 current_frame = context->swapchain.current_frame;
    VkCommandBuffer cmdbuf = context->graphics_command_buffers[current_frame];

    // Завершение динамического рендеринга.
    vkCmdEndRendering(cmdbuf);

    // Перевод layout изображения в PRESENT_SRC.
    VkImageMemoryBarrier color_barrier = {
        .sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask                   = 0,
        .oldLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image                           = context->swapchain.images[image_index],
        .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel   = 0,
        .subresourceRange.levelCount     = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount     = 1
    };

    // NOTE: Необходимости в переводе layout для буфера глубины нет, т.к. он нужен только на этапе
    //       тестирования глубины.

    vkCmdPipelineBarrier(
        cmdbuf, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &color_barrier
    );

    // Завершение записи команд в текущий буфер команд кадра.
    VkResult result = vkEndCommandBuffer(cmdbuf);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to end recording to the command buffer: %s (current index: %u).", vulkan_result_get_string(result), current_frame);
        return false;
    }

    // Проверка изображения цепочки обмена, что оно еще не используется.
    // NOTE: См. https://docs.vulkan.org/spec/latest/chapters/VK_KHR_surface/wsi.html#vkQueuePresentKHR
    //       Приложение не обязано отображать изображения в том же порядке, в котором они были получены
    //       - приложения могут произвольно отображать любое изображение, полученное в данный момент.
    if(context->images_in_flight[image_index] != nullptr)
    {
        result = vkWaitForFences(logical, 1, &context->images_in_flight[image_index], VK_TRUE, U64_MAX);
        if(!vulkan_result_is_success(result))
        {
            LOG_ERROR("Failed to wait image in flight fence: %s.", vulkan_result_get_string(result));
        }
    }

    // Задаем барьер для текущего изображения.
    context->images_in_flight[image_index] = context->in_flight_fences[current_frame];

    // Сбрасывание барьера для текущего кадра.
    result = vkResetFences(logical, 1, &context->in_flight_fences[current_frame]);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to reset in flight fence: %s", vulkan_result_get_string(result));
        return false;
    }

    // Указание ожидаемого состояние пайплайна.
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    // Отправление на выполнение записанного буфера команд.
    // NOTE: Может возникнуть ошибка VUID-vkQueueSubmit-pSignalSemaphores-00067 смотри в описании 'представления
    //       изображения на экран'.
    VkSubmitInfo submit_info = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &context->image_available_semaphores[current_frame],
        .pWaitDstStageMask    = &wait_stage,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &context->image_complete_semaphores[image_index],
        .commandBufferCount   = 1,
        .pCommandBuffers      = &cmdbuf,
    };

    VkQueue graphics_queue = context->device.graphics_queue.handle;
    result = vkQueueSubmit(graphics_queue, 1, &submit_info, context->in_flight_fences[current_frame]);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to submit queue: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Представление изображения на экран.
    // NOTE: См. https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
    //       vkQueuePresentKHR отличается от vkQueueSubmit тем, что не предоставляет способа сигнализировать о
    //       завершении работы. Это означает что мы не знаем используется ли текущий image_complete_semaphore[]
    //       или нет, но есть косвенный способ узнать о завершении благодаря vkAcquireNextImageKHR, т.е. через
    //       ожидание текущего image_available_semaphore[], указывающий, что операция представления изображения
    //       с полученным индексом завершилась.
    VkQueue present_queue = context->device.present_queue.handle;
    vulkan_swapchain_present(context, &context->swapchain, present_queue, context->image_complete_semaphores[image_index], image_index);

    return true;
}
