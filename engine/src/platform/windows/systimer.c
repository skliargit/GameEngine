#include "platform/systimer.h"

#ifdef PLATFORM_WINDOWS_FLAG

    #include "debug/assert.h"
    #include "core/logger.h"
    #include <windows.h>

    static LARGE_INTEGER start = {0};
    static LARGE_INTEGER freq  = {0};
    static bool initialized    = false;

    bool platform_systimer_initialize()
    {
        ASSERT(initialized == false, "Timer subsystem is already initialized.");

        if(!QueryPerformanceFrequency(&freq))
        {
            LOG_ERROR("Windows high-resolution timer (QPC) is not available.");
            return false;
        }

        if(!QueryPerformanceCounter(&start))
        {
            LOG_ERROR("Windows high-resolution timer (QPC) failed to initialize.");
            return false;
        }

        LOG_TRACE("Windows high-resolution timer (QPC) initialized with frequency: %llu Hz.", freq.QuadPart);

        initialized = true;
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
        ASSERT(initialized == true, "Timer subsystem not initialized. Call platform_systimer_initialize() first.");

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (f64)(now.QuadPart - start.QuadPart) / (f64)freq.QuadPart;
    }

#endif
