#include "renderer/vulkan/vulkan_shader.h"
#include "renderer/vulkan/vulkan_result.h"
#include "renderer/vulkan/vulkan_buffer.h"

#include "core/logger.h"
#include "core/memory.h"
#include "platform/file.h"
#include "math/types.h"

// TODO: Временно! Сделать shader_config, shader_stage и другие.
typedef struct shader_stage_file {
    char* path;
    VkShaderStageFlagBits stage;
} shader_stage_file;

static bool shader_create_modules(vulkan_context* context, const shader_stage_file* shader_stage_files, u32 shader_stage_count, VkPipelineShaderStageCreateInfo* out_shader_stages)
{
    // Очистка структуры.
    mzero(out_shader_stages, sizeof(VkPipelineShaderStageCreateInfo) * shader_stage_count);

    // Создание модулей шейдеров и формирование объектов программируемых стадий конвейера.
    for(u32 i = 0; i < shader_stage_count; ++i)
    {
        platform_file* file;

        if(platform_file_exists(shader_stage_files[i].path) == false)
        {
            LOG_ERROR("Shader file '%s' does not exist.", shader_stage_files[i].path);
            return false;
        }

        // TODO: Систему ресурсов.
        if(platform_file_open(shader_stage_files[i].path, PLATFORM_FILE_MODE_READ_BINARY, &file) == false)
        {
            LOG_ERROR("Unable to read shader file '%s'.", shader_stage_files[i].path);
            return false;
        }

        u64 file_size = 0;
        if(platform_file_size(file, &file_size) == false)
        {
            LOG_ERROR("Unable to get size of shader file '%s'.", shader_stage_files[i].path);
            return false;
        }

        u64 check_file_size = 0;
        u32* shader_bytes = mallocate(file_size, MEMORY_TAG_UNKNOWN);
        if(platform_file_read(file, file_size, shader_bytes, &check_file_size) == false || check_file_size != file_size)
        {
            LOG_ERROR("Unable to read data from shader file '%s'.", shader_stage_files[i].path);
            return false;
        }

        LOG_DEBUG("Shader file '%s' of size %u read successfully.", shader_stage_files[i].path, file_size);

        // Закрытие файла.
        platform_file_close(file);
        file = nullptr;

        VkShaderModuleCreateInfo shader_module_info = {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = file_size,
            .pCode    = shader_bytes
        };

        VkResult result = vkCreateShaderModule(context->device.logical, &shader_module_info, context->allocator, &out_shader_stages[i].module);
        if(!vulkan_result_is_success(result))
        {
            LOG_ERROR("Failed to create shader module from file '%s': %s.", shader_stage_files[i].path, vulkan_result_get_string(result));
            return false;
        }

        // Освобождение памяти для бинарных данных шейдера.
        mfree(shader_bytes, file_size, MEMORY_TAG_UNKNOWN);
        shader_bytes = nullptr;

        out_shader_stages[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        out_shader_stages[i].stage  = shader_stage_files[i].stage;
        out_shader_stages[i].pName  = "main"; // TODO: Сделать настраиваемой.
    }

    return true;
}

static void shader_destroy_modules(vulkan_context* context, u32 shader_stage_count, VkPipelineShaderStageCreateInfo* shader_stages)
{
    for(u32 i = 0; i < shader_stage_count; ++i)
    {
        if(shader_stages[i].module != nullptr)
        {
            vkDestroyShaderModule(context->device.logical, shader_stages[i].module, context->allocator);
        }
    }

    mzero(shader_stages, sizeof(VkPipelineShaderStageCreateInfo) * shader_stage_count);
}

bool vulkan_shader_create(vulkan_context* context, vulkan_shader* out_shader)
{
    // Создание программируемых стадий конфейера.
    u32 shader_stage_count = MAX_SHADER_STAGES;
    VkPipelineShaderStageCreateInfo shader_stages[MAX_SHADER_STAGES];

    // TODO: Сделать настраиваемым. Временно.
    static const shader_stage_file shader_stage_files[MAX_SHADER_STAGES] = {
        { "../assets/shaders/WorldShader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT   },
        { "../assets/shaders/WorldShader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
    };

    if(!shader_create_modules(context, shader_stage_files, shader_stage_count, shader_stages))
    {
        LOG_ERROR("Failed to create shader stages.");
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

    // Наборы дескрипторов конвейера.
    // TODO: Получить и проверить maxBoundDescriptorSets в VkPhysicalDeviceLimits через vkGetPhysicalDeviceProperties()!
    // TODO: Настраиваемые размещения (привязки).
    VkDescriptorSetLayoutBinding descriptor_set_bindings[] = {
        // Uniform буфер для вершиного шейдера.
        {
            .binding            = 0,                                 // Индекс размещения (привязки) данных для uniform переменной.
            .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // Тип связываемого дескриптора - uniform переменная.
            .descriptorCount    = 1,                                 // Кол-во связываемых дескрипторов с шейдером (например, передача массива в шейдер).
            .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,        // Стадия в которую передаются uniform переменные.
            .pImmutableSamplers = nullptr
        }
        // TODO: Другие размещения (привязки) данных: сэмплеры для фрагментного шейдера.
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = nullptr,
        .flags        = 0,
        .bindingCount = ARRAY_SIZE(descriptor_set_bindings),         // Кол-во размещений (привязок).
        .pBindings    = descriptor_set_bindings
    };

    VkResult result = vkCreateDescriptorSetLayout(context->device.logical, &descriptor_set_layout_info, context->allocator, &out_shader->descriptor_set_layout);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create descriptor set layouts: %s.", vulkan_result_get_string(result));
        return false;
    }

    // TODO: Push константы конвейера.

    // Макет конвейера, описывающий размещение (привязки) дескрипторных наборов и push-констант.
    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = 1,                                   // TODO: Настраиваемый.
        .pSetLayouts            = &out_shader->descriptor_set_layout,  // TODO: Настраиваемый.
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr
    };

    result = vkCreatePipelineLayout(context->device.logical, &pipeline_layout_info, context->allocator, &out_shader->pipeline_layout);
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
        .colorAttachmentCount    = 1,                                        // TODO: Должен соответствовать color_blend_attachment_count!!!
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
        .stageCount          = shader_stage_count,
        .pStages             = shader_stages,
        .layout              = out_shader->pipeline_layout,
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

    result = vkCreateGraphicsPipelines(context->device.logical, nullptr, 1, &graphics_pipeline, context->allocator, &out_shader->pipeline);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create graphics pipeline: %s.", vulkan_result_get_string(result));
        return false;
    }

    // NOTE: Шейдерные модули представляют собой байт-код, и после создания конвейера их можно удалить,
    //       т.к. байт-код компилируется в машинный код и становится частью конвейера.
    shader_destroy_modules(context, shader_stage_count, shader_stages);

    // Пул дескрипторов.
    VkDescriptorPoolSize descriptor_pool_sizes[] = {
        {
            .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = context->swapchain.max_frames_in_flight      // на кадр TODO: максимальный или max_frames_in_flight.
        }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, // Указывает на возможность изменения набора дескрипторов.
        .maxSets       = context->swapchain.max_frames_in_flight,           // Максамально возможное кол-во наборов дескрипторов TODO: максимальный или max_frames_in_flight.
        .poolSizeCount = ARRAY_SIZE(descriptor_pool_sizes),                 // Кол-во описаний размеров пула.
        .pPoolSizes    = descriptor_pool_sizes
    };

    result = vkCreateDescriptorPool(context->device.logical, &descriptor_pool_info, context->allocator, &out_shader->descriptor_pool);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create descriptor pool: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Выделение дескрипторных наборов (по одному на кадр).
    // Зарезервировано 3, т.к. кол-во кадров в обработке 2-3 (см. max_frames_in_flight).
    VkDescriptorSetLayout descriptor_set_layouts[3] = {
        out_shader->descriptor_set_layout,
        out_shader->descriptor_set_layout,
        out_shader->descriptor_set_layout
    };

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = nullptr,
        .descriptorPool     = out_shader->descriptor_pool,
        .descriptorSetCount = context->swapchain.max_frames_in_flight,
        .pSetLayouts        = descriptor_set_layouts
    };

    result = vkAllocateDescriptorSets(context->device.logical, &descriptor_set_allocate_info, out_shader->descriptor_sets);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to allocate descriptor sets: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Создание uniform буфера.
    // TODO: Проверить и установить размер minUniformBufferOffsetAlignment.
    if(!vulkan_buffer_create(context, VULKAN_BUFFER_TYPE_UNIFORM, 3 * sizeof(renderer_camera), &out_shader->uniform_buffer))
    {
        LOG_ERROR("Failed to create uniform buffer.");
        return false;
    }

    return true;
}

