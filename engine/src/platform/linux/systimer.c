#include "platform/systimer.h"

#ifdef PLATFORM_LINUX_FLAG

    #include "core/timer.h"
    #include "debug/assert.h"
    #include <time.h>

    static bool initialized = false;

    bool platform_systimer_initialize()
    {
        if(initialized)
        {
            return true;
        }

        initialized = true;

        volatile f64 t1 = platform_systimer_now();
        volatile f64 t2 = platform_systimer_now();
        timer_format resolution;
        timer_get_format(t2 - t1, &resolution);
        LOG_TRACE("Timer resolution: ~%f%s.", resolution.amount, resolution.unit);

        return true;
    }

    void platform_systimer_shutdown()
    {
        initialized = false;
    }

    bool platform_systimer_is_initialized()
    {
        return initialized;
    }

    f64 platform_systimer_now()
    {
        ASSERT(initialized == true, "Platform subsystem timer not initialized. Call platform_systimer_initialize() first.");

        struct timespec now;
        // NOTE: CLOCK_MONOTONIC_RAW игнорирует любые корректировки NTP и adjtime(), отражая "сырое" аппаратное время.
        //       Это делает его независимым от внешних влияний на системные часы.
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        return (f64)now.tv_sec + (f64)now.tv_nsec * 0.000000001;
    }

#endif
