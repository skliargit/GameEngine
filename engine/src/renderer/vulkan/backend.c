#include "renderer/vulkan/backend.h"
#include "renderer/vulkan/types.h"
#include "renderer/vulkan/result.h"
#include "renderer/vulkan/window.h"
#include "renderer/vulkan/device.h"
#include "renderer/vulkan/swapchain.h"
#include "renderer/vulkan/command.h"
#include "renderer/vulkan/image.h"
#include "renderer/vulkan/utils.h"

#include "debug/assert.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/string.h"
#include "core/containers/darray.h"
#include "platform/file.h"

// TODO: В отдельный файл.
// #include "vendor/spirv_reflect.h"

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
    vulkan_command_buffer_create(&context->graphics_command_manager, context->swapchain.max_frames_in_flight, context->graphics_command_buffers);
    return true;
}

static void command_buffers_destroy()
{
    if(context->graphics_command_buffers)
    {
        vulkan_command_buffer_destroy(&context->graphics_command_manager, context->swapchain.max_frames_in_flight, context->graphics_command_buffers);
        darray_destroy(context->graphics_command_buffers);
        context->graphics_command_buffers = nullptr;
    }
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

    LOG_TRACE("Vulkan backend initialized successfully.");
    return true;
}

void vulkan_backend_shutdown()
{
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

void vulkan_wait_idle_device()
{
    VkResult result = vkDeviceWaitIdle(context->device.logical);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to wait device operations.");
    }
}

void vulkan_frame_resize(const u32 width, const u32 height)
{
    context->frame_pending_width = width;
    context->frame_pending_height = height;
    context->frame_pending_generation++;
    LOG_TRACE("Vulkan resize event to %ux%u, generation: %u.", width, height, context->frame_pending_generation);
}

bool vulkan_frame_begin()
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

    // Получение командного буфера, обнуление для повторного использования и начало записи команд текущего кадра.
    // NOTE: То что командный буфер graphics_command_buffers[current_frame] свободен для использования гарантируется
    //       благодаря in_flight_fences[current_frame], который проверяется выше.
    VkCommandBuffer cmdbuf = context->graphics_command_buffers[current_frame];
    vulkan_command_buffer_reset(cmdbuf);
    vulkan_command_buffer_begin(cmdbuf, 0);

    u32 image_index = context->swapchain.image_index;

    // NOTE: Перед рендерингом в изображение swapchain'а и выполнения теста глубины нужно перевести их в правильные
    //       layout'ы. Для изображения цепиочки обмена и буфера глубины записывается команда барьера памяти, которая
    //       гарантирует, что все команды, которые используют эти изображения и записаны ПОСЛЕ барьера, будут ждать
    //       на указанных стадиях (для каждой команды своя стадия COLOR_ATTACHMENT_OUTPUT | EARLY_FRAGMENT_TESTS),
    //       пока команды ДО барьера, которые также используют эти же изображения, не завершат стадию TOP_OF_PIPE
    //       и/или не будут переведены layout'ы изображений (oldLayout -> newLayout). Так как команд ДО барьера
    //       работающих с этими изображениями нет, то команды ПОСЛЕ барьера будут ожидать только изменения layout'ов.
    // TODO: Перенести изменение лайаутов для буфера глубины только при создании/пересоздании цепочки обмена!
    VkImage images[] = { context->swapchain.images[image_index], context->swapchain.depth_image.handle };
    vulkan_image_transition_layout(cmdbuf, VULKAN_IMAGE_TRANSITION_ATTACHMENTS_TO_RENDERING, images);

    // TODO: Обернуть начало и конец рендеринга?
    VkRenderingAttachmentInfo color_attachment = {
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView   = context->swapchain.image_views[image_index],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue  =  (VkClearValue){ .color = {{ 0.01f, 0.01f, 0.01f, 1.0f }}} // Цвет очистки.
    };

    VkRenderingAttachmentInfo depth_attachment = {
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView   = context->swapchain.depth_image.view,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue  = (VkClearValue){ .depthStencil = { 1.0f, 0 }}               // Значение глубины.
    };

    VkRenderingInfo rendering_info = {
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea           = {{0, 0}, {context->frame_width, context->frame_height}},
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
    // TODO: Временно. Завершение.

    return true;
}

bool vulkan_frame_end()
{
    VkDevice logical = context->device.logical;
    u32 image_index = context->swapchain.image_index;
    u32 current_frame = context->swapchain.current_frame;
    VkCommandBuffer cmdbuf = context->graphics_command_buffers[current_frame];

    // Завершение динамического рендеринга.
    vkCmdEndRendering(cmdbuf);

    // Перевод layout изображения в PRESENT_SRC.
    // NOTE: Необходимости в переводе layout для буфера глубины нет, т.к. он нужен только на этапе
    //       тестирования глубины.
    VkImage images[] = { context->swapchain.images[image_index] };
    vulkan_image_transition_layout(cmdbuf, VULKAN_IMAGE_TRANSITION_ATTACHMENT_TO_PRESENT, images);

    // Завершение записи команд в текущий буфер команд кадра.
    vulkan_command_buffer_end(cmdbuf);

    // Проверка изображения цепочки обмена, что оно еще не используется.
    // NOTE: См. https://docs.vulkan.org/spec/latest/chapters/VK_KHR_surface/wsi.html#vkQueuePresentKHR
    //       Приложение не обязано отображать изображения в том же порядке, в котором они были получены
    //       - приложения могут произвольно отображать любое изображение, полученное в данный момент.
    if(context->images_in_flight[image_index] != nullptr)
    {
        VkResult result = vkWaitForFences(logical, 1, &context->images_in_flight[image_index], VK_TRUE, U64_MAX);
        if(!vulkan_result_is_success(result))
        {
            LOG_ERROR("Failed to wait image in flight fence: %s.", vulkan_result_get_string(result));
        }
    }

    // Задаем барьер для текущего изображения.
    context->images_in_flight[image_index] = context->in_flight_fences[current_frame];

    // Сбрасывание барьера для текущего кадра.
    VkResult result = vkResetFences(logical, 1, &context->in_flight_fences[current_frame]);
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
    vulkan_command_buffer_submit(
        &context->graphics_command_manager, 1, &cmdbuf, 1, &context->image_available_semaphores[current_frame],
        &wait_stage, 1, &context->image_complete_semaphores[image_index], context->in_flight_fences[current_frame]
    );

    // Представление изображения на экран.
    // NOTE: См. https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
    //       vkQueuePresentKHR отличается от vkQueueSubmit тем, что не предоставляет способа сигнализировать о
    //       завершении работы. Это означает что мы не знаем используется ли текущий image_complete_semaphore[]
    //       или нет, но есть косвенный способ узнать о завершении благодаря vkAcquireNextImageKHR, т.е. через
    //       ожидание текущего image_available_semaphore[], указывающий, что операция представления изображения
    //       с полученным индексом завершилась.
    vulkan_swapchain_present(
        context, &context->swapchain, context->device.present_queue.handle, context->image_complete_semaphores[image_index],
        image_index
    );

    return true;
}

void vulkan_frame_bind_shader(shader_t* shader)
{
    u32 current_frame = context->swapchain.current_frame;
    VkCommandBuffer cmdbuf = context->graphics_command_buffers[current_frame];
    vulkan_shader_t* vk_shader = shader->internal_data;

    // TODO: Настраиваемая точка привязки (графика или вычисления).
    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline);
}

