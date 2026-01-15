#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_window.h"
#include "renderer/vulkan/vulkan_result.h"

#include "debug/assert.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/string.h"
#include "core/containers/darray.h"

// TODO: Пересмотреть функцию, что имеют ли значения индексы очередей внутри семейства или только семейство очередей?
//       Поиск семейст, для каждого типа очереди организивать стек выбора.
bool vulkan_device_create(vulkan_context* context, vulkan_physical_device* physical, vulkan_device_config* config, vulkan_device* out_device)
{
    out_device->physical = physical->handle;
    out_device->memory_properties = physical->memory_properties;

    // Проверка требований к типу устройства.
    // TODO: Проверка в соответствии с конфигурацией!
    if(physical->properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
    && physical->properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        LOG_ERROR("Vulkan physical device does not match the required device type: integrated or discrete.");
        return false;
    }

    // Проверка поддержки необходимых очередей.
    if(physical->queue_graphics_count == 0 || physical->queue_compute_count == 0
    || physical->queue_transfer_count == 0 || physical->queue_present_count == 0)
    {
        LOG_ERROR("Vulkan physical device does not support the required queues.");
        return false;
    }

    // Проверка запрашиваемых расширений.
    if(config->extension_count > 0)
    {
        u32 available_extension_count = 0;
        VkResult result = vkEnumerateDeviceExtensionProperties(physical->handle, nullptr, &available_extension_count, nullptr);
        if(!vulkan_result_is_success(result) || available_extension_count == 0)
        {
            LOG_ERROR("Vulkan physical device: available extensions are not provided.");
            return false;
        }

        VkExtensionProperties* available_extensions = darray_create_custom(VkExtensionProperties, available_extension_count);
        vkEnumerateDeviceExtensionProperties(physical->handle, nullptr, &available_extension_count, available_extensions);

        LOG_TRACE("Vulkan device required extensions:");
        bool extentsion_not_found;
        for(u32 i = 0; i < config->extension_count; ++i)
        {
            extentsion_not_found = true;
            for(u32 j = 0; j < available_extension_count; ++j)
            {
                if(string_equal(config->extensions[i], available_extensions[j].extensionName))
                {
                    extentsion_not_found = false;
                    break;
                }
            }

            LOG_TRACE(" %s %s", extentsion_not_found ? "-" : "+", config->extensions[i]);

            if(extentsion_not_found)
            {
                LOG_ERROR("Vulkan physical device does not support '%s' extension.", config->extensions[i]);
                break;
            }
        }

        // Очистка используемой памяти.
        darray_destroy(available_extensions);

        if(extentsion_not_found)
        {
            return false;
        }
    }

    // Проверка поддержки анизотропной фильтрации.
    if(config->use_sampler_anisotropy && !physical->features.samplerAnisotropy)
    {
        LOG_TRACE("Vulkan physical device does not support sampler anisotropy.");
        return false;
    }

    // Выбрать семейств и индексов очередей.
    u32 family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical->handle, &family_count, nullptr);
    VkQueueFamilyProperties* families = darray_create_custom(VkQueueFamilyProperties, family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical->handle, &family_count, families);

    // Создание списка с информацией по семействам.
    VkDeviceQueueCreateInfo* family_info = darray_create_custom(VkDeviceQueueCreateInfo, family_count);
    darray_set_length(family_info, family_count);

    // Приоритеты по умолчанию.
    // TODO: Конфигурируемые!
    f32 queue_priorities[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    // Вспомогательне переменные для получения указателей на очереди.
    u32 graphics_family_index = INVALID_ID32;
    u32 compute_family_index  = INVALID_ID32;
    u32 transfer_family_index = INVALID_ID32;
    u32 present_family_index  = INVALID_ID32;
    u32 graphics_queue_index  = 0;
    u32 compute_queue_index   = 0;
    u32 transfer_queue_index  = 0;
    u32 present_queue_index   = 0;
    u32 transfer_score_min    = INVALID_ID32;

    // Процесс сбора информации и формирования очередей.
    for(u32 i = 0; i < family_count; ++i)
    {
        u32 queue_score_current = 0;
        u32 queue_next_index = 0;
        u32 queue_last_index = families[i].queueCount - 1;
        VkQueueFlags queue_flags = families[i].queueFlags;
        bool has_graphics = (queue_flags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT;
        bool has_compute  = (queue_flags & VK_QUEUE_COMPUTE_BIT ) == VK_QUEUE_COMPUTE_BIT;
        bool has_transfer = (queue_flags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT;
        bool has_present  = platform_window_supports_vulkan_presentation(context->window, physical->handle, i);

        // Приоритеты использования по умолчанию.
        VkDeviceQueueCreateInfo* current_family = &family_info[i];
        current_family->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        current_family->queueFamilyIndex = i;
        current_family->pQueuePriorities = queue_priorities;

        // NOTE: Согласно спецификации Vulkan, раздел 5.1 (физические устройства):
        //       Очереди которые поддерживают графические или вычислительные операции по умолчанию поддерживают
        //       и операции передачи данных. А потому специализированных графических и вычислительных очередей
        //       быть не может. Только очередь передачи данных может быть специализированной.

        // TODO: Проблема этого подхода в том, если вдруг present объедениться с graphics и в этом же семействе
        //       окажется compute и transfer, то нулевая очередь не будет занята. Получение индексов сделать поздним!
        if(has_graphics && (has_present || graphics_family_index == INVALID_ID32))
        {
            // Освобождение семейства.
            if(graphics_family_index != INVALID_ID32)
            {
                family_info[graphics_family_index].queueCount--;
            }

            // Прекрепление к новому семейству.
            graphics_family_index = i;
            graphics_queue_index  = queue_next_index;
            current_family->queueCount++;
            queue_score_current++;
        }

        if(has_compute && (!has_graphics || compute_family_index == INVALID_ID32))
        {
            // Освобождение семейства.
            if(compute_family_index != INVALID_ID32)
            {
                family_info[compute_family_index].queueCount--;
            }

            // Прекрепление к новому семейству.
            compute_family_index = i;
            compute_queue_index  = (queue_next_index < queue_last_index ? queue_next_index++ : queue_next_index);
            current_family->queueCount++;
            queue_score_current++;
        }

        if(has_transfer && (!has_graphics || transfer_family_index == INVALID_ID32)
        && (!has_compute || transfer_score_min > queue_score_current))
        {
            // Освобождение семейства.
            if(transfer_family_index != INVALID_ID32)
            {
                family_info[transfer_family_index].queueCount--;
            }

            // Прекрепление к новому семейству.
            transfer_family_index = i;
            transfer_queue_index  = (queue_next_index < queue_last_index ? queue_next_index++ : queue_next_index);
            current_family->queueCount++;
            transfer_score_min = has_compute ? queue_score_current : 0;
        }

        if(has_present && (graphics_family_index == i || present_family_index == INVALID_ID32))
        {
            // Освобождение семейства.
            if(present_family_index != INVALID_ID32)
            {
                family_info[present_family_index].queueCount--;
            }

            // Прекрепление к новому семейству.
            present_family_index = i;
            present_queue_index  = (queue_next_index < queue_last_index ? queue_next_index++ : queue_next_index);
            current_family->queueCount++;
        }
    }

    // Проверка что все необходимые семейства найдены.
    if(graphics_family_index == INVALID_ID32 && present_family_index == INVALID_ID32
    && compute_family_index == INVALID_ID32 && transfer_family_index == INVALID_ID32)
    {
        LOG_ERROR("Failed to find required queue families.");
        // TODO: Уничтожение ресурсов.
        return false;
    }

    // Объединение очередей graphics + present, если возможно.
    if(graphics_family_index == present_family_index)
    {
        family_info[present_family_index].queueCount--;
        present_queue_index = graphics_queue_index;
    }

    // Удаление незадействованых очередей.
    for(u32 i = 0; i < family_count; ++i)
    {
        if(family_info[i].queueCount == 0)
        {
            LOG_TRACE("Remove family index %u!", family_info[i].queueFamilyIndex);
            darray_remove(family_info, i, nullptr);
            family_count--;
            i--;
        }
        else
        {
            u32 family_index = family_info[i].queueFamilyIndex;
            u32 queue_count  = family_info[i].queueCount;
            u32 queue_max    = families[family_index].queueCount;
            LOG_DEBUG("In use family index %u, queues %u (max count %u)!", family_index, queue_count, queue_max);
        }
    }

    // Освобождение памяти.
    darray_destroy(families);

    // Указывают на необходимость синхронизации между очередями: графики, вычислений и передачи данных.
    bool compute_dedicated = (graphics_family_index != compute_family_index || graphics_queue_index != compute_queue_index)
                          && (compute_family_index != transfer_family_index || compute_queue_index != transfer_queue_index);

    bool transfer_dedicated = (graphics_family_index != transfer_family_index || graphics_queue_index != transfer_queue_index)
                           && compute_dedicated;

    // Указывает не необходимость синхронизации между очередеми: графики и презентации.
    bool present_dedicated  = (graphics_family_index != present_family_index) || (graphics_queue_index != present_queue_index);
    bool graphics_dedicated = present_dedicated && compute_dedicated && transfer_dedicated;

    // Вывод отладочной информации по получившимся очередям.
    LOG_DEBUG("Graphics : family index %u, queue index %u%s", graphics_family_index, graphics_queue_index, graphics_dedicated ? " (DEDICATED)":"");
    LOG_DEBUG("Present  : family index %u, queue index %u%s", present_family_index,  present_queue_index,  present_dedicated  ? " (DEDICATED)":"");
    LOG_DEBUG("Compute  : family index %u, queue index %u%s", compute_family_index,  compute_queue_index,  compute_dedicated  ? " (DEDICATED)":"");
    LOG_DEBUG("Transfer : family index %u, queue index %u%s", transfer_family_index, transfer_queue_index, transfer_dedicated ? " (DEDICATED)":"");

    // Сохранение значений.
    out_device->graphics_queue.family_index = graphics_family_index;
    out_device->graphics_queue.dedicated    = graphics_dedicated;
    out_device->compute_queue.family_index  = compute_family_index;
    out_device->compute_queue.dedicated     = compute_dedicated;
    out_device->transfer_queue.family_index = transfer_family_index;
    out_device->transfer_queue.dedicated    = transfer_dedicated;
    out_device->present_queue.family_index  = present_family_index;
    out_device->present_queue.dedicated     = present_dedicated;

    // TODO: Определить когда включать!
    // Включение дополнительной возможности: изменение динамических состояний конвейера на лету.
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamic_state_feature = {
        .sType                      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
        .extendedDynamicState       = VK_TRUE
    };

    // Включение дополнительной возможности: динамический рендеринг.
    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature = {
        .sType                      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .pNext                      = &dynamic_state_feature,
        .dynamicRendering           = VK_TRUE
    };

    // Включение базовых возможностей.
    VkPhysicalDeviceFeatures2 features = {
        .sType                      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext                      = &dynamic_rendering_feature,
        .features.samplerAnisotropy = config->use_sampler_anisotropy ? VK_TRUE : VK_FALSE
    };

    VkDeviceCreateInfo device_info = {
        .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                      = &features,
        .pEnabledFeatures           = nullptr,                // NOTE: Смотри pNext.
        .queueCreateInfoCount       = family_count,
        .pQueueCreateInfos          = family_info,
        .enabledExtensionCount      = config->extension_count,
        .ppEnabledExtensionNames    = config->extensions,
        // NOTE: Устарело и больше не используется!
        .enabledLayerCount          = 0,
        .ppEnabledLayerNames        = nullptr,
    };

    VkResult result = vkCreateDevice(physical->handle, &device_info, context->allocator, &out_device->logical);

    // Освобождение памяти.
    darray_destroy(family_info);

    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create device: %s.", vulkan_result_get_string(result));
        return false;
    }
    LOG_TRACE("Logical device created successfully.");

    // Получение очередей.
    vkGetDeviceQueue(out_device->logical, graphics_family_index, graphics_queue_index, &out_device->graphics_queue.handle);
    vkGetDeviceQueue(out_device->logical, present_family_index,  present_queue_index,  &out_device->present_queue.handle );
    vkGetDeviceQueue(out_device->logical, compute_family_index,  compute_queue_index,  &out_device->compute_queue.handle );
    vkGetDeviceQueue(out_device->logical, transfer_family_index, transfer_queue_index, &out_device->transfer_queue.handle);

    // Вывод указателей очередей.
    LOG_TRACE("Graphics queue handle : %p", out_device->graphics_queue.handle);
    LOG_TRACE("Present queue handle  : %p", out_device->present_queue.handle );
    LOG_TRACE("Compute queue handle  : %p", out_device->compute_queue.handle );
    LOG_TRACE("Transfer queue handle : %p", out_device->transfer_queue.handle);

    // Доп. проверка.
    ASSERT(
        graphics_family_index == present_family_index || out_device->graphics_queue.handle == out_device->present_queue.handle,
        "Graphics queue handle and present queue handle must be equal!"
    );

    // TODO: Изменить алгоритм создания пулов и стратегии их использования!

    // Получение пула команд очереди графики.
    VkCommandPoolCreateInfo graphics_command_pool_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // т.к. используется для кадров, а они не являются временным явлением.
        .queueFamilyIndex = graphics_family_index,
    };

    result = vkCreateCommandPool(out_device->logical, &graphics_command_pool_info, context->allocator, &out_device->graphics_queue.command_pool);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create graphics command pool: %s.", vulkan_result_get_string(result));
        return false;
    }
    LOG_TRACE("Graphics command pool created successfully.");

    // Получение пула команд очереди показа.
    if(graphics_family_index != present_family_index)
    {
        VkCommandPoolCreateInfo present_command_pool_info = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = present_family_index,
        };

        result = vkCreateCommandPool(out_device->logical, &present_command_pool_info, context->allocator, &out_device->present_queue.command_pool);
        if(!vulkan_result_get_string(result))
        {
            LOG_ERROR("Failed to create present command pool: %s.", vulkan_result_get_string(result));
            return false;
        }
        LOG_TRACE("Present command pool created successfully.");
    }
    else
    {
        out_device->present_queue.command_pool = out_device->graphics_queue.command_pool;
        LOG_TRACE("Present command pool shared with graphics.");
    }

    // Получение вычислительного пула команд.
    VkCommandPoolCreateInfo compute_command_pool_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = compute_family_index,
    };

    result = vkCreateCommandPool(out_device->logical, &compute_command_pool_info, context->allocator, &out_device->compute_queue.command_pool);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create compute command pool: %s.", vulkan_result_get_string(result));
        return false;
    }
    LOG_TRACE("Compute command pool created successfully.");

    // Получение пула команд для передачи данных.
    VkCommandPoolCreateInfo transfer_command_pool_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = transfer_family_index,
    };

    result = vkCreateCommandPool(out_device->logical, &transfer_command_pool_info, context->allocator, &out_device->transfer_queue.command_pool);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create transfer command pool: %s.", vulkan_result_get_string(result));
        return false;
    }
    LOG_TRACE("Transfer command pool created successfully.");

    return true;
}

