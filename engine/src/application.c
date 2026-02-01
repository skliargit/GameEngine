#include "application.h"

#include "core/logger.h"
#include "core/timer.h"
#include "core/memory.h"
#include "core/input.h"
#include "core/event.h"

#include "debug/assert.h"
#include "platform/console.h"
#include "platform/memory.h"
#include "platform/time.h"
#include "platform/thread.h"
#include "platform/window.h"
#include "renderer/renderer.h"

typedef struct application_context {
    // Указатель на контекст окна приложения.
    platform_window* window;
    // Целевое время кадра в секундах для ограничения FPS.
    f64 target_frame_time;
    // Статистика производительности за последний завершенный кадр.
    application_frame_stats frame_stats;
    // Флаг выполнения главного цикла приложения.
    bool is_running;
    // Флаг приостановки работы приложения.
    bool is_suspended;
    // @brief Callback-функция, вызываемая при изменении размеров окна.
    application_resize_callback on_resize;
    // @brief Callback-функиця, вызываемая при обновлении логики приложения.
    application_update_callback update;
    // @brief Callback-функция, вызываемая при отрисовке кадра.
    application_render_callback render;
    // @brief Callback-функция, вызываемая при завершении работы проложения.
    application_shutdown_callback shutdown;
} application_context;

// Контекст приложения.
static application_context* context = nullptr;

void application_on_close()
{
    application_quit();
}

void application_on_resize(const u32 width, const u32 height)
{
    renderer_on_resize(width, height);
    context->on_resize(width, height);

    event_context data = {.u32[0] = width, .u32[1] = height};
    event_send(EVENT_CODE_APPLICATION_RESIZE, nullptr, &data);
}

void application_on_focus(const bool focus_state)
{
    event_context data = {.u32[0] = focus_state};
    event_send(EVENT_CODE_APPLICATION_FOCUS, nullptr, &data);
}

void application_on_key(const keyboard_key key, const bool press_state)
{
    input_keyboard_key_update(key, press_state);

    event_context data = {.u32[0] = key, .u32[1] = 0, .u32[2] = press_state};
    event_send(EVENT_CODE_KEYBOARD_KEY, nullptr, &data);
}

void application_on_mouse_button(const mouse_button btn, const bool press_state)
{
    input_mouse_button_update(btn, press_state);

    event_context data = { .u32[0] = btn, .u32[1] = press_state};
    event_send(EVENT_CODE_MOUSE_BUTTON, nullptr, &data);
}

void application_on_mouse_move(const i32 x, const i32 y)
{
    input_mouse_position_update(x, y);

    event_context data = {.i32[0] = x, .i32[1] = y};
    event_send(EVENT_CODE_MOUSE_MOVE, nullptr, &data);
}

void application_on_mouse_wheel(const i32 vertical_delta, const i32 horizontal_delta)
{
    input_mouse_wheel_update(vertical_delta, horizontal_delta);

    event_context data = {.i32[0] = vertical_delta, .i32[1] = horizontal_delta};
    event_send(EVENT_CODE_MOUSE_WHEEL, nullptr, &data);
}

