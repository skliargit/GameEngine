#include "core/timer.h"
#include "debug/assert.h"
#include "platform/systimer.h"

// TODO: Обернуть.
#include <math.h>

void timer_init(timer* t)
{
    ASSERT(t != nullptr, "Timer pointer must be non-zero.");
    t->start = 0.0;
    t->end = 0.0;
}

void timer_start(timer* t)
{
    ASSERT(t != nullptr, "Timer pointer must be non-zero.");
    t->start = platform_systimer_now();
    t->end = 0.0;
}

void timer_stop(timer* t)
{
    ASSERT(t != nullptr, "Timer pointer must be non-zero.");
    if(t->start > 0.0 && t->end == 0.0)
    {
        t->end = platform_systimer_now();
    }
}

f64 timer_elapsed(const timer* t)
{
    ASSERT(t != nullptr, "Timer pointer must be non-zero.");
    if(t->start > 0.0 && t->end == 0.0)
    {
        return platform_systimer_now() - t->start;
    }
    return 0.0;
}

f64 timer_duration(const timer* t)
{
    ASSERT(t != nullptr, "Timer pointer must be non-zero.");
    if(t->start > 0.0 && t->end > 0.0)
    {
        return t->end - t->start;
    }
    return 0.0;
}

bool timer_is_running(const timer* t)
{
    ASSERT(t != nullptr, "Timer pointer must be non-zero.");
    return t->start > 0.0 && t->end == 0.0;
}

void timer_get_format(f64 time_sec, timer_format* out_format)
{
    f64 abs_time = fabs(time_sec);

    if(abs_time < 1e-6)
    {
        out_format->unit = "ns";
        out_format->amount = (f32)(time_sec * 1e9);
    }
    else if (abs_time < 1e-3)
    {
        out_format->unit = "us";
        out_format->amount = (f32)(time_sec * 1e6);
    }
    else if (abs_time < 1.0)
    {
        out_format->unit = "ms";
        out_format->amount = (f32)(time_sec * 1e3);
    }
    else
    {
        out_format->unit = "s";
        out_format->amount = (f32)time_sec;
    }
}
