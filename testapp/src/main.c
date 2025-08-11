#include <platform/console.h>
#include <platform/systimer.h>
#include <platform/time.h>
#include <platform/window.h>
#include <core/logger.h>
#include <debug/assert.h>

// TODO: Временно.
#include <stdio.h>

int main()
{

    log_set_level(LOG_LEVEL_TRACE);

    // Вывод тестовых сообщений для всех цветов
    platform_console_write(CONSOLE_COLOR_DEFAULT, "Test message 0 (Default)\n");
    platform_console_write(CONSOLE_COLOR_RED,     "Test message 1 (Light Red) (Error)\n");
    platform_console_write(CONSOLE_COLOR_ORANGE,  "Test message 2 (Light Orange)\n");
    platform_console_write(CONSOLE_COLOR_GREEN,   "Test message 3 (Light Green) (Information)\n");
    platform_console_write(CONSOLE_COLOR_YELLOW,  "Test message 4 (Light Yellow) (Warning)\n");
    platform_console_write(CONSOLE_COLOR_BLUE,    "Test message 5 (Light Blue) (Debug)\n");
    platform_console_write(CONSOLE_COLOR_MAGENTA, "Test message 6 (Light Magenta) (Fatal/Assertion)\n");
    platform_console_write(CONSOLE_COLOR_CYAN,    "Test message 7 (Light Cyan)\n");
    platform_console_write(CONSOLE_COLOR_WHITE,   "Test message 8 (White) (Trace)\n");
    platform_console_write(CONSOLE_COLOR_GRAY,    "Test message 9 (Gray)\n");

    // --------------------------- Test 1 ---------------------------

    platform_console_write(CONSOLE_COLOR_ORANGE, "\nFirst call system time now:\n");
    f64 start = platform_systimer_now();
    u64 time_now = platform_time_now();
    f64 end = platform_systimer_now();
    printf("System time sec: %llu (%lf us)\n", time_now, (end - start) * 1000000.0);

    platform_console_write(CONSOLE_COLOR_ORANGE, "Next call system time now:\n");
    start = platform_systimer_now();
    time_now = platform_time_now();
    end = platform_systimer_now();
    printf("System time sec: %llu (%lf us)\n", time_now, (end - start) * 1000000.0);

    // --------------------------- Test 2 ---------------------------

    platform_console_write(CONSOLE_COLOR_ORANGE, "\nFirst call system time to datetime:\n");

    start = platform_systimer_now();
    datetime time_local = platform_time_to_local(time_now);
    end = platform_systimer_now();
    printf(
        "Local time: %hhu/%hhu/%hu %hhu:%hhu:%hhu (%lf us)\n",
        time_local.day, time_local.month, time_local.year, time_local.hour, time_local.minute, time_local.second,
        (end - start) * 1000000.0
    );

    start = platform_systimer_now();
    datetime time_utc = platform_time_to_utc(time_now);
    end = platform_systimer_now();
    printf(
        "UTC time: %hhu/%hhu/%hu %hhu:%hhu:%hhu (%lf us)\n",
        time_utc.day, time_utc.month, time_utc.year, time_utc.hour, time_utc.minute, time_utc.second,
        (end - start) * 1000000.0
    );

    platform_console_write(CONSOLE_COLOR_ORANGE, "Next call system time to datetime:\n");

    start = platform_systimer_now();
    time_local = platform_time_to_local(time_now);
    end = platform_systimer_now();
    printf(
        "Local time: %hhu/%hhu/%hu %hhu:%hhu:%hhu (%lf us)\n",
        time_local.day, time_local.month, time_local.year, time_local.hour, time_local.minute, time_local.second,
        (end - start) * 1000000.0
    );

    start = platform_systimer_now();
    time_utc = platform_time_to_utc(time_now);
    end = platform_systimer_now();
    printf(
        "UTC time: %hhu/%hhu/%hu %hhu:%hhu:%hhu (%lf us)\n",
        time_utc.day, time_utc.month, time_utc.year, time_utc.hour, time_utc.minute, time_utc.second,
        (end - start) * 1000000.0
    );

    // --------------------------- Test 3 ---------------------------

    platform_console_write(CONSOLE_COLOR_ORANGE, "\nFirst call system time from datetime:\n");

    start = platform_systimer_now();
    u64 localstamp = platform_time_from_datetime(&time_local);
    end = platform_systimer_now();
    printf("Datetime(local) to localstamp: %llu (%lf us)\n", localstamp, (end - start) * 1000000.0);

    start = platform_systimer_now();
    u64 utcstamp = platform_time_from_datetime(&time_utc);
    end = platform_systimer_now();
    printf("Datetime(utc) to utcstamp: %llu (%lf us)\n", utcstamp, (end - start) * 1000000.0);

    platform_console_write(CONSOLE_COLOR_ORANGE, "Next call system time from datetime:\n");

    start = platform_systimer_now();
    localstamp = platform_time_from_datetime(&time_local);
    end = platform_systimer_now();
    printf("Datetime(local) to localstamp: %llu (%lf us)\n", localstamp, (end - start) * 1000000.0);

    start = platform_systimer_now();
    utcstamp = platform_time_from_datetime(&time_utc);
    end = platform_systimer_now();
    printf("Datetime(utc) to utcstamp: %llu (%lf us)\n", utcstamp, (end - start) * 1000000.0);

    // --------------------------- Test 4 ---------------------------

    platform_console_write(CONSOLE_COLOR_ORANGE, "\nFirst call systimer now:\n");
    start = platform_systimer_now();
    f64 timer = platform_systimer_now();
    end = platform_systimer_now();
    printf("Systimer sec: %lf (%lf us)\n", timer, (end - start) * 1000000.0);

    platform_console_write(CONSOLE_COLOR_ORANGE, "Next call systimer now:\n");
    start = platform_systimer_now();
    timer = platform_systimer_now();
    end = platform_systimer_now();
    printf("Systimer sec: %lf (%lf us)\n", timer, (end - start) * 1000000.0);

    // --------------------------- Test 5 ---------------------------

    platform_console_write(CONSOLE_COLOR_DEFAULT, "\n");

    start = platform_systimer_now();
    LOG_FATAL("This is FATAL message");
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_FATAL("This is FATAL message");
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_FATAL("This is FATAL message with number %lf", start);
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_FATAL("This is FATAL message with number %lf", start);
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    // --------------------------- Test 6 ---------------------------

    platform_console_write(CONSOLE_COLOR_DEFAULT, "\n");

    start = platform_systimer_now();
    LOG_ERROR("This is ERROR message");
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_ERROR("This is ERROR message");
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_ERROR("This is ERROR message with number %lf", start);
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_ERROR("This is ERROR message with number %lf", start);
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    // --------------------------- Test 7 ---------------------------

    platform_console_write(CONSOLE_COLOR_DEFAULT, "\n");

    start = platform_systimer_now();
    LOG_WARN("This is WARNG message");
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_WARN("This is WARNG message");
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_WARN("This is WARNG message with number %lf", start);
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_WARN("This is WARNG message with number %lf", start);
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);
    
    // --------------------------- Test 8 ---------------------------

    platform_console_write(CONSOLE_COLOR_DEFAULT, "\n");

    start = platform_systimer_now();
    LOG_INFO("This is INFO message");
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_INFO("This is INFO message");
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_INFO("This is INFO message with number %lf", start);
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_INFO("This is INFO message with number %lf", start);
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    // --------------------------- Test 8 ---------------------------

    platform_console_write(CONSOLE_COLOR_DEFAULT, "\n");

    start = platform_systimer_now();
    LOG_DEBUG("This is DEBUG message");
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_DEBUG("This is DEBUG message");
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_DEBUG("This is DEBUG message with number %lf", start);
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_DEBUG("This is DEBUG message with number %lf", start);
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    // --------------------------- Test 9 ---------------------------

    platform_console_write(CONSOLE_COLOR_DEFAULT, "\n");

    start = platform_systimer_now();
    LOG_TRACE("This is TRACE message");
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_TRACE("This is TRACE message");
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_TRACE("This is TRACE message with number %lf", start);
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    start = platform_systimer_now();
    LOG_TRACE("This is TRACE message with number %lf", start);
    end = platform_systimer_now();
    printf("Time call %lf us\n", (end - start) * 1000000.0);

    // ------------------------- Test Window -------------------------

    platform_console_write(CONSOLE_COLOR_DEFAULT, "\n");

    platform_window* window_inst = nullptr;
    platform_window_config window_conf;
    window_conf.backend_type = PLATFORM_WINDOW_BACKEND_TYPE_AUTO;
    window_conf.title = "Simple window";
    window_conf.width = 800;
    window_conf.height = 600;

    start = platform_systimer_now();
    platform_window_create(&window_conf, &window_inst);
    end = platform_systimer_now();
    LOG_INFO("Window successfully created (time call: %lf us).", (end - start) * 1000000.0);

    while(platform_window_poll_events(window_inst)); //{ LOG_DEBUG("MESSAGE"); };

    start = platform_systimer_now();
    platform_window_destroy(window_inst);
    end = platform_systimer_now();
    LOG_INFO("Window successfully destroyed (time call: %lf us).", (end - start) * 1000000.0);

    // --------------------------------------------------------------

    platform_console_write(CONSOLE_COLOR_DEFAULT, "\n");
    // NOTE: Специально вызвал ошибку, что бы протестировать.
    // ASSERT(1!=1, "1 must be qeual to 1");

    return 0;
}