bool application_initialize(const application_config* config)
{
    // NOTE: Ошибка утверждения на данном этапе говорит о том, что консоль успешно инициализирована.
    ASSERT(context == nullptr, "Application layer is already initialized.");

    // NOTE: Консольный вывод является необязательным, если приложение запускается только в оконном режиме,
    //       в случае запуска программы с использованием консоли она МОЖЕТ работать.
    platform_console_initialize();

    if(!config)
    {
        LOG_ERROR("%s requires a valid configuration pointer. Aborted.", __func__);
        application_terminate();
        return false;
    }

    if(!config->initialize || !config->shutdown || !config->on_resize || !config->update || !config->render)
    {
        LOG_ERROR("%s requires valid callback pointers in configuration. Aborted.", __func__);
        application_terminate();
        return false;
    }

    if(!platform_memory_initialize())
    {
        LOG_ERROR("Failed to initialize platform memory subsystem. Unable to continue.");
        application_terminate();
        return false;
    }
    LOG_INFO("Memory subsystem initialized successfully.");

    if(!platform_time_initialize())
    {
        LOG_ERROR("Failed to initialize platform time subsystem. Unable to continue.");
        application_terminate();
        return false;
    }
    LOG_INFO("Time subsystem initialized successfully.");

    if(!platform_thread_initialize())
    {
        LOG_ERROR("Failed to initialize platform thread subsystem. Unable to continue.");
        application_terminate();
        return false;
    }
    LOG_INFO("Thread subsystem initialized successfully.");

    if(!memory_system_initialize())
    {
        LOG_ERROR("Failed to initialize memory system. Unable to continue.");
        application_terminate();
        return false;
    }
    LOG_INFO("Memory system initialized successfully.");

    if(!input_system_initialize())
    {
        LOG_ERROR("Failed to initialize input system. Unable to continue.");
        application_terminate();
        return false;
    }
    LOG_INFO("Input system initialized successfully.");

    if(!event_system_initialize())
    {
        LOG_ERROR("Failed to initialize event system. Unable to continue.");
        return false;
    }
    LOG_INFO("Event system initialized successfully.");

    if(!platform_window_initialize(config->window.backend_type))
    {
        LOG_ERROR("Failed to initialize window subsystem. Unable to continue.");
        application_terminate();
        return false;
    }
    LOG_INFO("Window subsystem initialized successfully.");

    context = mallocate(sizeof(application_context), MEMORY_TAG_APPLICATION);
    if(!context)
    {
        LOG_ERROR("Failed to allocate memory for application context.");
        application_terminate();
        return false;
    }
    mzero(context, sizeof(application_context));

    // Получение callback функций.
    context->shutdown = config->shutdown;
    context->on_resize = config->on_resize;
    context->update = config->update;
    context->render = config->render;

    // Установка количества кадров по умолчанию.
    if(config->performance.target_fps)
    {
        context->target_frame_time = 1.0 / (f64)config->performance.target_fps;
    }
    else
    {
        context->target_frame_time = 1.0 / 1000.0;
    }

    // Создание окна.
    platform_window_config windowcfg = {
        .title = config->window.title,
        .width = config->window.width,
        .height = config->window.height,
        .on_close = application_on_close,
        .on_resize = application_on_resize,
        .on_focus = application_on_focus,
        .on_key = application_on_key,
        .on_mouse_button = application_on_mouse_button,
        .on_mouse_move = application_on_mouse_move,
        .on_mouse_wheel = application_on_mouse_wheel
    };

    context->window = platform_window_create(&windowcfg);
    if(!context->window)
    {
        LOG_ERROR("Failed to create application window.");
        application_terminate();
        return false;
    }
    LOG_INFO("Window has been created successfully.");

    // TODO: При создании окна вызывает обновление экрана, пока рендерер еще не инициализирован!
    //       Как вариант позднее связывание функций обработчиков.

    // Инициализация рендерера.
    renderer_config rendercfg = {
        .backend_type = RENDERER_BACKEND_TYPE_VULKAN,
        .window = context->window
    };

    if(!renderer_initialize(&rendercfg))
    {
        LOG_ERROR("Failed to initialize renderer. Unable to continue.");
        application_terminate();
        return false;
    }
    LOG_INFO("Renderer initialized successfully.");

    if(!config->initialize(config))
    {
        LOG_ERROR("Failed to initialize user application.");
        application_terminate();
        return false;
    }
    LOG_INFO("User application initialized successfully.");

    // Обновление размеров приложения.
    // application_on_resize(config->window.width, config->window.height);

    return true;
}