void vulkan_frame_bind_buffer(buffer_t* buffer, const usize buffer_offset)
{
    u32 current_frame = context->swapchain.current_frame;
    VkCommandBuffer cmdbuf = context->graphics_command_buffers[current_frame];
    vulkan_buffer_t* vk_buffer = buffer->internal_data;

    switch(buffer->type)
    {
        case BUFFER_TYPE_VERTEX:
            // TODO: Настраиваемая.
            vkCmdBindVertexBuffers(cmdbuf, 0, 1, &vk_buffer->handle, &buffer_offset);
            break;
        case BUFFER_TYPE_INDEX:
            // TODO: Настраиваемая.
            vkCmdBindIndexBuffer(cmdbuf, vk_buffer->handle, buffer_offset, VK_INDEX_TYPE_UINT32);
            break;
        case BUFFER_TYPE_UNIFORM:
        case BUFFER_TYPE_STAGING:
        case BUFFER_TYPE_READ:
        case BUFFER_TYPE_STORAGE:
            LOG_ERROR("Current buffer binding is not supported.");
            break;
        }
}

void vulkan_frame_draw(const u32 vertex_count)
{
    u32 current_frame = context->swapchain.current_frame;
    VkCommandBuffer cmdbuf = context->graphics_command_buffers[current_frame];
    vkCmdDraw(cmdbuf, vertex_count, 1, 0, 0);
}

void vulkan_frame_draw_indexed(const u32 index_count)
{
    u32 current_frame = context->swapchain.current_frame;
    VkCommandBuffer cmdbuf = context->graphics_command_buffers[current_frame];
    vkCmdDrawIndexed(cmdbuf, index_count, 1, 0, 0, 0);
}

// Указывает на использование только локальной памяти (VRAM only).
static inline bool buffer_is_device_local_only(vulkan_buffer_t* vk_buffer)
{
    return (vk_buffer->memory_property_flags & (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
}

static void buffer_copy_range(vulkan_buffer_t* src, usize src_offset, vulkan_buffer_t* dst, usize dst_offset, usize size)
{
    VkBufferCopy region = {
        .srcOffset = src_offset,
        .dstOffset = dst_offset,
        .size      = size
    };

    VkCommandBuffer cmdbuf;
    vulkan_command_buffer_begin_single_use(&context->graphics_command_manager, &cmdbuf);
    vkCmdCopyBuffer(cmdbuf, src->handle, dst->handle, 1, &region);
    vulkan_command_buffer_end_single_use(&context->graphics_command_manager, cmdbuf);

    // TODO:
    // Барьер гарантирует, что операции после него получат обновленный буфер данных на необходимой стадии.
    // VkBufferMemoryBarrier buffer_barrier = {
    //     .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
    //     .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
    //     .dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT,
    //     .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //     .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //     .buffer              = dst->handle,
    //     .offset              = dst_offset,
    //     .size                = size
    // };

    // switch(dst->type)
    // {
    //     case VULKAN_BUFFER_TYPE_VERTEX:
    //     case VULKAN_BUFFER_TYPE_INDEX:
    //         buffer_barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    //         break;
    //     case VULKAN_BUFFER_TYPE_UNIFORM:
    //         buffer_barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
    //         break;
    //     default:
    //     case VULKAN_BUFFER_TYPE_STAGING:
    //     case VULKAN_BUFFER_TYPE_READ:
    //     case VULKAN_BUFFER_TYPE_STORAGE:
    //         LOG_ERROR("No support buffer type for copy operation yet.");
    //         return;
    // }

    // vkCmdPipelineBarrier(
    //     cmdbuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &buffer_barrier, 0, nullptr
    // );
}

bool vulkan_buffer_create(buffer_t* buffer)
{
    buffer->internal_data = mallocate(sizeof(vulkan_buffer_t), MEMORY_TAG_RENDERER);
    vulkan_buffer_t* vk_buffer = buffer->internal_data;
    mzero(vk_buffer, sizeof(vulkan_buffer_t));

    switch(buffer->type)
    {
        case BUFFER_TYPE_VERTEX:
            vk_buffer->usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            vk_buffer->memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        case BUFFER_TYPE_INDEX:
            vk_buffer->usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            vk_buffer->memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        case BUFFER_TYPE_STAGING:
            vk_buffer->usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            vk_buffer->memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;
        case BUFFER_TYPE_UNIFORM:
            vk_buffer->usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            vk_buffer->memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            vk_buffer->memory_property_flags |= context->device.supports_host_local_memory ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
            break;
        case BUFFER_TYPE_READ:
        case BUFFER_TYPE_STORAGE:
        default:
            LOG_ERROR("Buffer type %u is not yet supported.", buffer->type);
            return false;
    }

    VkBufferCreateInfo buffer_info = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = buffer->size,
        .usage       = vk_buffer->usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE // TODO: Добавить поддержку совместного использования семействами очередей.
    };

    VkResult result = vkCreateBuffer(context->device.logical, &buffer_info, context->allocator, &vk_buffer->handle);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create buffer type %u: %s.", buffer->type, vulkan_result_get_string(result));
        return false;
    }

    // Получение требований памяти.
    vkGetBufferMemoryRequirements(context->device.logical, vk_buffer->handle, &vk_buffer->memory_requirements);

    // Получение индекса типа памяти.
    vk_buffer->memory_index = vulkan_util_find_memory_index(&context->device, vk_buffer->memory_requirements.memoryTypeBits, vk_buffer->memory_property_flags);
    if(vk_buffer->memory_index == INVALID_ID32)
    {
        LOG_ERROR("Failed to find memory index for buffer type %u.", buffer->type);
        return false;
    }

    // Выделение памяти.
    VkMemoryAllocateInfo memory_allocate_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = vk_buffer->memory_requirements.size,
        .memoryTypeIndex = vk_buffer->memory_index
    };

    result = vkAllocateMemory(context->device.logical, &memory_allocate_info, context->allocator, &vk_buffer->memory);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to allocate memory for buffer type %u: %s.", buffer->type, vulkan_result_get_string(result));
        return false;
    }

    // TODO: Выделять большой кусок и выполнять связывание с ним буферов.
    result = vkBindBufferMemory(context->device.logical, vk_buffer->handle, vk_buffer->memory, 0);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to bind memory buffer of type %u: %s.", buffer->type, vulkan_result_get_string(result));
        return false;
    }

    return true;
}

void vulkan_buffer_destroy(buffer_t* buffer)
{
    vulkan_buffer_t* vk_buffer = buffer->internal_data;

    if(vk_buffer->memory != nullptr)
    {
        vkFreeMemory(context->device.logical, vk_buffer->memory, context->allocator);
    }

    if(vk_buffer->handle != nullptr)
    {
        vkDestroyBuffer(context->device.logical, vk_buffer->handle, context->allocator);
    }

    mfree(vk_buffer, sizeof(vulkan_buffer_t), MEMORY_TAG_RENDERER);
}

