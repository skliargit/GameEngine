#include "renderer/vulkan/vulkan.h"
#include "renderer/vulkan/vulkan_types.h"
#include "renderer/vulkan/vulkan_result.h"
#include "renderer/vulkan/vulkan_window.h"

#include "debug/assert.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/string.h"
#include "core/containers/darray.h"

static vulkan_context* context = nullptr;

bool vulkan_backend_initialize(const char* title, u32 width, u32 height)
{
    UNUSED(width);
    UNUSED(height);

    ASSERT(context == nullptr, "Vulkan backend is already initialized.");

    context = mallocate(sizeof(vulkan_context), MEMORY_TAG_RENDERER);
    if(!context)
    {
        LOG_ERROR("Failed to allocate memory for vulkan context.");
        return false;
    }
    mzero(context, sizeof(vulkan_context));

    // Последняя доступная версия Vulkan API.
    u32 api_ver;
    vkEnumerateInstanceVersion(&api_ver);
    LOG_TRACE("Available Vulkan API %u.%u.%u", VK_VERSION_MAJOR(api_ver), VK_VERSION_MINOR(api_ver), VK_VERSION_PATCH(api_ver));

    // Выбранная версия Vulkan API.
    api_ver = VK_MAKE_API_VERSION(0, 1, 4, 321);
    LOG_TRACE("Chosen Vulkan API %u.%u.%u", VK_VERSION_MAJOR(api_ver), VK_VERSION_MINOR(api_ver), VK_VERSION_PATCH(api_ver));

    // Получение списка слоев для экземпляра Vulkan.
    u32 required_layer_count = 0;
    const char** required_layers = nullptr;

#ifdef DEBUG_FLAG
    // Запрос доступных слоев.
    u32 available_layer_count;
    vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);
    VkLayerProperties* available_layers = darray_create_custom(VkLayerProperties, available_layer_count);
    vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers);

    // Требуемые слои.
    required_layers = darray_create(const char*);
    darray_push(required_layers, &"VK_LAYER_KHRONOS_validation"); // Проверяет правильность использования Vulkan API.
    // darray_push(required_layers, &"VK_LAYER_LUNARG_api_dump");    // Выводит на консоль вызовы и передаваемые параметры.
    // darray_push(required_layers, &"VK_LAYER_RENDERDOC_Capture");  // Захват кадров для отладки в RenderDoc.

    LOG_DEBUG("Vulkan instance used layers:");
    required_layer_count = darray_length(required_layers);
    for(u32 i = 0; i < required_layer_count; ++i)
    {
        bool found = false;
        for(u32 j = 0; j < available_layer_count; ++j)
        {
            if(string_equal(required_layers[i], available_layers[j].layerName))
            {
                found = true;
                break;
            }
        }

        LOG_DEBUG("[%s] %s", found ? "+" : "-", required_layers[i]);
        if(!found)
        {
            darray_remove(required_layers, i, nullptr);
        }
    }

    // Удаление временного массива доступных слоев.
    darray_destroy(available_layers);

    // Обновление количества требуемых слоев.
    required_layer_count = darray_length(required_layers);
#endif

    // Получение списка расширений для экземпляра Vulkan.
    u32 required_extention_count = 0;
    const char** required_extentions = nullptr;

    // Запрос доступных расширений.
    u32 available_extention_count;
    vkEnumerateInstanceExtensionProperties(nullptr, &available_extention_count, nullptr);
    VkExtensionProperties* available_extentions = darray_create_custom(VkExtensionProperties, available_extention_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &available_extention_count, available_extentions);

    // Платформо-зависимые и другие требуемые расширения.
    platform_window_enumerate_vulkan_extentions(&required_extention_count, nullptr);
    required_extentions = darray_create_custom(const char*, required_extention_count);
    platform_window_enumerate_vulkan_extentions(&required_extention_count, required_extentions);
    darray_set_length(required_extentions, required_extention_count);
    darray_push(required_extentions, &VK_KHR_SURFACE_EXTENSION_NAME);     // Расширение для использования поверхности windowы.

#ifdef DEBUG_FLAG
    darray_push(required_extentions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Расширение для отладки.
#endif

    LOG_DEBUG("Vulkan instance used extentions:");
    required_extention_count = darray_length(required_extentions);
    for(u32 i = 0; i < required_extention_count; ++i)
    {
        bool found = false;
        for(u32 j = 0; j < available_extention_count; ++j)
        {
            if(string_equal(required_extentions[i], available_extentions[j].extensionName))
            {
                found = true;
                break;
            }
        }

        LOG_DEBUG("[%s] %s", found ? "+" : "-", required_extentions[i]);
        if(!found)
        {
            darray_remove(required_extentions, i, nullptr);
        }
    }

    // Удаление временного массива доступных расширений.
    darray_destroy(available_extentions);

    // Обновление количества требуемых расширений.
    required_extention_count = darray_length(required_extentions);

    // Создание экземпляра Vulkan.
    VkApplicationInfo application_info = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = title,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "GameEngine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = api_ver
    };

    VkInstanceCreateInfo instance_info = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &application_info,
        .enabledExtensionCount   = required_extention_count,
        .ppEnabledExtensionNames = required_extentions,
        .enabledLayerCount       = required_layer_count,
        .ppEnabledLayerNames     = required_layers
    };

    VkResult result = vkCreateInstance(&instance_info, context->allocator, &context->instance);
    if(result != VK_SUCCESS)
    {
        LOG_ERROR("Vulkan instance creation failed.");
        return false;
    }
    LOG_TRACE("Vulkan instance created successfully.");

    // Освобождение памяти.
    if(required_layers)
    {
        darray_destroy(required_layers);
    }

    if(required_extentions)
    {
        darray_destroy(required_extentions);
    }

    // TODO: Перехват консоли vulkan.
    // TODO: Создание поверхности window.

    return true;
}

void vulkan_backend_shutdown()
{
    ASSERT(context != nullptr, "Vulkan backend should be initialized.");

    vkDestroyInstance(context->instance, context->allocator);
    LOG_TRACE("Vulkan instance destroy complete.");

    mfree(context, sizeof(vulkan_context), MEMORY_TAG_RENDERER);
    context = nullptr;
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
        LOG_TRACE("Failed to enumerate physical devices. Result: %s.", vulkan_result_is_success(result));
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
