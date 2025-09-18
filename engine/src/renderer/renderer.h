#pragma once

#include <core/defines.h>
#include <renderer/renderer_types.h>

/*
*/
bool renderer_initialize(renderer_config* config);

/*
*/
void renderer_shutdown();

/*
*/
API bool renderer_system_is_initialized();

/*
*/
void renderer_on_resize(u32 width, u32 height);