bool vulkan_buffer_resize(buffer_t* buffer, usize new_size)
{
    vulkan_buffer_t* vk_buffer = buffer->internal_data;

    VkBufferCreateInfo buffer_info = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = new_size,
        .usage       = vk_buffer->usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE // TODO: Добавить поддержку совместного использования семействами очередей.
    };

    vulkan_buffer_t new_buffer = {0};
    VkResult result = vkCreateBuffer(context->device.logical, &buffer_info, context->allocator, &new_buffer.handle);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to resize buffer type %u: %s.", buffer->type, vulkan_result_get_string(result));
        return false;
    }

    // Получение новых требований памяти.
    vkGetBufferMemoryRequirements(context->device.logical, new_buffer.handle, &new_buffer.memory_requirements);

    // Выделение новой памяти.
    VkMemoryAllocateInfo memory_allocate_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = new_buffer.memory_requirements.size,
        .memoryTypeIndex = vk_buffer->memory_index
    };

    result = vkAllocateMemory(context->device.logical, &memory_allocate_info, context->allocator, &new_buffer.memory);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to allocate memory to resize buffer of type %u: %s.", buffer->type, vulkan_result_get_string(result));
        return false;
    }

    // Привязывание новой памяти для выполнения операций с ней.
    result = vkBindBufferMemory(context->device.logical, new_buffer.handle, new_buffer.memory, 0);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to bind memory to resize buffer of type %u: %s.", buffer->type, vulkan_result_get_string(result));
        return false;
    }

    // Копирование данных из старой памяти в новую.
    buffer_copy_range(vk_buffer, 0, &new_buffer, 0, buffer->size);

    // Освобождение памяти старого буфера.
    if(vk_buffer->memory != nullptr)
    {
        vkFreeMemory(context->device.logical, vk_buffer->memory, context->allocator);
    }

    // Уничтожение старого буфера.
    if(vk_buffer->handle != nullptr)
    {
        vkDestroyBuffer(context->device.logical, vk_buffer->handle, context->allocator);
    }

    // Обновление указателей и данных буфера.
    buffer->size = new_size;
    vk_buffer->handle = new_buffer.handle;
    vk_buffer->memory = new_buffer.memory;
    vk_buffer->memory_requirements = new_buffer.memory_requirements;

    return true;
}

bool vulkan_buffer_map_memory(buffer_t* buffer, usize offset, usize size, void** data)
{
    vulkan_buffer_t* vk_buffer = buffer->internal_data;
    VkResult result = vkMapMemory(context->device.logical, vk_buffer->memory, offset, size, 0, data);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to map memory buffer of type %u: %s.", buffer->type, vulkan_result_get_string(result));
        return false;
    }

    return true;
}

void vulkan_buffer_unmap_memory(buffer_t* buffer)
{
    vulkan_buffer_t* vk_buffer = buffer->internal_data;
    vkUnmapMemory(context->device.logical, vk_buffer->memory);
}

bool vulkan_buffer_load_range(buffer_t* buffer, usize offset, usize size, const void* data)
{
    vulkan_buffer_t* vk_buffer = buffer->internal_data;

    if(buffer_is_device_local_only(vk_buffer))
    {
        // Создание промежуточного буфера для загрузки в видеопамять.
        buffer_t staging = { .type = BUFFER_TYPE_STAGING, .size = size };
        if(!vulkan_buffer_create(&staging))
        {
            LOG_ERROR("Failed to create staging buffer.");
            return false;
        }

        vulkan_buffer_load_range(&staging, 0, size, data);
        vulkan_buffer_copy_range(&staging, 0, buffer, offset, size);

        // NOTE: Была идея в том, что бы вынести создание командного буфера наружу из всех функций, но
        //       вот фигня, при текущей логике пока запись буфера идет, мы уничтожаем временный буфер
        //       с данными и поэтому vkCmdCopyBuffer просто нет доступа к staging!!!
        // TODO: Пересмотреть логику работы функции!
        vulkan_buffer_destroy(&staging);
    }
    // Копирование без промежуточного буфера.
    else
    {
        // NOTE: Неявная синхронизация памяти при наличии флага HOST_COHERENT по каманде vkQueueSubmit().
        void* mapped_data = nullptr;
        if(!vulkan_buffer_map_memory(buffer, offset, size, &mapped_data))
        {
            LOG_ERROR("Failed to mapping memory buffer.");
            return false;
        }

        mcopy(mapped_data, data, size);
        vulkan_buffer_unmap_memory(buffer);
        // TODO: Применение flash если не флага памяти HOST_COHERENT!
    }

    return true;
}

void vulkan_buffer_copy_range(buffer_t* src, usize src_offset, buffer_t* dst, usize dst_offset, usize size)
{
    buffer_copy_range(src->internal_data, src_offset, dst->internal_data, dst_offset, size);
}

// static u32 shader_reflect_attribute_size(SpvReflectInterfaceVariable* attr)
// {
//     u32 component_count = 0;
//     u32 component_size = attr->numeric.scalar.width / 8;

//     switch(attr->format)
//     {
//         case SPV_REFLECT_FORMAT_R16_UINT:
//         case SPV_REFLECT_FORMAT_R16_SINT:
//         case SPV_REFLECT_FORMAT_R16_SFLOAT:
//         case SPV_REFLECT_FORMAT_R32_UINT:
//         case SPV_REFLECT_FORMAT_R32_SINT:
//         case SPV_REFLECT_FORMAT_R32_SFLOAT:
//         case SPV_REFLECT_FORMAT_R64_UINT:
//         case SPV_REFLECT_FORMAT_R64_SINT:
//         case SPV_REFLECT_FORMAT_R64_SFLOAT:
//             component_count = 1;
//             break;
//         case SPV_REFLECT_FORMAT_R16G16_UINT:
//         case SPV_REFLECT_FORMAT_R16G16_SINT:
//         case SPV_REFLECT_FORMAT_R16G16_SFLOAT:
//         case SPV_REFLECT_FORMAT_R32G32_UINT:
//         case SPV_REFLECT_FORMAT_R32G32_SINT:
//         case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
//         case SPV_REFLECT_FORMAT_R64G64_UINT:
//         case SPV_REFLECT_FORMAT_R64G64_SINT:
//         case SPV_REFLECT_FORMAT_R64G64_SFLOAT:
//             component_count = 2;
//             break;
//         case SPV_REFLECT_FORMAT_R16G16B16_UINT:
//         case SPV_REFLECT_FORMAT_R16G16B16_SINT:
//         case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT:
//         case SPV_REFLECT_FORMAT_R32G32B32_UINT:
//         case SPV_REFLECT_FORMAT_R32G32B32_SINT:
//         case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
//         case SPV_REFLECT_FORMAT_R64G64B64_UINT:
//         case SPV_REFLECT_FORMAT_R64G64B64_SINT:
//         case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:
//             component_count = 3;
//             break;
//         case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:
//         case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:
//         case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT:
//         case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
//         case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
//         case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
//         case SPV_REFLECT_FORMAT_R64G64B64A64_UINT:
//         case SPV_REFLECT_FORMAT_R64G64B64A64_SINT:
//         case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:
//             component_count = 4;
//             break;
//         default:
//             LOG_ERROR("Attribute format not supported: %d.", attr->format);
//             return 0;
//     }

//     u32 element_size = component_count * component_size;
//     return element_size;
// }

// static void shader_reflect(const char* shader_name, const usize data_size, const void* data)
// {
//     SpvReflectShaderModule module;
//     if(spvReflectCreateShaderModule(data_size, data, &module) != SPV_REFLECT_RESULT_SUCCESS)
//     {
//         LOG_ERROR("Failed to create spv reflector shader module for %s.", shader_name);
//         return;
//     }

//     LOG_TRACE("----------------------------------------------------------");
//     LOG_TRACE("Shader analysis: %s", shader_name);
//     LOG_TRACE("----------------------------------------------------------");

//     // Стадия шейдера.
//     switch(module.shader_stage)
//     {
//         case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
//             LOG_TRACE("Stage: VERTEX.");
//             break;
//         case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
//             LOG_TRACE("Stage: FRAGMENT.");
//             break;
//         case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
//             LOG_TRACE("Stage: COMPUTE but not supported yet.");
//             break;
//         case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
//             LOG_TRACE("Stage: GEOMETRY but not supported yet.");
//             break;
//         case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
//             LOG_TRACE("Stage: TESSELLATION CONTROL but not supported yet.");
//             break;
//         case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
//             LOG_TRACE("Stage: TESSELLATION EVALUTION but not supported yet.");
//             break;
//         default:
//             LOG_ERROR("No support shader stage: %d.", module.shader_stage);
//             return;
//     }

