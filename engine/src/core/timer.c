#include "core/timer.h"

#include "debug/assert.h"
#include "platform/time.h"
#include "platform/math.h"

void timer_init(timer* t)
{
    ASSERT(t != nullptr, "Timer pointer must be non-null.");

    t->start = 0.0;
    t->last = 0.0;
}

void timer_start(timer* t)
{
    ASSERT(t != nullptr, "Timer pointer must be non-null.");

    const f64 now = platform_time_uptime();
    t->start = now;
    t->last = now;
}

void timer_stop(timer* t)
{
    ASSERT(t != nullptr, "Timer pointer must be non-null.");

    if(t->start > 0.0)
    {
        t->last = 0.0; // platform_time_uptime() - t->start;
        t->start = 0.0;
    }
}

f64 timer_elapsed(const timer* t)
{
    ASSERT(t != nullptr, "Timer pointer must be non-null.");

    if(t->start > 0.0)
    {
        return platform_time_uptime() - t->start;
    }

    return 0.0;
}

f64 timer_delta(timer* t)
{
    ASSERT(t != nullptr, "Timer pointer must be non-null.");

    f64 delta = 0.0;
    if(t->start > 0.0)
    {
        f64 now = platform_time_uptime();
        delta = now - t->last;
        t->last = now;
    }

    return delta;
}

bool timer_is_running(const timer* t)
{
    ASSERT(t != nullptr, "Timer pointer must be non-null.");

    return t->start > 0.0;
}

void timer_get_format(f64 time_sec, timer_format* out_format)
{
    ASSERT(out_format != nullptr, "Pointer must be non-null.");

    const f64 NS_LIMIT = 0.9e-6;   // 900 ns
    const f64 US_LIMIT = 0.9e-3;   // 900 us  
    const f64 MS_LIMIT = 0.9;      // 900 ms
    const f64 time_sec_abs = platform_math_abs(time_sec);

    if(time_sec_abs < NS_LIMIT)
    {
        out_format->unit = "ns";
        out_format->amount = CAST_F32(time_sec * 1e9);
    }
    else if(time_sec_abs < US_LIMIT)
    {
        out_format->unit = "us";
        out_format->amount = CAST_F32(time_sec * 1e6);
    }
    else if(time_sec_abs < MS_LIMIT)
    {
        out_format->unit = "ms";
        out_format->amount = CAST_F32(time_sec * 1e3);
    }
    else
    {
        out_format->unit = "s";
        out_format->amount = CAST_F32(time_sec);
    }
}
