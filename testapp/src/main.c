#include <application.h>
#include <core/logger.h>
#include <core/memory.h>
#include <core/string.h>
#include <core/timer.h>
#include <core/input.h>
#include <core/event.h>

#include <renderer/renderer.h>
#include <renderer/camera.h>
#include <math/vector.h>
#include <math/matrix.h>

// TODO: Временно!!!
static camera cam;
static mat4 proj;
static mat4 view;
static u32 width_cached;
static u32 height_cached;
static bool cursor_locked;

static shader_t world_shader;
static u32 world_shader_camera;

static renderer_camera_t world_camera;
static renderer_model_t plane_model;
static buffer_t vertex_buffer;
static buffer_t index_buffer;

// TODO: Временно.
static shader_stage_file_t shader_stage_files[] = {
    { "../assets/shaders/WorldShader.vert.spv", SHADER_STAGE_VERTEX   },
    { "../assets/shaders/WorldShader.frag.spv", SHADER_STAGE_FRAGMENT }
};

static bool game_initialize(const application_config* config)
{
    UNUSED(config);

    if(!renderer_shader_create(ARRAY_SIZE(shader_stage_files), shader_stage_files, &world_shader))
    {
        LOG_ERROR("Failed to load world shader.");
        return false;
    }
    LOG_TRACE("World shader created successfully.");

    // TODO: Временно!
    renderer_shader_acquire_resource(&world_shader, 0, &world_shader_camera);

    if(!renderer_buffer_create(BUFFER_TYPE_VERTEX, sizeof(vertex3d) * 1000, &vertex_buffer))
    {
        LOG_ERROR("Failed to create vertex buffer.");
        return false;
    }

    if(!renderer_buffer_create(BUFFER_TYPE_INDEX, sizeof(u32) * 1000, &index_buffer))
    {
        LOG_ERROR("Failed to create index buffer.");
        return false;
    }
    LOG_TRACE("Vertex/index buffers created successfully.");

    u32 indices[] = {0, 1, 2, 0, 3, 1};
    vertex3d verts[] = {
        { {{ -0.5, -0.5, 0.0 }}, {{ 1.0, 0.0, 0.0, 1.0 }} }, // 0
        { {{  0.5,  0.5, 0.0 }}, {{ 0.0, 1.0, 0.0, 1.0 }} }, // 1
        { {{ -0.5,  0.5, 0.0 }}, {{ 0.0, 0.0, 1.0, 1.0 }} }, // 2
        { {{  0.5, -0.5, 0.0 }}, {{ 1.0, 1.0, 0.0, 1.0 }} }, // 3
    };

    if(!renderer_buffer_load_range(&vertex_buffer, 0, sizeof(verts), verts))
    {
        LOG_ERROR("Failed to load verts data.");
        return false;
    }

    if(!renderer_buffer_load_range(&index_buffer, 0, sizeof(indices), indices))
    {
        LOG_ERROR("Failed to load indices data.");
        return false;
    }
    LOG_TRACE("Test geometry loaded successfully.");

    // Перспектива.
    const f32 aspect = (f32)config->window.width / config->window.height;
    const f32 fov_rad = math_deg_to_rad(60);
    proj = mat4_perspective(fov_rad, aspect, 0.1f, 1000.0f);

    // Какмера.
    cam  = camera_init(vec3_backward());
    view = camera_get_view(&cam);
    world_camera.proj = proj;
    world_camera.view = view;

    // Модель.
    plane_model.transform = mat4_identity();

    width_cached = config->window.width;
    height_cached = config->window.height;

    cursor_locked = false;
    application_set_cursor_lock(cursor_locked);

    return true;
}

static void game_shutdown()
{
    renderer_wait_idle_device();

    renderer_buffer_destroy(&vertex_buffer);
    renderer_buffer_destroy(&index_buffer);
    LOG_TRACE("Vertex/index buffers destroy complete.");

    renderer_shader_destroy(&world_shader);
    LOG_TRACE("World shader destroy complete.");
}

static void game_on_resize(u32 width, u32 height)
{
    renderer_frame_resize(width, height);

    // TODO: Временно для камеры!
    const f32 aspect = (f32)width / height;
    mat4_perspective_update_aspect(&proj, aspect);

    width_cached = width;
    height_cached = height;
}

static bool game_update(f32 delta_time)
{
    // Завершение приложения.
    if(input_key_down('Q'))
    {
        application_quit();
    }

    // TODO: При поворотах меняется управление WASD!!! Проблема в углах Эйлера!
    // Управление камерой.
    {
        f32 speed = 2 * delta_time;

        if(input_key_held(KEY_LSHIFT))
        {
            speed *= 3.0f;
        }

        if(input_key_down(KEY_TAB))
        {
            cursor_locked = !cursor_locked;
            application_set_cursor_lock(cursor_locked);
        }

        if(input_key_held('W'))
        {
            camera_move_forward(&cam, speed);
        }

        if(input_key_held('S'))
        {
            camera_move_backward(&cam, speed);
        }

        if(input_key_held('A'))
        {
            camera_move_left(&cam, speed);
        }

        if(input_key_held('D'))
        {
            camera_move_right(&cam, speed);
        }

        if(input_key_held(KEY_SPACE))
        {
            camera_move_up(&cam, speed);
        }

        if(input_key_held(KEY_LCONTROL))
        {
            camera_move_down(&cam, speed);
        }

        i32 mouse_dx, mouse_dy;
        if(cursor_locked && input_mouse_delta(&mouse_dx, &mouse_dy))
        {
            const f32 mouse_sens = 450;
            f32 yaw_deg = -(f32)mouse_dx / (f32)width_cached * mouse_sens;
            f32 pitch_deg = -(f32)mouse_dy / (f32)height_cached * mouse_sens;

            camera_rotate(&cam, yaw_deg, pitch_deg);
        }

        // Обновление состояния камеры.
        if(cam.wait_update_view)
        {
            view = camera_get_view(&cam);
        }
    }

    // Вывод статистики на консоль.
    {
        if(input_key_down('M'))
        {
            const char* meminfo = memory_system_usage_str();
            LOG_DEBUG(meminfo);
            string_free(meminfo);
        }

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
    }

    return true;
}

static bool game_render(f32 delta_time)
{
    UNUSED(delta_time);

    renderer_frame_bind_shader(&world_shader);

    world_camera.proj = proj;
    world_camera.view = view;
    renderer_shader_update_resource_binding(&world_shader, world_shader_camera, 0, &world_camera);
    renderer_shader_apply_resource(&world_shader, world_shader_camera);

    renderer_shader_update_model(&world_shader, &plane_model);

    renderer_frame_bind_buffer(&vertex_buffer, 0);
    renderer_frame_bind_buffer(&index_buffer, 0);
    renderer_frame_draw_indexed(6);

    return true;
}

int main()
{
    application_config config = {0};
    config.initialize = game_initialize;
    config.shutdown = game_shutdown;
    config.on_resize = game_on_resize;
    config.update = game_update;
    config.render = game_render;
    config.performance.target_fps = 60;
    config.window.backend_type = PLATFORM_WINDOW_BACKEND_DEFAULT;
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