//     // TODO: А как формировать VkVertexInputBindingDescription??? Конфигурацией передавать в шейдер?
//     // 1. Точки входа.
//     LOG_TRACE("Entry points (count: %u):", module.entry_point_count);
//     for(u32 i = 0; i < module.entry_point_count; ++i)
//     {
//         LOG_TRACE("    Name: %s.", module.entry_points[i].name);
//     }

//     // 2. Атрибуты (входные переменные) - только для vertex стадии.
//     if(module.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
//     {
//         u32 attr_count = 0;
//         if(spvReflectEnumerateInputVariables(&module, &attr_count, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
//         {
//             LOG_ERROR("Failed to get input attribute count.");
//             spvReflectDestroyShaderModule(&module);
//             return;
//         }

//         if(attr_count > 0)
//         {
//             SpvReflectInterfaceVariable** attrs = darray_create_custom(SpvReflectInterfaceVariable*, attr_count);
//             if(spvReflectEnumerateInputVariables(&module, &attr_count, attrs) != SPV_REFLECT_RESULT_SUCCESS)
//             {
//                 LOG_ERROR("Failed to enumerate input attributes.");
//                 darray_destroy(attrs);
//                 spvReflectDestroyShaderModule(&module);
//                 return;
//             }

//             LOG_TRACE("Input attributes (count: %u):", attr_count);
//             for(u32 i = 0; i < attr_count; ++i)
//             {
//                 SpvReflectInterfaceVariable* attr = attrs[i];

//                 if(attr->location != INVALID_ID32)
//                 {
//                     u32 size = shader_reflect_attribute_size(attr);
//                     LOG_TRACE("    layout(location=%u) in size: %u, name: %s", attr->location, size, attr->name);
//                 }
//             }

//             darray_destroy(attrs);
//         }
//     }

//     // 3. Атрибуты (выходные переменные) - только для fragment стадии.
//     if(module.shader_stage == SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT)
//     {
//         // TODO: для color_attachments!
//         u32 attr_count = 0;

//         if(spvReflectEnumerateOutputVariables(&module, &attr_count, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
//         {
//             LOG_ERROR("Failed to get output attribute count.");
//             spvReflectDestroyShaderModule(&module);
//             return;
//         }

//         if(attr_count > 0)
//         {
//             SpvReflectInterfaceVariable** attrs = darray_create_custom(SpvReflectInterfaceVariable*, attr_count);
//             if(spvReflectEnumerateOutputVariables(&module, &attr_count, attrs) != SPV_REFLECT_RESULT_SUCCESS)
//             {
//                 LOG_ERROR("Failed to enumerate output attributes.");
//                 darray_destroy(attrs);
//                 spvReflectDestroyShaderModule(&module);
//                 return;
//             }

//             LOG_TRACE("Output attributes (count: %u):", attr_count);
//             for(u32 i = 0; i < attr_count; ++i)
//             {
//                 SpvReflectInterfaceVariable* attr = attrs[i];

//                 if(attr->location != INVALID_ID32)
//                 {
//                     u32 size = shader_reflect_attribute_size(attr);
//                     LOG_TRACE("    layout(location=%u) out size: %u, name: %s, array count: %u", attr->location, size, attr->name, attr->array.dims_count ? attr->array.dims[0] : 1);
//                 }
//             }

//             darray_destroy(attrs);
//         }
//     }

//     // 4. Дескрипторы (ресурсы).
//     u32 binding_count = 0;
//     if(spvReflectEnumerateDescriptorBindings(&module, &binding_count, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
//     {
//         LOG_ERROR("Failed to get binding count.");
//         spvReflectDestroyShaderModule(&module);
//         return;
//     }

//     if(binding_count > 0)
//     {
//         SpvReflectDescriptorBinding** bindings = darray_create_custom(SpvReflectDescriptorBinding*, binding_count);
//         if(spvReflectEnumerateDescriptorBindings(&module, &binding_count, bindings) != SPV_REFLECT_RESULT_SUCCESS)
//         {
//             LOG_ERROR("Failed to enumerate bindings.");
//             darray_destroy(bindings);
//             spvReflectDestroyShaderModule(&module);
//             return;
//         }

//         LOG_TRACE("Bindings (count=%u):", binding_count);
//         for(u32 i = 0; i < binding_count; ++i)
//         {
//             SpvReflectDescriptorBinding* binding = bindings[i];
//             LOG_TRACE(
//                 "    layout(set=%u, binding=%u) type=%d size=%u name=%s", binding->set, binding->binding,
//                 binding->descriptor_type, binding->block.size, binding->name
//             );
//         }

//         darray_destroy(bindings);
//     }

//     // 5. TODO: Push constants

//     spvReflectDestroyShaderModule(&module);
// }

static bool shader_create_modules(u32 stage_count, shader_stage_file_t* stage_files, VkPipelineShaderStageCreateInfo* out_stages)
{
    // Очистка структуры.
    mzero(out_stages, sizeof(VkPipelineShaderStageCreateInfo) * stage_count);

    // Создание модулей шейдеров и формирование объектов программируемых стадий конвейера.
    for(u32 i = 0; i < stage_count; ++i)
    {
        platform_file* file;

        if(platform_file_exists(stage_files[i].path) == false)
        {
            LOG_ERROR("Shader file '%s' does not exist.", stage_files[i].path);
            return false;
        }

        // TODO: Систему ресурсов.
        if(platform_file_open(stage_files[i].path, PLATFORM_FILE_MODE_READ_BINARY, &file) == false)
        {
            LOG_ERROR("Unable to read shader file '%s'.", stage_files[i].path);
            return false;
        }

        u64 file_size = 0;
        if(platform_file_size(file, &file_size) == false)
        {
            LOG_ERROR("Unable to get size of shader file '%s'.", stage_files[i].path);
            return false;
        }

        u64 check_file_size = 0;
        u32* shader_bytes = mallocate(file_size, MEMORY_TAG_UNKNOWN);
        if(platform_file_read(file, file_size, shader_bytes, &check_file_size) == false || check_file_size != file_size)
        {
            LOG_ERROR("Unable to read data from shader file '%s'.", stage_files[i].path);
            return false;
        }

        LOG_DEBUG("Shader file '%s' of size %u read successfully.", stage_files[i].path, file_size);

        // Закрытие файла.
        platform_file_close(file);
        file = nullptr;

        VkShaderModuleCreateInfo shader_module_info = {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = file_size,
            .pCode    = shader_bytes
        };

        VkResult result = vkCreateShaderModule(context->device.logical, &shader_module_info, context->allocator, &out_stages[i].module);
        if(!vulkan_result_is_success(result))
        {
            LOG_ERROR("Failed to create shader module from file '%s': %s.", stage_files[i].path, vulkan_result_get_string(result));
            return false;
        }

        // TODO: Временно.
        // shader_reflect(stage_files[i].path, file_size, shader_bytes);

        // Освобождение памяти для бинарных данных шейдера.
        mfree(shader_bytes, file_size, MEMORY_TAG_UNKNOWN);
        shader_bytes = nullptr;

        out_stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        out_stages[i].pName = "main"; // TODO: Сделать настраиваемой.

        // TODO: stage_files[i].stage & 1<<i и убрать stage_count, но ограничить SHADER_MAX_STAGES!
        switch(stage_files[i].stage)
        {
            case SHADER_STAGE_VERTEX:
                out_stages[i].stage = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case SHADER_STAGE_FRAGMENT:
                out_stages[i].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            default:
                LOG_ERROR("Unsupported shader stage: %d.", stage_files->stage);
                return false;
        }
    }

    return true;
}

