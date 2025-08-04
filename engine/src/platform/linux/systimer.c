#include "platform/systimer.h"

#if PLATFORM_LINUX_FLAG

    #include <time.h>

    f64 platform_systimer_now()
    {
        struct timespec now;
        // NOTE: CLOCK_MONOTONIC_RAW игнорирует любые корректировки NTP и adjtime(), отражая "сырое" аппаратное время.
        //       Это делает его независимым от внешних влияний на системные часы.
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        return (f64)now.tv_sec + (f64)now.tv_nsec * 0.000000001;
    }

#endif
