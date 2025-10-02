#include <application.h>
#include <core/logger.h>
#include <core/memory.h>
#include <core/string.h>
#include <core/timer.h>
#include <core/input.h>
#include <core/event.h>

bool game_event_handler(event_code code, void* sender, void* listener, event_context* data)
{
    UNUSED(sender);
    UNUSED(listener);

    switch(code)
    {
        case EVENT_CODE_APPLICATION_QUIT:
            LOG_DEBUG("Game event quit.");
            break;
        case EVENT_CODE_APPLICATION_RESIZE:
            LOG_DEBUG("Game event resize to %ux%u.", data->u32[0], data->u32[1]);
            break;
        case EVENT_CODE_APPLICATION_FOCUS:
            LOG_DEBUG("Game event focus state %s.", (data->u32[0] ? "FOCUSED" : "LOST FOCUS"));
            break;
        case EVENT_CODE_KEYBOARD_KEY:
            LOG_DEBUG("Game event key %s, state %s, unicode %u.", input_key_to_str(data->u32[0]), (data->u32[2] ? "PRESSED" : "RELEASED"), data->u32[1]);
            break;
        case EVENT_CODE_MOUSE_BUTTON:
            LOG_DEBUG("Game event button %s, state %s.", input_mouse_button_to_str(data->u32[0]), (data->u32[1] ? "PRESSED" : "RELEASED"));
            break;
        case EVENT_CODE_MOUSE_WHEEL:
            LOG_DEBUG("Game event wheel v:%i, h:%i.", data->i32[0], data->i32[1]);
            break;
        default:
            return false;
    }

    return true;
}

static bool game_initialize(const application_config* config)
{
    UNUSED(config);

    event_register(EVENT_CODE_APPLICATION_QUIT, nullptr, game_event_handler);
    event_register(EVENT_CODE_APPLICATION_RESIZE, nullptr, game_event_handler);
    event_register(EVENT_CODE_APPLICATION_FOCUS, nullptr, game_event_handler);
    event_register(EVENT_CODE_KEYBOARD_KEY, nullptr, game_event_handler);
    event_register(EVENT_CODE_MOUSE_BUTTON, nullptr, game_event_handler);
    // event_register(EVENT_CODE_MOUSE_MOVE, nullptr, game_event_handler);
    event_register(EVENT_CODE_MOUSE_WHEEL, nullptr, game_event_handler);

    return true;
}

static void game_shutdown()
{
}

static void game_on_resize(u32 width, u32 height)
{
    UNUSED(width);
    UNUSED(height);
}

static bool game_update(f32 delta_time)
{
    UNUSED(delta_time);

    if(input_key_down('F'))
    {
        application_frame_stats stats = application_get_frame_stats();

        timer_format window_ft, update_ft, render_ft, frame_ft, sleep_ft, sleep_actual_ft, sleep_error_ft;
        timer_get_format(stats.window_time, &window_ft);
        timer_get_format(stats.update_time, &update_ft);
        timer_get_format(stats.render_time, &render_ft);
        timer_get_format(stats.frame_time, &frame_ft);
        timer_get_format(stats.sleep_time, &sleep_ft);
        timer_get_format(stats.sleep_actual_time, &sleep_actual_ft);
        timer_get_format(stats.sleep_error_time, &sleep_error_ft);

        LOG_DEBUG(
            "Frame stats:\n\nWindow time %.2f%s\nUpdate time: %.2f%s\nRender time %.2f%s\nFrame time %.2f%s\nSleep time %.2f%s\nReally sleep time %.2f%s\nError sleep time %.2f%s\nCurrent FPS %hu\n",
            window_ft.amount, window_ft.unit, update_ft.amount, update_ft.unit, render_ft.amount, render_ft.unit,
            frame_ft.amount, frame_ft.unit, sleep_ft.amount, sleep_ft.unit, sleep_actual_ft.amount, sleep_actual_ft.unit,
            sleep_error_ft.amount, sleep_error_ft.unit, stats.fps
        );
    }
    else if(input_key_down('M'))
    {
        const char* meminfo = memory_system_usage_str();
        LOG_DEBUG(meminfo);
        string_free(meminfo);
    }
    else if(input_key_down('Q'))
    {
        application_quit();
    }

    return true;
}

static bool game_render(f32 delta_time)
{
    UNUSED(delta_time);
    return true;
}

int main()
{
    // TODO: Временное отключение TRACE!
    // log_set_level(LOG_LEVEL_DEBUG);

    application_config config = {0};
    config.initialize = game_initialize;
    config.shutdown = game_shutdown;
    config.on_resize = game_on_resize;
    config.update = game_update;
    config.render = game_render;
    config.performance.target_fps = 60;
    config.window.backend_type = PLATFORM_WINDOW_BACKEND_TYPE_AUTO;
    config.window.title  = "Simple window";
    config.window.width  = 1024;
    config.window.height = 768;

    if(!application_initialize(&config))
    {
        LOG_ERROR("Failed to initialize application.");
        application_terminate();
        return false;
    }

    if(!application_run())
    {
        LOG_ERROR("Failed to run application.");
        application_terminate();
        return false;
    }

    return true;
}