static void shader_destroy_modules(u32 stage_count, VkPipelineShaderStageCreateInfo* stages)
{
    for(u32 i = 0; i < stage_count; ++i)
    {
        if(stages[i].module != nullptr)
        {
            vkDestroyShaderModule(context->device.logical, stages[i].module, context->allocator);
        }
    }

    mzero(stages, sizeof(VkPipelineShaderStageCreateInfo) * stage_count);
}

bool vulkan_shader_create(shader_t* shader, u32 stage_count, shader_stage_file_t* stage_files)
{
    shader->internal_data = mallocate(sizeof(vulkan_shader_t), MEMORY_TAG_RENDERER);
    vulkan_shader_t* vk_shader = shader->internal_data;
    mzero(vk_shader, sizeof(vulkan_shader_t));

    // Создание шейдерных модулей.
    VkPipelineShaderStageCreateInfo stages[RENDERER_MAX_SHADER_STAGES];
    if(!shader_create_modules(stage_count, stage_files, stages))
    {
        LOG_ERROR("Failed to create shader modules.");
        return false;
    }

    // Атрибуты конвейера (входящие данные вершинного шейдера).
    // TODO: Настраиваемый.
    VkVertexInputBindingDescription binding_descriptions[] = {
        // Описание данных и частоты обновления каждой вершины для vertex3d.
        {
            .binding   = 0,                           // Индекс размещения (привязки) данных.
            .stride    = sizeof(vertex3d),            // Размер выршины (шаг данных). // TODO: Выравнивание для производительности!
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX  // Частота обновления данных: для каждой вершины. // TODO: Инстансинг!
        }
    };
 
    // TODO: Настраиваемый.
    VkVertexInputAttributeDescription attribute_descriptions[] = {
        // Описание атрибутов для vertex3d.
        // Position.
        {
            .binding  = 0,                             // Индекс размещения (привязки, см. описание выше).
            .location = 0,                             // Номер смещения данных вершины для шейдера / layout(location = 0) in vec3 in_position.
            .format   = VK_FORMAT_R32G32B32_SFLOAT,    // Размер и тип передаваемых данных.
            .offset   = OFFSET_OF(vertex3d, position)  // Смещение от начала данных вершины в байтах.
        },
        // Color
        {
            .binding  = 0,                             // Индекс размещения (привязки, см. описание выше).
            .location = 1,                             // Номер смещения данных вершины для шейдера / layout(location = 1) in vec4 in_color.
            .format   = VK_FORMAT_R32G32B32A32_SFLOAT, // Размер и тип передаваемых данных.
            .offset   = OFFSET_OF(vertex3d, color)     // Смещение от начала данных вершины в байтах.
        }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = ARRAY_SIZE(binding_descriptions),   // Кол-во размещений (привязок).
        .pVertexBindingDescriptions      = binding_descriptions,
        .vertexAttributeDescriptionCount = ARRAY_SIZE(attribute_descriptions), // Кол-во атрибутов.
        .pVertexAttributeDescriptions    = attribute_descriptions,
    };

    // TODO: Временно! Организовать рефлексию биндингов из spv файлов или шейдерных модулей.
    // TODO: Получить и проверить maxBoundDescriptorSets в VkPhysicalDeviceLimits через vkGetPhysicalDeviceProperties()!
    vk_shader->set_count = 1;

    // Глобальный набор дескрипторов конвейера (камера).
    vk_shader->set_configs[0].binding_count = 1;
    vk_shader->set_configs[0].binding_sizes[0] = sizeof(renderer_camera_t);
    vk_shader->set_configs[0].bindings[0] = (VkDescriptorSetLayoutBinding){
        .binding            = 0,                                 // Индекс размещения (привязки) данных для uniform переменной.
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // Тип связываемого дескриптора - uniform переменная.
        .descriptorCount    = 1,                                 // Кол-во связываемых дескрипторов с шейдером (например, передача массива в шейдер). // TODO: Учесть при выделении памяти, получении ресурсов, освобождении, обновлении и привязке.
        .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,        // Стадия в которую передаются uniform переменные.
        .pImmutableSamplers = nullptr
    };

    // TODO: Локальный набор дескрипторов конвейера (объект).
    // vk_shader->set_configs[1].binding_count = 1;
    // vk_shader->set_configs[1].binding_sizes[0] = sizeof(renderer_object_t);
    // vk_shader->set_configs[1].bindings[0] = (VkDescriptorSetLayoutBinding){
    //     .binding            = 0,
    //     .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    //     .descriptorCount    = 1,
    //     .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
    //     .pImmutableSamplers = nullptr
    // };

    // Создание макетов дескрипторных наборов.
    for(u32 i = 0; i < vk_shader->set_count; ++i)
    {
        vulkan_shader_set_config_t* set_config = &vk_shader->set_configs[i];

        VkDescriptorSetLayoutCreateInfo set_layout_info = {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = set_config->binding_count,
            .pBindings    = set_config->bindings
        };

        VkResult result = vkCreateDescriptorSetLayout(context->device.logical, &set_layout_info, context->allocator, &vk_shader->set_layouts[i]);
        if(!vulkan_result_is_success(result))
        {
            LOG_ERROR("Failed to create descriptor set layout %u: %s.", i, vulkan_result_get_string(result));
            return false;
        }
    }

    // Push константы конвейера.
    VkPushConstantRange push_constants[] = {
        // Матрица модели.
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset     = 0,
            .size       = sizeof(renderer_model_t)
        }
    };

    // Макет конвейера, описывающий размещение дескрипторных наборов и push-констант.
    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = vk_shader->set_count,
        .pSetLayouts            = vk_shader->set_layouts,
        .pushConstantRangeCount = ARRAY_SIZE(push_constants),         // TODO: Настраиваемый.
        .pPushConstantRanges    = push_constants                      // TODO: Настраиваемый.
    };

    VkResult result = vkCreatePipelineLayout(context->device.logical, &pipeline_layout_info, context->allocator, &vk_shader->pipeline_layout);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create graphics pipeline layout: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Настройка сборочного этапа конвейера.
    // NOTE: Если в поле primitiveRestartEnable задать значение VK_TRUE, можно прервать отрезки и треугольники с
    //       топологией VK_PRIMITIVE_TOPOLOGY_LINE_STRIP и VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP и начать рисовать
    //       новые примитивы, используя специальный индекс 0xFFFF или 0xFFFFFFFF.
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // TODO: Настраиваемый (В данном момент вершины собираются в треугольники).
        .primitiveRestartEnable = VK_FALSE
    };

    // TODO: Тесселяция.

    // Область видимости и отсечение.
    // TODO: Множественные области видимости и отсечения!
    // NOTE: Т.к. используется динамическое состояние для viewport и scissor, то в указатели передается nullptr,
    //       но значения должны быть установленны обязательно больше нуля.
    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,        // TODO: Настраиваемый.
        .pViewports    = nullptr,
        .scissorCount  = 1,        // TODO: Настраиваемый.
        .pScissors     = nullptr 
    };

    // Настройка растерезаующего этапа конвейера.
    // NOTE: Координаты в 3D (и 2D) часто связаны с вращением против часовой стрелки из-за «правила правой руки» в математике
    //       и физике, где положительное вращение определяется направлением от оси X к оси Y (против часовой стрелки), что
    //       является стандартным соглашением для определения ориентации и векторов в пространстве, делая многие формулы
    //       более элегантными.
    VkPipelineRasterizationStateCreateInfo rasterization_state = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = VK_FALSE,                        // Отсекать фрагменты за пределами дальней и ближней плоскости???
        .rasterizerDiscardEnable = VK_FALSE,                        // Выполнять растерезацию и передавать во фреймбуфер.
        .polygonMode             = VK_POLYGON_MODE_FILL,            // Ребра полигона отрезки (VK_POLYGON_MODE_LINE) или полигон полностью заполняет фрагмент (VK_POLYGON_MODE_FILL).
        .cullMode                = VK_CULL_MODE_BACK_BIT,           // Тип отсечения поверхностей (VK_CULL_MODE_*: NONE, FRONT, FRONT_AND_BACK, BACK).
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE, // Порядок обхода вершин (на данный момент - против часовой стрелки).
        .depthBiasEnable         = VK_FALSE,                        // TODO: Нужно ли использовать? Для изменения значения глубины?
        .depthBiasConstantFactor = 0.0f,                            // TODO: Настраиваемый.
        .depthBiasClamp          = 0.0f,                            // TODO: Настраиваемый.
        .depthBiasSlopeFactor    = 0.0f,                            // TODO: Настраиваемый.
        .lineWidth               = 1.0f                             // TODO: Ширина линии (динамическое состояние).
    };

    // Фильтрация.
    VkPipelineMultisampleStateCreateInfo multisample_state = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT, // Без сглаживания.
        .sampleShadingEnable   = VK_FALSE,              // TODO: Нужно ли использовать???
        .minSampleShading      = 1.0f,                  // TODO: Настраиваемый.
        .pSampleMask           = nullptr,               // TODO: Настраиваемый.
        .alphaToCoverageEnable = VK_FALSE,              // TODO: Настраиваемый.
        .alphaToOneEnable      = VK_FALSE               // TODO: Настраиваемый.
    };

    // Тест глубины и трафарета.
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable       = VK_TRUE,             // TODO: Настраиваемый.
        .depthWriteEnable      = VK_TRUE,             // TODO: Настраиваемый.
        .depthCompareOp        = VK_COMPARE_OP_LESS,  // TODO: Настраиваемый.
        .depthBoundsTestEnable = VK_FALSE,            // TODO: Настраиваемый.
        .stencilTestEnable     = VK_FALSE,            // TODO: Настраиваемый.
        // .front                 = ,                    // TODO: Настраиваемый.
        // .back                  = ,                    // TODO: Настраиваемый.
        // .minDepthBounds        = 0.0f,                // TODO: Настраиваемый.
        // .maxDepthBounds        = 0.0f                 // TODO: Настраиваемый.
    };

    // Настройка смешивания цветов.
    // TODO: Множественные вложения!
    // NOTE: Источник - то что рисуется (верхний слой), назначение - то что нарисовано в изображении (нижний слой).
    // NOTE: Количество элементов должно соответствовать количеству attachment-ов передаваемых при динамическом рендере,
    //       а так же количеству выходных vec4 векторов во фрагментном шейдере!
    VkPipelineColorBlendAttachmentState color_blend_attachments[] = {
        // NOTE: Соответствует строке в фрагментном шейдере layout(location = 0) out vec4 out_color.
        {
            .blendEnable         = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp        = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .alphaBlendOp        = VK_BLEND_OP_ADD,
            .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                 | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        },
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable   = VK_FALSE,                            // TODO: Настраиваемый.
        .logicOp         = VK_LOGIC_OP_COPY,                    // TODO: Настраиваемый.
        .attachmentCount = ARRAY_SIZE(color_blend_attachments), // TODO: Настраиваемый.
        .pAttachments    = color_blend_attachments,             // TODO: Настраиваемый.
        // .blendConstants  = ,                                    // TODO: Настраиваемый.
    };

    // TODO: Настраиваемый в соответствиие с передаваемым количеством цветовых attachment-ов при начале динамического
    //       рендеринга, буфер глубины не учитывается. Не забыть про color_blend_state, смотри выше!!!
    VkPipelineRenderingCreateInfo rendering_info = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .viewMask                = 0,
        .colorAttachmentCount    = ARRAY_SIZE(color_blend_attachments),
        .pColorAttachmentFormats = &context->swapchain.image_format.format,  // TODO: Должен быть массив форматов для каждого attachment.
        .depthAttachmentFormat   = context->swapchain.depth_format,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
    };

    // Динамическое состояние: Позволяет менять состояние графического конвейера не создавая занаво.
    //                         В результате эти настройки нужно указывать прямо перед командой отрисовки.
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,    // Изменение вьюпорта.
        VK_DYNAMIC_STATE_SCISSOR,     // Изменение отсечение вьюпорта.
        VK_DYNAMIC_STATE_LINE_WIDTH   // Изменение ширины отрезков (см. растеризатор).
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = ARRAY_SIZE(dynamic_states),    // Кол-во динамических состояний.
        .pDynamicStates    = dynamic_states
    };

    // Создание графического конвейера.
    VkGraphicsPipelineCreateInfo graphics_pipeline = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &rendering_info,
        .stageCount          = stage_count,
        .pStages             = stages,
        .layout              = vk_shader->pipeline_layout,
        .pVertexInputState   = &vertex_input_state,
        .pInputAssemblyState = &input_assembly_state,
        .pTessellationState  = nullptr,
        .pViewportState      = &viewport_state,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState   = &multisample_state,
        .pDepthStencilState  = &depth_stencil_state,
        .pColorBlendState    = &color_blend_state,
        .pDynamicState       = &dynamic_state,
        .renderPass          = nullptr,                     // NOTE: Используется динамический рендеринг.
        .subpass             = 0,
        .basePipelineHandle  = nullptr,
        .basePipelineIndex   = -1
    };

    result = vkCreateGraphicsPipelines(context->device.logical, nullptr, 1, &graphics_pipeline, context->allocator, &vk_shader->pipeline);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create graphics pipeline: %s.", vulkan_result_get_string(result));
        return false;
    }

    // NOTE: Шейдерные модули представляют собой байт-код, и после создания конвейера их можно удалить,
    //       т.к. байт-код компилируется в машинный код и становится частью конвейера.
    shader_destroy_modules(stage_count, stages);

    // Подсчет дескрипторов по типам.
    u32 uniform_buffer_count  = 0;
    u32 uniform_buffer_size   = 0;

    for(u32 s = 0; s < vk_shader->set_count; ++s)
    {
        vulkan_shader_set_config_t* set_config = &vk_shader->set_configs[s];
        for(u32 b = 0; b < set_config->binding_count; ++b)
        {
            if(set_config->bindings[b].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            {                
                u32 descriptor_count = set_config->bindings[b].descriptorCount;
                u32 descriptor_size  = set_config->binding_sizes[b];

                uniform_buffer_count += descriptor_count;
                uniform_buffer_size += descriptor_size * descriptor_count;
            }
            // TODO: Добавить другие типы.
        }
    }

    // Поправка на количество кадров.
    uniform_buffer_count *= context->swapchain.max_frames_in_flight;
    uniform_buffer_size *= context->swapchain.max_frames_in_flight;

    // Создание пула дескрипторов.
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniform_buffer_count }
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, // Указывает на возможность изменения набора дескрипторов.
        .maxSets       = vk_shader->set_count * context->swapchain.max_frames_in_flight,
        .poolSizeCount = ARRAY_SIZE(pool_sizes),
        .pPoolSizes    = pool_sizes
    };

    result = vkCreateDescriptorPool(context->device.logical, &pool_info, context->allocator, &vk_shader->descriptor_pool);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create descriptor pool: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Создание uniform буфера для сетов.
    // TODO: Проверить и установить размер minUniformBufferOffsetAlignment.
    vk_shader->uniform_buffer = (buffer_t){ BUFFER_TYPE_UNIFORM, uniform_buffer_size, nullptr };
    if(!vulkan_buffer_create(&vk_shader->uniform_buffer))
    {
        LOG_ERROR("Failed to create shader: Unable to create uniform buffer.");
        return false;
    }

    if(!vulkan_buffer_map_memory(&vk_shader->uniform_buffer, 0, VK_WHOLE_SIZE, &vk_shader->uniform_buffer_mapped_data))
    {
        LOG_ERROR("Failed to create shader: Unable to map uniform buffer memory.");
        return false;
    }

    // Инвалидация ресурсов.
    for(u32 i = 0; i < RENDERER_MAX_SHADER_RESOURCES; ++i)
    {
        vk_shader->resources[i].id = INVALID_ID32;
    }

    return true;
}

