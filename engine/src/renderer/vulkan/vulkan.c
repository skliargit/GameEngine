#include "renderer/vulkan/vulkan.h"
#include "renderer/vulkan/vulkan_types.h"
#include "renderer/vulkan/vulkan_result.h"
#include "renderer/vulkan/vulkan_window.h"
#include "renderer/vulkan/vulkan_device.h"

#include "debug/assert.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/string.h"
#include "core/containers/darray.h"

static vulkan_context* context = nullptr;

static VkResult vulkan_instance_create()
{
    // Проверка требований к версии Vulkan API.
    u32 instance_version;
    vkEnumerateInstanceVersion(&instance_version);

    u32 min_required_version = VK_API_VERSION_1_4;
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
    // darray_push(layers, &"VK_LAYER_RENDERDOC_Capture");  // Захват кадров для отладки в RenderDoc (NOTE: в Wayland не работает).
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

static void vulkan_instance_destroy()
{
    vkDestroyInstance(context->instance, context->allocator);
    context->instance = nullptr;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_messenger_handler(
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

            // DEBUG_BREAK();
            return VK_TRUE; // Указывает прервать работу.
    }

    return VK_FALSE; // Указывает продолжать работу.
}

static VkResult vulkan_debug_messenger_create()
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
        .pfnUserCallback = vulkan_debug_messenger_handler,
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

static void vulkan_debug_messenger_destroy()
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

    VkResult vk_result = vulkan_instance_create();
    if(!vulkan_result_is_success(vk_result))
    {
        LOG_ERROR("Failed to create vulkan instance. Result: %s.", vulkan_result_get_string(vk_result));
        return false;
    }
    LOG_TRACE("Vulkan instance created successfully.");

#ifdef DEBUG_FLAG
    vk_result = vulkan_debug_messenger_create();
    if(!vulkan_result_is_success(vk_result))
    {
        LOG_ERROR("Failed to create vulkan debug messenger. Result: %s.", vulkan_result_get_string(vk_result));
        return false;
    }
    LOG_TRACE("Vulkan debug messenger created successfully.");
#endif

    vk_result = platform_window_create_vulkan_surface(context->window, context->instance, context->allocator, (void**)&context->surface);
    if(!vulkan_result_is_success(vk_result))
    {
        LOG_ERROR("Failed to create vulkan surface. Result: %s.", vulkan_result_get_string(vk_result));
        return false;
    }
    LOG_TRACE("Vulkan surfcae created successfully.");

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

    // Конфигурация устройства.
    // TODO: Настраиваемое.
    vulkan_device_config device_cfg = {
        .device_type = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
        .extension_count = darray_length(device_extensions),
        .extensions = device_extensions,
        .use_sampler_anisotropy = true
    };

    bool b_result = vulkan_device_create(context, physical_device_selected, &device_cfg, &context->device);

    // Оcвобождение временного списка устройств и расширений устройства.
    darray_destroy(physical_devices);
    darray_destroy(device_extensions);

    if(!b_result)
    {
        LOG_ERROR("Failed to create vulkan device.");
        return false;
    }
    LOG_TRACE("Vulkan device created successfully.");

    // TODO: Цепочку обмена.
    // TODO: Командные буферы.
    // TODO: Синхронизацию.
    // TODO: Буферы рендеринга.

    LOG_TRACE("Vulkan backend initialized successfully.");
    return true;
}

void vulkan_backend_shutdown()
{
    ASSERT(context != nullptr, "Vulkan backend should be initialized.");

    if(context->device.logical)
    {
        vulkan_device_destroy(context, &context->device);
        LOG_TRACE("Vulkan device destroy complete.");
    }

    if(context->surface)
    {
        platform_window_destroy_vulkan_surface(context->window, context->instance, context->allocator, context->surface);
        context->surface = nullptr;
        LOG_TRACE("Vulkan surface destroy complete.");
    }

    if(context->debug_messenger)
    {
        vulkan_debug_messenger_destroy();
        LOG_TRACE("Vulkan debug messenger destroy complete.");
    }

    if(context->instance)
    {
        vulkan_instance_destroy();
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
        LOG_TRACE("Vulkan is not supported by the system. Result: %s.", vulkan_result_get_string(result));
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
        LOG_TRACE("Failed to create Vulkan instance. Result: %s.", vulkan_result_get_string(result));
        return false;
    }

    u32 device_count = 0;
    result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if(!vulkan_result_is_success(result))
    {
        LOG_TRACE("Failed to enumerate physical devices. Result: %s.", vulkan_result_get_string(result));
        vkDestroyInstance(instance, nullptr);
        return false;
    }

    if(device_count == 0)
    {
        LOG_TRACE("No Vulkan compatible graphics cards found.");
        vkDestroyInstance(instance, nullptr);
        return false;
    }

    vkDestroyInstance(instance, nullptr);
    return true;
}

void vulkan_backend_on_resize(const u32 width, const u32 height)
{
    LOG_TRACE("Vulkan resize event to %ux%u.", width, height);
}