void vulkan_shader_destroy(vulkan_context* context, vulkan_shader* shader)
{
    vulkan_buffer_destroy(context, &shader->uniform_buffer);

    // NOTE: Уничтожение shader->descriptor_sets происходит при уничтожении shader->descriptor_sets.

    if(shader->descriptor_pool != nullptr)
    {
        vkDestroyDescriptorPool(context->device.logical, shader->descriptor_pool, context->allocator);
    }

    if(shader->pipeline != nullptr)
    {
        vkDestroyPipeline(context->device.logical, shader->pipeline, context->allocator);
    }

    if(shader->pipeline_layout != nullptr)
    {
        vkDestroyPipelineLayout(context->device.logical, shader->pipeline_layout, context->allocator);
    }

    if(shader->descriptor_set_layout != nullptr)
    {
        vkDestroyDescriptorSetLayout(context->device.logical, shader->descriptor_set_layout, context->allocator);
    }

    mzero(shader, sizeof(vulkan_shader));
}

void vulkan_shader_use(vulkan_context* context, vulkan_shader* shader)
{
    u32 current_frame = context->swapchain.current_frame;
    VkCommandBuffer cmdbuf = context->graphics_command_buffers[current_frame];

    // TODO: Настраиваемая точка привязки (графика или вычисления).
    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline);
}

void vulkan_shader_update_camera(vulkan_context* context, vulkan_shader* shader, renderer_camera* camera)
{
    u32 current_frame = context->swapchain.current_frame;
    VkCommandBuffer cmdbuf = context->graphics_command_buffers[current_frame];
    VkDescriptorSet descriptor_set = shader->descriptor_sets[current_frame];

    // Указание в какую часть буфера зугружать данные.
    u64 load_size = sizeof(renderer_camera);
    u64 load_offset = current_frame * load_size;

    // Загрузка новых данных в буфер.
    vulkan_buffer_load_data(context, &shader->uniform_buffer, load_offset, load_size, camera);

    // Обновление дескрипторного набора.
    VkDescriptorBufferInfo descriptor_buffer_info = {
        .buffer = shader->uniform_buffer.handle,
        .offset = load_offset,
        .range  = load_size
    };

    VkWriteDescriptorSet write_descriptor_set = {
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext            = nullptr,
        .dstSet           = descriptor_set,
        .dstBinding       = 0,
        .dstArrayElement  = 0,
        .descriptorCount  = 1,
        .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo       = nullptr,
        .pBufferInfo      = &descriptor_buffer_info,
        .pTexelBufferView = nullptr
    };

    vkUpdateDescriptorSets(context->device.logical, 1, &write_descriptor_set, 0, nullptr);

    // Привязка набора дескрипторов к размещению для обновления.
    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
}
