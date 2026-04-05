#pragma once

#include <core/defines.h>
#include <renderer/types.h>

/**/
bool renderer_initialize(renderer_config* config);

/**/
void renderer_shutdown();

/**/
CORE_API bool renderer_system_is_initialized();

/**/
void renderer_on_resize(u32 width, u32 height);

/**/
bool renderer_frame_begin();

/**/
bool renderer_frame_end();

/**/
CORE_API void renderer_update_camera(const mat4* proj, const mat4* view);

/**/
CORE_API void renderer_update_model(const mat4* model);
