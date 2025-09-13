#include "platform/thread.h"

#ifdef PLATFORM_WINDOWS_FLAG

    #include "debug/assert.h"
    #include "core/logger.h"
    #include <Windows.h>

    static u32 timer_resolution = 0;
    static bool initialized = false;

    bool platform_thread_initialize()
    {
        ASSERT(initialized == false, "Thread subsystem is already initialized.");

        // NOTE: Если вдруг завершиться неудачей, просто будет ограничение по кадрам.
        TIMECAPS tc;
        if(timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR)
        {
            // Установка минимального допустимого разрешение таймера (1 мс).
            timer_resolution = 1;
            timer_resolution = CLAMP(timer_resolution, tc.wPeriodMin, tc.wPeriodMax);

            if(timeBeginPeriod(timer_resolution) == TIMERR_NOERROR)
            {
                LOG_TRACE("Interval interrupt resolution is set to %u ms.", timer_resolution);
            }
            else
            {
                LOG_WARN("Failed to set interval interrupt resolution. Sleep accuracy may be reduced.");
                timer_resolution = 0;
            }
        }
        else
        {
            LOG_WARN("Failed to obtain interval interrupt capability. Sleep accuracy may be reduced.");
        }

        initialized = true;
        return true;
    }

    void platform_thread_shutdown()
    {
        if(timer_resolution > 0)
        {
            timeEndPeriod(timer_resolution);
            LOG_TRACE("Restored default interval interrupt resolution.");
        }

        timer_resolution = 0;
        initialized = false;
    }

    bool platform_thread_is_initialized()
    {
        return initialized;
    }

    bool platform_thread_sleep(u32 time_ms)
    {
        ASSERT(initialized == true, "Thread subsystem not initialized. Call platform_thread_initialize() first.");
        ASSERT(time_ms > 0, "Sleep time must be greater than 0.");

        Sleep(time_ms);
        return true;  
    }

#endif
