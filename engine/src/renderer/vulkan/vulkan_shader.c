#include "renderer/vulkan/vulkan_shader.h"
#include "renderer/vulkan/vulkan_result.h"

#include "core/logger.h"
#include "core/memory.h"
#include "platform/file.h"
#include "math/math_types.h"

// TODO: Временно!
typedef struct shader_file_info {
    char* path;
    VkShaderStageFlagBits stage;
} shader_file_info;

bool vulkan_shader_create(vulkan_context* context, vulkan_shader* out_shader)
{
    static const shader_file_info shader_file[MAX_SHADER_STAGES] = {
        { "../assets/shaders/WorldShader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT   },
        { "../assets/shaders/WorldShader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
    };

    // NOTE: Объявлено здесь для уничтожения после создания пайплайна.
    VkShaderModule shader_modules[MAX_SHADER_STAGES] = {};

    // Программируемые стадии конвейера.
    u32 shader_stage_count = MAX_SHADER_STAGES;
    VkPipelineShaderStageCreateInfo shader_stages[MAX_SHADER_STAGES] = {};

    // Создание модулей шейдеров и формирование объектов программируемых стадий конвейера.
    for(u32 i = 0; i < MAX_SHADER_STAGES; ++i)
    {
        platform_file* tmp_file;

        if(platform_file_exists(shader_file[i].path) == false)
        {
            LOG_ERROR("File '%s' does not exist.", shader_file[i].path);
            return false;
        }

        if(platform_file_open(shader_file[i].path, PLATFORM_FILE_MODE_READ_BINARY, &tmp_file) == false)
        {
            LOG_ERROR("Unable to read file '%s'.", shader_file[i].path);
            return false;
        }

        u64 tmp_file_size = 0;
        if(platform_file_size(tmp_file, &tmp_file_size) == false)
        {
            LOG_ERROR("Unable to get size of file '%s'.", shader_file[i].path);
            return false;
        }

        u64 check_file_size = 0;
        u32* shader_bytes = mallocate(tmp_file_size, MEMORY_TAG_UNKNOWN);
        if(platform_file_read(tmp_file, tmp_file_size, shader_bytes, &check_file_size) == false || check_file_size != tmp_file_size)
        {
            LOG_ERROR("Unable to read data from file '%s'.", shader_file[i].path);
            return false;
        }

        LOG_DEBUG("Shader file '%s' of size %u read successfully.", shader_file[i].path, tmp_file_size);

        // Закрытие файла.
        platform_file_close(tmp_file);
        tmp_file = nullptr;

        VkShaderModuleCreateInfo shader_module_info = {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = tmp_file_size,
            .pCode    = shader_bytes
        };

        VkResult result = vkCreateShaderModule(context->device.logical, &shader_module_info, context->allocator, &shader_modules[i]);
        if(!vulkan_result_is_success(result))
        {
            LOG_ERROR("Failed to create shader module from file '%s': %s.", shader_file[i].path, vulkan_result_get_string(result));
            return false;
        }

        // Освобождение памяти для бинарных данных шейдера.
        mfree(shader_bytes, tmp_file_size, MEMORY_TAG_UNKNOWN);
        shader_bytes = nullptr;

        shader_stages[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stages[i].stage  = shader_file[i].stage;
        shader_stages[i].module = shader_modules[i];
        shader_stages[i].pName  = "main"; // TODO: Сделать настраиваемой.
    }

    // TODO: Дескрипторы.
    // NOTE: maxBoundDescriptorSets в VkPhysicalDeviceLimits через vkGetPhysicalDeviceProperties()!
    //       стр. 179 руководства разработчика.
    // VkDescriptorSetLayoutBinding descriptor_set_bindings[];
    // VkDescriptorSetLayoutCreateInfo descriptor_set_layout = {
    //     .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    //     .pNext        = nullptr,
    //     .flags        = 0,
    //     .bindingCount = 0,
    //     .pBindings    = nullptr
    // };
    // vkCreateDescriptorSetLayout(context->device.logical, &descriptor_set_layout, context->allocator, nullptr);

    // TODO: Макет конвейера, описывающий размещение дескрипторных наборов и push-констант.
    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = 0,
        .pSetLayouts            = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr
    };

    VkResult result = vkCreatePipelineLayout(context->device.logical, &pipeline_layout_info, context->allocator, &out_shader->pipeline_layout);
    if(!vulkan_result_is_success(result))
    {
        LOG_ERROR("Failed to create graphics pipeline layout: %s.", vulkan_result_get_string(result));
        return false;
    }

    // Входные данные вершинного шейдер: настройка привязок и передаваемых атрибутов.
    // TODO: Настраиваемый.
    VkVertexInputBindingDescription binding_descriptions[] = {
        // Описание данных и частоты обновления каждой вершины для vertex3d.
        {
            .binding   = 0,                           // Индекс привязки данных.
            .stride    = sizeof(vertex3d),            // Размер выршины (шаг данных). // TODO: Выравнивание для производительности!
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX  // Частота обновления данных: для каждой вершины.
        }
    };

    u32 binding_description_count = sizeof(binding_descriptions) / sizeof(VkVertexInputBindingDescription);

    // TODO: Настраиваемый.
    VkVertexInputAttributeDescription attribute_descriptions[] = {
        // Описание атрибутов для vertex3d.
        // Position.
        {
            .binding  = 0,                                    // Индекс привязки (описан выше).
            .location = 0,                                    // Номер смещения данных вершины для шейдера / layout(location = 0) in vec3 in_position.
            .format   = VK_FORMAT_R32G32B32_SFLOAT,           // Размер и тип передаваемых данных.
            .offset   = MEMBER_OFFSET_U32(vertex3d, position) // Смещение от начала данных вершины в байтах.
        },
        // Color
        {
            .binding  = 0,                                    // Индекс привязки (описан выше).
            .location = 1,                                    // Номер смещения данных вершины для шейдера / layout(location = 1) in vec4 in_color.
            .format   = VK_FORMAT_R32G32B32A32_SFLOAT,        // Размер и тип передаваемых данных.
            .offset   = MEMBER_OFFSET_U32(vertex3d, color)    // Смещение от начала данных вершины в байтах.
        }
    };

    u32 attribute_description_count = sizeof(attribute_descriptions) / sizeof(VkVertexInputAttributeDescription);

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = binding_description_count,
        .pVertexBindingDescriptions      = binding_descriptions,
        .vertexAttributeDescriptionCount = attribute_description_count,
        .pVertexAttributeDescriptions    = attribute_descriptions,
    };

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
    VkPipelineRasterizationStateCreateInfo rasterization_state = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = VK_FALSE,                        // Отсекать фрагменты за пределами дальней и ближней плоскости.
        .rasterizerDiscardEnable = VK_FALSE,                        // Выполнять растерезацию и передавать во фреймбуфер.
        .polygonMode             = VK_POLYGON_MODE_FILL,            // Ребра полигона отрезки (VK_POLYGON_MODE_LINE) или полигон полностью заполняет фрагмент (VK_POLYGON_MODE_FILL).
        .cullMode                = VK_CULL_MODE_BACK_BIT,           // Тип отсечения поверхностей (VK_CULL_MODE_*: NONE, FRONT, FRONT_AND_BACK, BACK).
        .frontFace               = VK_FRONT_FACE_CLOCKWISE,         // Порядок обхода вершин (в данный момент - против часовой стрелки).
        .depthBiasEnable         = VK_FALSE,                        // TODO: Нужно ли использовать? Для изменения значения глубины?
        .depthBiasConstantFactor = 0.0f,                            // TODO: Настраиваемый.
        .depthBiasClamp          = 0.0f,                            // TODO: Настраиваемый.
        .depthBiasSlopeFactor    = 0.0f,                            // TODO: Настраиваемый.
        .lineWidth               = 1.0f                             // TODO: Ширина линии (динамическое состояние).
    };

    // Multisample state.
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

    // NOTE: Количество элементов должно соответствовать количеству attachment-ов передаваемых при динамическом рендере,
    //       а так же количеству выходных vec4 векторов во фрагментном шейдере!
    u32 color_blend_attachment_count = sizeof(color_blend_attachments) / sizeof(VkPipelineColorBlendAttachmentState);

    VkPipelineColorBlendStateCreateInfo color_blend_state = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable   = VK_FALSE,                     // TODO: Настраиваемый.
        .logicOp         = VK_LOGIC_OP_COPY,             // TODO: Настраиваемый.
        .attachmentCount = color_blend_attachment_count, // TODO: Настраиваемый.
        .pAttachments    = color_blend_attachments,      // TODO: Настраиваемый.
        // .blendConstants  = ,                             // TODO: Настраиваемый.
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
    //                         В результате эти настройки нужно указывать прямо во время отрисовки.
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,    // Изменение вьюпорта.
        VK_DYNAMIC_STATE_SCISSOR,     // Изменение отсечение вьюпорта.
        VK_DYNAMIC_STATE_LINE_WIDTH   // Изменение ширины отрезков (см. растеризатор).
    };

    const u32 dynamic_state_count = sizeof(dynamic_states) / sizeof(VkDynamicState);

    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamic_state_count,
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
    for(u32 i = 0; i < MAX_SHADER_STAGES; ++i)
    {
        if(shader_modules[i] != nullptr)
        {
            vkDestroyShaderModule(context->device.logical, shader_modules[i], context->allocator);
            shader_modules[i] = nullptr;
        }
    }

    return true;
}

void vulkan_shader_destroy(vulkan_context* context, vulkan_shader* shader)
{
    if(shader->pipeline)
    {
        vkDestroyPipeline(context->device.logical, shader->pipeline, context->allocator);
        shader->pipeline = nullptr;
    }

    if(shader->pipeline_layout)
    {
        vkDestroyPipelineLayout(context->device.logical, shader->pipeline_layout, context->allocator);
        shader->pipeline_layout = nullptr;
    }
}

void vulkan_shader_use(vulkan_context* context, vulkan_shader* shader)
{
    u32 current_frame = context->swapchain.current_frame;
    VkCommandBuffer cmdbuf = context->graphics_command_buffers[current_frame];

    // TODO: Настраиваемая точка привязки (графика или вычисления).
    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline);
}
