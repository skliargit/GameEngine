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

        char buffer[4000] = "\n\nFrame timings:";

        u32 msg_length = string_length(buffer);
        u32 buf_offset = msg_length;
        u32 buf_length = ARRAY_SIZE(buffer) - msg_length;

        //---------------------------------------------------------------------------------------------------------------------

        timer_format total_tf, delta_tf;
        timer_get_format(stats.frame_time + stats.sleep_actual_time, &total_tf);
        timer_get_format(delta_time, &delta_tf); // TODO: Взять дельту предыдущего кадра!

        msg_length = string_format(buffer + buf_offset, buf_length, " (total %.2f%s, delta %.2f%s, FPS %hu)\n",
                                   total_tf.amount, total_tf.unit, delta_tf.amount, delta_tf.unit, stats.fps);
        buf_offset += msg_length;
        buf_length -= msg_length;

        //---------------------------------------------------------------------------------------------------------------------

        timer_format window_tf;
        timer_get_format(stats.window_time, &window_tf);

        msg_length = string_format(buffer + buf_offset, buf_length, "  Window events  : %.2f%s\n", window_tf.amount, window_tf.unit);
        buf_offset += msg_length;
        buf_length -= msg_length;

        //---------------------------------------------------------------------------------------------------------------------

        timer_format update_tf;
        timer_get_format(stats.update_time, &update_tf);

        msg_length = string_format(buffer + buf_offset, buf_length, "  Physics update : %.2f%s\n", update_tf.amount, update_tf.unit);
        buf_offset += msg_length;
        buf_length -= msg_length;

        //---------------------------------------------------------------------------------------------------------------------

        timer_format render_tf;
        timer_get_format(stats.render_time, &render_tf);

        msg_length = string_format(buffer + buf_offset, buf_length, "  Renderer draw  : %.2f%s\n", render_tf.amount, render_tf.unit);
        buf_offset += msg_length;
        buf_length -= msg_length;

        //---------------------------------------------------------------------------------------------------------------------

        timer_format frame_tf;
        timer_get_format(stats.frame_time, &frame_tf);

        msg_length = string_format(buffer + buf_offset, buf_length, "  Frame actual   : %.2f%s\n", frame_tf.amount, frame_tf.unit);
        buf_offset += msg_length;
        buf_length -= msg_length;

        //---------------------------------------------------------------------------------------------------------------------

        timer_format sleep_expected_tf;
        timer_get_format(stats.sleep_expected_time, &sleep_expected_tf);

        msg_length = string_format(buffer + buf_offset, buf_length, "  Sleep expected : %.2f%s\n", sleep_expected_tf.amount, sleep_expected_tf.unit);
        buf_offset += msg_length;
        buf_length -= msg_length;

        //---------------------------------------------------------------------------------------------------------------------

        timer_format sleep_actual_tf;
        timer_get_format(stats.sleep_actual_time, &sleep_actual_tf);

        msg_length = string_format(buffer + buf_offset, buf_length, "  Sleep actual   : %.2f%s\n", sleep_actual_tf.amount, sleep_actual_tf.unit);
        buf_offset += msg_length;
        buf_length -= msg_length;

        //---------------------------------------------------------------------------------------------------------------------

        timer_format sleep_error_tf;
        timer_get_format(stats.sleep_error_time, &sleep_error_tf);

        msg_length = string_format(buffer + buf_offset, buf_length, "  Sleep error    : %.2f%s\n", sleep_error_tf.amount, sleep_error_tf.unit);
        buf_offset += msg_length;
        buf_length -= msg_length;

        //---------------------------------------------------------------------------------------------------------------------

        LOG_DEBUG(buffer);
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