void vulkan_shader_destroy(shader_t* shader)
{
    vulkan_shader_t* vk_shader = shader->internal_data;

    // TODO: отсоединить память внутри функции по уничтожению буфера. 
    vulkan_buffer_unmap_memory(&vk_shader->uniform_buffer);
    vulkan_buffer_destroy(&vk_shader->uniform_buffer);

    // NOTE: Уничтожение дескрипторных наборов происходит при уничтожении их пула автоматически.

    if(vk_shader->descriptor_pool != nullptr)
    {
        vkDestroyDescriptorPool(context->device.logical, vk_shader->descriptor_pool, context->allocator);
    }

    if(vk_shader->pipeline != nullptr)
    {
        vkDestroyPipeline(context->device.logical, vk_shader->pipeline, context->allocator);
    }

    if(vk_shader->pipeline_layout != nullptr)
    {
        vkDestroyPipelineLayout(context->device.logical, vk_shader->pipeline_layout, context->allocator);
    }

    for(u32 i = 0; i < ARRAY_SIZE(vk_shader->set_layouts); ++i)
    {
        if(vk_shader->set_layouts[i] != nullptr)
        {
            vkDestroyDescriptorSetLayout(context->device.logical, vk_shader->set_layouts[i], context->allocator);
        }
    }

    mfree(vk_shader, sizeof(vulkan_shader_t), MEMORY_TAG_RENDERER);
}

