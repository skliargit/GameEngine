#include "application.h"

#include "core/logger.h"
#include "core/timer.h"
#include "core/memory.h"
#include "core/string.h"

#include "debug/assert.h"
#include "platform/console.h"
#include "platform/memory.h"
#include "platform/systimer.h"
#include "platform/thread.h"
#include "platform/window.h"

// TODO: Вынести в пространство пользователя.
typedef struct application_frame_stats {
    f64 window_time;        // Время оконных операций.
    f64 render_timr;        // Время рендера (в данный момент вывод нашей статистики).
    f64 frame_time;         // Время всего кадра.
    f64 sleep_time;         // Время необходимого сна (оставшееся время до конца кадра).
    f64 sleep_actual_time;  // Реальное время сна.
    f64 sleep_error_time;   // Ошибка сна (+ пересып / - недосып).
    u64 current_fps;        // Количество кадров в секунду.
} application_frame_stats;

typedef struct application_context {
    platform_window* window;
    bool is_running;
    bool is_suspended;

    // Настройки тайминга.
    // TODO: Конфигурируемый.
    f64 target_frame_time;  // Установленное время кадра.

    // Статистика производительности.
    application_frame_stats frame_stats;
} application_context;

static application_context* context = nullptr;

bool application_initialize(const application_config* config)
{
    ASSERT(context == nullptr, "Application layer is already initialized.");
    ASSERT(config != nullptr, "Requires a valid configuration pointer.");

    platform_console_initialize();
    platform_memory_initialize();
    platform_systimer_initialize();
    platform_thread_initialize();
    memory_system_initialize();
    // TODO: Конфигурируемый.
    platform_window_initialize(PLATFORM_WINDOW_BACKEND_TYPE_AUTO);

    context = mallocate(sizeof(application_context), MEMORY_TAG_APPLICATION);
    if(!context)
    {
        LOG_ERROR("Failed to allocate memory for application context.");
        return false;
    }
    mzero(context, sizeof(application_context));

    context->window = platform_window_create(&config->window);
    if(!context->window)
    {
        LOG_ERROR("Failed to create application window.");
        mfree(context, sizeof(application_context), MEMORY_TAG_APPLICATION);
        context = nullptr;
        return false;
    }

    // Инициализация настроек тайминга по умолчанию.
    // NOTE: Для снятия ограничений установить более 1000.0
    context->target_frame_time = 1.0 / 60.0;

    // Инициализация состояний.
    context->is_running = true;
    context->is_suspended = false;

    return true;
}

void application_shutdown()
{
    ASSERT(context != nullptr, "Application should be initialized before shutdown.");

    platform_window_destroy(context->window);
    context->window = nullptr;

    mfree(context, sizeof(application_context), MEMORY_TAG_APPLICATION);
    context = nullptr;

    // TODO: Временно.
    const char* meminfo = memory_system_usage_str();
    LOG_TRACE(meminfo);
    string_free(meminfo);

    platform_window_shutdown();
    memory_system_shutdown();
    platform_thread_shutdown();
    platform_systimer_shutdown();
    platform_memory_shutdown();
    platform_console_shutdown();
}

void application_run()
{
    ASSERT(context != nullptr, "Application should be initialized before running.");

    // Тайминги управления кадром.
    timer frame_timer, game_timer;
    timer_init(&frame_timer);
    timer_init(&game_timer);

    // Запуск таймеров.
    timer_start(&frame_timer);
    timer_start(&game_timer);

    application_frame_stats frame_stats = {0};
    f64 last_time = timer_elapsed(&game_timer);
    f64 frame_time_accumulator = 0.0;
    u64 frame_count = 0;

    // TODO: Для временного отображения статистики в консоль.
    timer_format window_ft, render_ft, frame_ft, sleep_ft, sleep_actual_ft, sleep_error_ft;

    while(context->is_running)
    {
        timer_restart(&frame_timer);

        if(!platform_window_poll_events())
        {
            LOG_ERROR("Failed to process window events.");
            break;
        }

        // Время обработки пользовательского ввода, и др. событий окна.
        frame_stats.window_time = timer_elapsed(&frame_timer);

        // Обновление игрового времени (всегда).
        f64 current_time = timer_elapsed(&game_timer);
        f64 delta_time = current_time - last_time;
        last_time = current_time;

        // Сбор статистики FPS.
        frame_count++;
        frame_time_accumulator += delta_time;

        // Обновление статистики FPS.
        if(frame_time_accumulator >= 1.0)
        {
            frame_stats.current_fps = frame_count;
            frame_time_accumulator -= 1.0;
            frame_count = 0;

            // TODO: Временное отображение статистики кадра в консоль. Убрать!
            timer_get_format(context->frame_stats.window_time, &window_ft);
            timer_get_format(context->frame_stats.render_timr, &render_ft);
            timer_get_format(context->frame_stats.frame_time, &frame_ft);
            timer_get_format(context->frame_stats.sleep_time, &sleep_ft);
            timer_get_format(context->frame_stats.sleep_actual_time, &sleep_actual_ft);
            timer_get_format(context->frame_stats.sleep_error_time, &sleep_error_ft);

            LOG_DEBUG(
                "Frame stats:\n\nWindow time %.2f%s\nRender time %.2f%s\nFrame time %.2f%s\nSleep time %.2f%s\nReally sleep time %.2f%s\nError sleep time %.2f%s\nCurrent FPS %llu\n",
                window_ft.amount, window_ft.unit, render_ft.amount, render_ft.unit, frame_ft.amount, frame_ft.unit, sleep_ft.amount,
                sleep_ft.unit, sleep_actual_ft.amount, sleep_actual_ft.unit, sleep_error_ft.amount, sleep_error_ft.unit, context->frame_stats.current_fps
            );
        }

        if(!context->is_suspended)
        {
            // TODO: Обновление игровой логики.
            // TODO: Отрисовка кадра (рендерер).

            // TODO: Добавить обновление игровой логики, физики. А пока что общее время.
            frame_stats.render_timr = timer_elapsed(&frame_timer) - frame_stats.window_time;
        }

        // Время кадра и расчет времени сна с поправкой на ошибку.
        frame_stats.frame_time = timer_elapsed(&frame_timer);
        frame_stats.sleep_time = context->target_frame_time - frame_stats.frame_time - frame_stats.sleep_error_time;
        frame_stats.sleep_error_time = 0.0;

        if(frame_stats.sleep_time >= 0.001)
        {
            // Возвращение управления операционной система, время сна.
            platform_thread_sleep((u32)(frame_stats.sleep_time * 1000.0));

            // Вычисление ошибки c ограничением в разумных пределах.
            frame_stats.sleep_actual_time = timer_elapsed(&frame_timer) - frame_stats.frame_time;
            frame_stats.sleep_error_time = frame_stats.sleep_actual_time - frame_stats.sleep_time;
            frame_stats.sleep_error_time = CLAMP(
                frame_stats.sleep_error_time, -context->target_frame_time * 0.5, context->target_frame_time * 0.5
            );
        }

        // Для корректного отображения статистики кадра.
        context->frame_stats = frame_stats;

        // TODO: Обновление состояния игрового ввода только после кадра.
        // input_system_update() 
    }
}