void vulkan_device_destroy(vulkan_context* context, vulkan_device* device)
{
    // Освобождение пулов команд.
    vkDestroyCommandPool(context->device.logical, device->graphics_queue.command_pool, context->allocator);
    LOG_TRACE("Graphics command pool destroy complete.");

    if(device->graphics_queue.family_index != device->present_queue.family_index)
    {
        vkDestroyCommandPool(context->device.logical, device->present_queue.command_pool, context->allocator);
        LOG_TRACE("Present command pool destroy complete.");
    }

    vkDestroyCommandPool(context->device.logical, device->compute_queue.command_pool, context->allocator);
    LOG_TRACE("Compute command pool destroy complete.");

    vkDestroyCommandPool(context->device.logical, device->transfer_queue.command_pool, context->allocator);
    LOG_TRACE("Transfer command pool destroy complete.");

    vkDestroyDevice(device->logical, context->allocator);
    LOG_TRACE("Logical device destroy complete.");

    // Обнуление переменных.
    mzero(device, sizeof(vulkan_device));
}

bool vulkan_device_enumerate_physical_devices(vulkan_context* context, u32* physical_device_count, vulkan_physical_device* physical_devices)
{
    // Получение количества доступных физических устройств.
    if(physical_devices == nullptr)
    {
        VkResult result = vkEnumeratePhysicalDevices(context->instance, physical_device_count, nullptr);
        if(!vulkan_result_is_success(result) || (*physical_device_count) == 0)
        {
            LOG_ERROR("Failed to get physical device count: %s.", vulkan_result_get_string(result));
            return false;
        }

        return true;
    }

    // Получение списка доступных физических устройств.
    VkPhysicalDevice* physical_device_handlers = darray_create_custom(VkPhysicalDevice, (*physical_device_count));
    VkResult result = vkEnumeratePhysicalDevices(context->instance, physical_device_count, physical_device_handlers);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to enumerate physical devices: %s.", vulkan_result_get_string(result));
        darray_destroy(physical_device_handlers);
        return false;
    }

    // Обнуление полученного массива.
    mzero(physical_devices, sizeof(vulkan_physical_device) * (*physical_device_count));

    // Сбор информации по устройствам.
    for(u32 i = 0; i < (*physical_device_count); ++i)
    {
        vulkan_physical_device* physical = &physical_devices[i];
        physical->handle = physical_device_handlers[i];
        vkGetPhysicalDeviceFeatures(physical->handle, &physical->features);
        vkGetPhysicalDeviceProperties(physical->handle, &physical->properties);
        vkGetPhysicalDeviceMemoryProperties(physical->handle, &physical->memory_properties);

        u32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical->handle, &queue_family_count, nullptr);
        VkQueueFamilyProperties* queue_families = darray_create_custom(VkQueueFamilyProperties, queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical->handle, &queue_family_count, queue_families);

        // Подсчет очередей, так же указывает на наличие очередей.
        for(u32 j = 0; j < queue_family_count; ++j)
        {
            u32 queue_count = queue_families[j].queueCount;
            VkQueueFlags queue_flags = queue_families[j].queueFlags;

            if((queue_flags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT)
            {
                physical->queue_graphics_count += queue_count;
            }

            if((queue_flags & VK_QUEUE_COMPUTE_BIT)  == VK_QUEUE_COMPUTE_BIT)
            {
                physical->queue_compute_count += queue_count;
            }

            if((queue_flags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT)
            {
                physical->queue_transfer_count += queue_count;
            }

            if(platform_window_supports_vulkan_presentation(context->window, physical->handle, j))
            {
                physical->queue_present_count += queue_count;
            }

            physical->queue_total_count += queue_count;
        }

        // Освобождение памяти.
        darray_destroy(queue_families);
    }

    // Освобождение памяти.
    darray_destroy(physical_device_handlers);

    return true;
}

const char* vulkan_device_get_physical_device_type_str(VkPhysicalDeviceType type)
{
    static const char* type_names[] = {
        [VK_PHYSICAL_DEVICE_TYPE_OTHER]          = "unknown",
        [VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU] = "integrated gpu",
        [VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU]   = "discrete gpu",
        [VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU]    = "virtual gpu",
        [VK_PHYSICAL_DEVICE_TYPE_CPU]            = "cpu"
    };

    static const u32 type_name_count = sizeof(type_names) / sizeof(char*);

    if((u32)type >= type_name_count)
    {
        LOG_WARN("Unknown vulkan physical device type: %d.", type);
        return type_names[0];
    }

    return type_names[type];
}