// TODO: Проверить получение, освобождени, обновление и применение для биндингов с несколькими дексрипторами.
//       в спешке не стал их временно учитывать.
bool vulkan_shader_acquire_resource(shader_t* shader, u32 set_index, u32* out_resource_id)
{
    vulkan_shader_t* vk_shader = shader->internal_data;
    *out_resource_id = INVALID_ID32;

    if(set_index >= vk_shader->set_count)
    {
        LOG_ERROR("Failed to acquire shader resource for SET=%u: Set index is out of range [0; %u).", set_index, vk_shader->set_count);
        return false;
    }

    // Поиск свободного слота.
    u32 resource_id = INVALID_ID32;
    for(u32 i = 0; i < RENDERER_MAX_SHADER_RESOURCES; ++i)
    {
        if(vk_shader->resources[i].id == INVALID_ID32)
        {
            resource_id = i;
            break;
        }
    }

    if(resource_id == INVALID_ID32)
    {
        LOG_ERROR("Failed to acquire shader resource for SET=%u: No free resource slots left.", set_index);
        return false;
    }

    // Получение указателя на ресурс.
    vulkan_shader_resource_t* resource = &vk_shader->resources[resource_id];
    resource->id = resource_id;
    resource->set_index = set_index;
    resource->binding_count = vk_shader->set_configs[set_index].binding_count;

    // Выделение памяти в uniform буфере.
    for(u32 b = 0; b < resource->binding_count; ++b)
    {
        vulkan_shader_resource_binding_t* resource_binding = &resource->bindings[b];

        // TODO: Организовать список свободных участков буфера.
        // TODO: Выравнивание размера для правильного получения следующего смещения.
        {
            // NOTE: Поправка на количество кадров.
            usize memory_requirements = vk_shader->set_configs[set_index].binding_sizes[b] * resource->binding_count * context->swapchain.max_frames_in_flight; // TODO: для текстуры достаточно одного дескриптора.

            if(vk_shader->uniform_buffer.size < (vk_shader->uniform_buffer_next_offset + memory_requirements))
            {
                LOG_ERROR("Failed to acquire shader resource for SET=%u: Out of space in uniform buffer.", set_index);
                LOG_DEBUG("SET=%u, required size %zu byte, but buffer size %zu byte.", set_index, memory_requirements, vk_shader->uniform_buffer.size);
                return false;
            }

            resource_binding->uniform_buffer_offset = vk_shader->uniform_buffer_next_offset;
            vk_shader->uniform_buffer_next_offset += memory_requirements;
        }

        mzero(resource_binding->generations, sizeof(u32) * RENDERER_MAX_FRAME_IN_FLIGHT);
        resource_binding->generation = 1;
    }

    // Создание дискрипторов ресурса.
    // NOTE: Зарезервировано 3, т.к. кол-во кадров в обработке 2-3 (см. max_frames_in_flight).
    VkDescriptorSetLayout set_layouts[3] = {
        vk_shader->set_layouts[set_index],
        vk_shader->set_layouts[set_index],
        vk_shader->set_layouts[set_index]
    };

    VkDescriptorSetAllocateInfo set_allocate_info = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = vk_shader->descriptor_pool,
        .descriptorSetCount = context->swapchain.max_frames_in_flight, // TODO: для текстуры достаточно одного дескриптора.
        .pSetLayouts        = set_layouts
    };

    VkResult result = vkAllocateDescriptorSets(context->device.logical, &set_allocate_info, resource->descriptor_sets);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to acquire shader resource for SET=%u: Descriptor sets could not be allocated: %s.", set_index, vulkan_result_get_string(result));
        return false;
    }

    *out_resource_id = resource_id;
    vk_shader->resource_count++;
    return true;
}

void vulkan_shader_release_resource(shader_t* shader, u32 resource_id)
{
    vulkan_shader_t* vk_shader = shader->internal_data;
    vulkan_shader_resource_t* resource = &vk_shader->resources[resource_id];

    // Ожидание завершения операций использующих этот ресурс.
    vkDeviceWaitIdle(context->device.logical);

    if(resource->id == INVALID_ID32)
    {
        LOG_ERROR("Failed to release shader resource for ID=%u: Slot is free.", resource_id);
        return;
    }

    VkResult result = vkFreeDescriptorSets(
        context->device.logical, vk_shader->descriptor_pool, context->swapchain.max_frames_in_flight, resource->descriptor_sets
    );

    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to release shader resource for ID=%u: Descriptor sets could not be freed: %s.", resource_id, vulkan_result_get_string(result));
        return;
    }

    // TODO: Организовать список свободных участков буфера + освободить память буфера.

    // Освобождение памяти ресурса и инвалидация его.
    mzero(resource, sizeof(vulkan_shader_resource_t));
    resource->id = INVALID_ID32;
    vk_shader->resource_count--;
}