bool application_run()
{
    ASSERT(context != nullptr, "Application should be initialized before running.");

    // Инициализация состояний.
    context->is_running = true;
    context->is_suspended = false;

    // Тайминги управления кадром и сбором статистики.
    timer frame_timer, physic_timer, stats_timer;
    timer_start(&frame_timer);
    timer_start(&physic_timer);

    application_frame_stats frame_stats = {0};
    f32 frame_time_accumulator = 0.0;
    // f32 physic_time_accumulator = 0.0;
    u16 frame_count = 0;

    while(context->is_running)
    {
        timer_reset(&stats_timer);

        if(!platform_window_poll_events())
        {
            LOG_ERROR("Failed to process window events.");
            application_terminate();
            return false;
        }

        // Время от начала кадра: обработки пользовательского ввода, и др. событий окна.
        frame_stats.window_time = timer_delta(&stats_timer);

        // Обновление игрового времени (всегда).
        f32 delta_time = CAST_F32(timer_delta(&physic_timer));

        // Сбор статистики FPS.
        frame_count++;
        frame_time_accumulator += delta_time;

        // Обновление статистики FPS.
        if(frame_time_accumulator >= 1.0)
        {
            frame_stats.fps = frame_count;
            frame_time_accumulator -= 1.0;
            frame_count = 0;
        }

        if(!context->is_suspended)
        {
            // TODO: Fixed Timestep для плавности + интерполяция шага.
            if(!context->update(delta_time))
            {
                LOG_ERROR("Update of user application failed. Shutting down.");
                application_terminate();
                return false;
            }

            // Время от начала кадра: обработки обновления логики приложения.
            frame_stats.update_time = timer_delta(&stats_timer);

            if(!context->render(delta_time))
            {
                LOG_ERROR("Render of user application failed. Shutting down.");
                application_terminate();
                return false;
            }

            // TODO: Временно.
            renderer_draw();

            // Время от начала кадра: отрисовка буфера.
            frame_stats.render_time = timer_delta(&stats_timer);
        }

        // Время от начала кадра и расчет времени сна с поправкой на ошибку.
        // NOTE: Если время кадра превысило заданное, то начало нового кадра после этой команды!
        frame_stats.frame_time = timer_delta(&frame_timer);
        frame_stats.sleep_expected_time = context->target_frame_time - frame_stats.frame_time - frame_stats.sleep_error_time;
        frame_stats.sleep_error_time = 0.0; // NOTE: Обнуляется для случая пропуска сна!

        if(frame_stats.sleep_expected_time >= 0.001)
        {
            // Возвращение управления операционной система, время сна.
            platform_thread_sleep(CAST_U32(frame_stats.sleep_expected_time * 1000.0));

            // Вычисление ошибки c ограничением в разумных пределах.
            // NOTE: Если время кадра не превысило заданное, то начало нового кадра после этой команды!
            frame_stats.sleep_actual_time = timer_delta(&frame_timer);

            const f64 half_range_time = context->target_frame_time * 0.5;
            frame_stats.sleep_error_time = frame_stats.sleep_actual_time - frame_stats.sleep_expected_time;
            frame_stats.sleep_error_time = CLAMP(frame_stats.sleep_error_time, -half_range_time, half_range_time);
        }

        // Обновление статистики предыдущего кадра.
        context->frame_stats = frame_stats;

        // Обновление состояния системы ввода.
        input_system_update();
    }

    application_terminate();
    return true;
}

void application_quit()
{
    if(context)
    {
        context->is_running = false;
        event_send(EVENT_CODE_APPLICATION_QUIT, nullptr, nullptr);
    }
}

// TODO: Переделать на exit()!
void application_terminate()
{
    if(context)
    {
        // Вызов завершения пользовательского приложения.
        if(context->shutdown)
        {
            context->shutdown();
            context->shutdown = nullptr;
            LOG_INFO("User application shutdown complete.");
        }

        // Завершение системы рендерера.
        if(renderer_system_is_initialized())
        {
            renderer_shutdown();
            LOG_INFO("Renderer shutdown complete.");
        }

        // Уничтожение окна приложения.
        if(context->window)
        {
            platform_window_destroy(context->window);
            context->window = nullptr;
            LOG_INFO("Window destroy complete.");
        }

        // Освобождение контекста.
        mfree(context, sizeof(application_context), MEMORY_TAG_APPLICATION);
        context = nullptr;
    }

    // Завершение подсистемы окон.
    if(platform_window_is_initialized())
    {
        platform_window_shutdown();
        LOG_INFO("Window subsystem shutdown complete.");
    }

    // Завершение системы событий.
    if(event_system_is_initialized())
    {
        event_system_shutdown();
        LOG_INFO("Event system shutdown complete.");
    }

    // Завершение системы ввода.
    if(input_system_is_initialized())
    {
        input_system_shutdown();
        LOG_INFO("Input system shutdown complete.");
    }

    // Завершение системы памяти.
    if(memory_system_is_initialized())
    {
        memory_system_shutdown();
        LOG_INFO("Memory system shutdown complete.");
    }

    // Завершение подсистемы потоков.
    if(platform_thread_is_initialized())
    {
        platform_thread_shutdown();
        LOG_INFO("Thread subsystem shutdown complete.");
    }

    // Завершение подсистемы таймера.
    if(platform_time_is_initialized())
    {
        platform_time_shutdown();
        LOG_INFO("Time subsystem shutdown complete.");
    }

    // Завершение подсистемы памяти.
    if(platform_memory_is_initialized())
    {
        platform_memory_shutdown();
        LOG_INFO("Memory subsystem shutdown complete.");
    }

    // Завершение подсистемы консоли.
    if(platform_console_is_initialized())
    {
        platform_console_shutdown();
    }
}

application_frame_stats application_get_frame_stats()
{
    if(context)
    {
        return context->frame_stats;
    }

    return (application_frame_stats){0};
}
