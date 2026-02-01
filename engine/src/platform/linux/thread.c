#include "platform/thread.h"

#ifdef PLATFORM_LINUX_FLAG

    #include "debug/assert.h"

    #include <time.h>
    #include <asm-generic/errno-base.h>

    static bool initialized = false;

    bool platform_thread_initialize()
    {
        ASSERT(initialized == false, "Thread subsystem is already initialized.");

        initialized = true;
        return true;
    }

    void platform_thread_shutdown()
    {
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

        struct timespec req;
        req.tv_sec = time_ms / 1000ULL;
        req.tv_nsec = (time_ms % 1000ULL) * 1000000ULL;
        i32 result = clock_nanosleep(CLOCK_MONOTONIC, 0, &req, nullptr);

        return (result == 0 || result == EINTR);
    }

#endif
