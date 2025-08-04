#include "platform/systimer.h"

#if PLATFORM_WINDOWS_FLAG

    #include <windows.h>

    f64 platform_systimer_now()
    {
        static LARGE_INTEGER start = {0};
        static LARGE_INTEGER freq = {0};

        // TODO: Потокобезопасным.
        if(!freq.QuadPart)
        {
            QueryPerformanceFrequency(&freq);
            QueryPerformanceCounter(&start);
        }

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (f64)(now.QuadPart - start.QuadPart) / (f64)freq.QuadPart;
    }

#endif
