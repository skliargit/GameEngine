#include "platform/systimer.h"

#ifdef PLATFORM_WINDOWS_FLAG

    #include "core/logger.h"
    #include "debug/assert.h"
    #include <windows.h>

    static LARGE_INTEGER start = {0};
    static LARGE_INTEGER freq  = {0};
    static bool initialized    = false;

    bool platform_systimer_initialize()
    {
        if(initialized)
        {
            return true;
        }

        if(!QueryPerformanceFrequency(&freq))
        {
            LOG_FATAL("Windows high-resolution timer (QPC) is not available.");
            return false;
        }

        if(!QueryPerformanceCounter(&start))
        {
            LOG_FATAL("Windows high-resolution timer (QPC) failed to initialize.");
            return false;
        }

        LOG_TRACE("Windows high-resolution timer (QPC) initialized with frequency: %llu Hz.", freq.QuadPart);
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
        start.QuadPart = 0;
        freq.QuadPart = 0;
        initialized = false;
    }

    bool platform_systimer_is_initialized()
    {
        return initialized;
    }

    f64 platform_systimer_now()
    {
        ASSERT(initialized == true, "Platform subsystem timer not initialized. Call platform_systimer_initialize() first.");

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (f64)(now.QuadPart - start.QuadPart) / (f64)freq.QuadPart;
    }

#endif
