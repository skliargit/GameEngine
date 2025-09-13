#include <application.h>
#include <core/logger.h>
#include <core/memory.h>
#include <core/input.h>
#include <core/string.h>
#include <core/timer.h>

static bool game_initialize(const application_config* config)
{
    UNUSED(config);
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
    log_set_level(LOG_LEVEL_DEBUG);

    application_config config = {0};
    config.initialize = game_initialize;
    config.shutdown = game_shutdown;
    config.on_resize = game_on_resize;
    config.update = game_update;
    config.render = game_render;
    config.performance.target_fps = 60;
    config.window.backend_type = PLATFORM_WINDOW_BACKEND_TYPE_XCB;
    config.window.title  = "Simple window";
    config.window.width  = 1024;
    config.window.height = 768;

    application_initialize(&config);
    return application_run();
}
