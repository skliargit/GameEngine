#pragma once

#include <core/defines.h>
#include <renderer/vulkan/vulkan_types.h>

/**/
bool vulkan_shader_create(vulkan_context* context, vulkan_shader* out_shader);

/**/
void vulkan_shader_destroy(vulkan_context* context, vulkan_shader* shader);

/**/
void vulkan_shader_use(vulkan_context* context, vulkan_shader* shader);

/**/
void vulkan_shader_update_camera(vulkan_context* context, vulkan_shader* shader, renderer_camera* camera);