void vulkan_shader_update_resource_binding(shader_t* shader, u32 resource_id, u32 binding_index, const void* data)
{
    vulkan_shader_t* vk_shader = shader->internal_data;
    vulkan_shader_resource_t* resource = &vk_shader->resources[resource_id];
    vulkan_shader_set_config_t* resource_config = &vk_shader->set_configs[resource->set_index];

    if(resource->id == INVALID_ID32)
    {
        LOG_ERROR("Failed to update resource for BINDING=%u: Invalid resource ID=%u.", binding_index, resource_id);
        return;
    }

    // TODO: Пока что обновляет массив ресурсов целиком. Сделать отдельное обновление по индексу: u32 array_first, u32 array_count.
    usize resource_size = resource_config->binding_sizes[binding_index] * resource_config->bindings[binding_index].descriptorCount;
    usize resource_offset = resource->bindings[binding_index].uniform_buffer_offset + resource_size * context->swapchain.current_frame;

    // TODO: В зависимости от типа дискриптора, обновлять нужные данные.
    void* addr = POINTER_ADD_OFFSET(vk_shader->uniform_buffer_mapped_data, resource_offset);
    mcopy(addr, data, resource_size);

    // TODO: Необходимо только для текстур.
    // resource->bindings[binding_index].generation++;
}

void vulkan_shader_apply_resource(shader_t* shader, u32 resource_id)
{
    vulkan_shader_t* vk_shader = shader->internal_data;
    vulkan_shader_resource_t* resource = &vk_shader->resources[resource_id];
    vulkan_shader_set_config_t* resource_config = &vk_shader->set_configs[resource->set_index];

    if(resource->id == INVALID_ID32)
    {
        LOG_ERROR("Failed to apply resource: Invalid resource ID=%u.", resource_id);
        return;
    }

    u32 current_frame = context->swapchain.current_frame;
    VkCommandBuffer cmdbuf = context->graphics_command_buffers[current_frame];
    VkDescriptorSet descriptor_set = resource->descriptor_sets[current_frame];

    // Обновление дескриптора.
    u32 write_set_count = 0;
    VkWriteDescriptorSet write_sets[RENDERER_MAX_SHADER_SETS];
    // mzero(write_sets, sizeof(VkWriteDescriptorSet) * RENDERER_MAX_SHADER_SETS);

    for(u32 binding_index = 0; binding_index < resource->binding_count; ++binding_index)
    {
        vulkan_shader_resource_binding_t* binding = &resource->bindings[binding_index];
        if(binding->generation == binding->generations[current_frame]) continue;

        usize resource_size = resource_config->binding_sizes[binding_index] * resource_config->bindings[binding_index].descriptorCount;
        usize resource_offset = binding->uniform_buffer_offset + resource_size * current_frame;

        VkDescriptorBufferInfo buffer_info = {
            .buffer = ((vulkan_buffer_t*)vk_shader->uniform_buffer.internal_data)->handle,
            .offset = resource_offset,
            .range  = resource_size
        };

        write_sets[write_set_count] = (VkWriteDescriptorSet){
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = descriptor_set,
            .dstBinding       = binding_index,
            .dstArrayElement  = 0,
            .descriptorCount  = resource_config->bindings[binding_index].descriptorCount,
            .descriptorType   = resource_config->bindings[binding_index].descriptorType,
            .pBufferInfo      = &buffer_info,
            .pImageInfo       = nullptr,
            .pTexelBufferView = nullptr
        };

        write_set_count++;
        binding->generations[current_frame] = binding->generation;
    }

    if(write_set_count > 0)
    {
        vkUpdateDescriptorSets(context->device.logical, write_set_count, write_sets, 0, nullptr);
        LOG_DEBUG("Resource descriptor with ID=%u has been updated. Current frame %u, set %u.", resource_id, current_frame, resource->set_index);
    }

    // Привязка текущего ресурса к соответствующему размещению пайплайна.
    vkCmdBindDescriptorSets(
        cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline_layout, resource->set_index, 1, &descriptor_set, 0, nullptr
    );
}

void vulkan_shader_update_model(shader_t* shader, renderer_model_t* model)
{
    u32 current_frame = context->swapchain.current_frame;
    VkCommandBuffer cmdbuf = context->graphics_command_buffers[current_frame];
    vulkan_shader_t* vk_shader = shader->internal_data;

    vkCmdPushConstants(cmdbuf, vk_shader->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(renderer_model_t), model);
}

void vulkan_texture_create(texture_t* t, const void* data)
{
    t->internal_data = mallocate(sizeof(vulkan_texture_map_t), MEMORY_TAG_TEXTURE);
    vulkan_texture_map_t* map = t->internal_data;
    mzero(map, sizeof(vulkan_texture_map_t));

    VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;
    u32 image_size = t->width * t->height * t->channels;

    // 1. Создание изображения и промежуточного буфера для загрузки в него данных.
    if(!vulkan_image_create(
        context, t->width, t->height, image_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &map->image
    ))
    {
        LOG_ERROR("Failed to create texture image.");
        return;
    }

    if(!vulkan_image_create_view(context, image_format, VK_IMAGE_ASPECT_COLOR_BIT, &map->image))
    {
        LOG_ERROR("Failed to create texture image views.");
        return;
    }

    buffer_t staging = { .type = BUFFER_TYPE_STAGING, .size = image_size };
    if(!vulkan_buffer_create(&staging))
    {
        LOG_ERROR("Failed to create staging buffer.");
        return;
    }

    VkCommandBuffer cmdbuf;
    vulkan_command_buffer_begin_single_use(&context->graphics_command_manager, &cmdbuf);

    // 2. Загузка данных в промежуточный буфер.
    vulkan_buffer_load_range(&staging, 0, image_size, data);

    // 3. Копирование данных из промежуточного буфера.
    vulkan_image_transition_layout(cmdbuf, VULKAN_IMAGE_TRANSITION_UNDEFINED_TO_TRANSFER_DST, &map->image.handle);
    vulkan_image_copy_from_buffer(cmdbuf, &map->image, ((vulkan_buffer_t*)staging.internal_data)->handle);
    vulkan_image_transition_layout(cmdbuf, VULKAN_IMAGE_TRANSITION_TRANSFER_DST_TO_SHADER_READ, &map->image.handle);

    vulkan_command_buffer_end_single_use(&context->graphics_command_manager, cmdbuf);
    vulkan_buffer_destroy(&staging);

    // TODO: Сделать полностью настраиваемой.
    VkSamplerCreateInfo sampler_info = {
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter               = VK_FILTER_LINEAR,                      // TODO: Заменить
        .minFilter               = VK_FILTER_LINEAR,                      // TODO: Заменить
        .addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // TODO: Заменить
        .addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // TODO: Заменить
        .addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // TODO: Заменить
        .anisotropyEnable        = VK_TRUE,                               // TODO: Должно быть в соответсвии с настроками устройства.
        .maxAnisotropy           = 16,
        .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable           = VK_FALSE,
        .compareOp               = VK_COMPARE_OP_ALWAYS,
        .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias              = 0.0f,
        .minLod                  = 0.0f,
        .maxLod                  = 0.0f,
    };

    VkResult result = vkCreateSampler(context->device.logical, &sampler_info, context->allocator, &map->sampler);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create sampler: %s.", vulkan_result_get_string(result));
        return;
    }

    t->generation++;
}

void vulkan_texture_destroy(texture_t* t)
{
    vulkan_texture_map_t* map = t->internal_data;
    vkDestroySampler(context->device.logical, map->sampler, context->allocator);
    vulkan_image_destroy(context, &map->image);
    t->internal_data = nullptr;
}
