#include "platform/time.h"

#ifdef PLATFORM_LINUX_FLAG

    // Для потокобезопасных версий.
    #define _POSIX_C_SOURCE 200809L

    #include "debug/assert.h"
    #include <time.h>

    static bool initialized = false;

    bool platform_time_initialize()
    {
        ASSERT(initialized == false, "Time subsystem is already initialized.");

        initialized = true;
        return true;
    }

    void platform_time_shutdown()
    {
        initialized = false;
    }

    bool platform_time_is_initialized()
    {
        return initialized;
    }

    u64 platform_time_now()
    {
        return (u64)time(nullptr);
    }

    u64 platform_time_seed(void)
    {
        ASSERT(initialized == true, "Time subsystem not initialized. Call platform_time_initialize() first.");

        // TODO: Добавить энтропии на основе PID процесса и адреса стека.
        // seed ^= (u64)getpid() << 32
        // seed ^= (u64)((usize)&stack);
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
    }

    f64 platform_time_uptime()
    {
        ASSERT(initialized == true, "Time subsystem not initialized. Call platform_time_initialize() first.");

        struct timespec now;
        // NOTE: CLOCK_MONOTONIC_RAW игнорирует любые корректировки NTP и adjtime(), отражая "сырое" аппаратное время.
        //       Это делает его независимым от внешних влияний на системные часы.
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        return (f64)now.tv_sec + (f64)now.tv_nsec * 0.000000001;
    }

    platform_datetime platform_time_to_local(u64 time_sec)
    {
        time_t t = (time_t)time_sec;
        struct tm local;
        // NOTE: При первом вызове выполняется медленно, т.к. читает из файла TZ,
        //       после чего кеширует его, и последующие вызовы работают быстрее.
        localtime_r(&t, &local);

        platform_datetime dt;
        dt.year   = (u16)(local.tm_year + 1900);
        dt.month  = (u8)(local.tm_mon + 1);
        dt.day    = (u8)(local.tm_mday);
        dt.hour   = (u8)(local.tm_hour);
        dt.minute = (u8)(local.tm_min);
        dt.second = (u8)(local.tm_sec);

        return dt;
    }

    platform_datetime platform_time_to_utc(u64 time_sec)
    {
        time_t t = (time_t)time_sec;
        struct tm utc;
        gmtime_r(&t, &utc);

        platform_datetime dt;
        dt.year   = (u16)(utc.tm_year + 1900);
        dt.month  = (u8)(utc.tm_mon + 1);
        dt.day    = (u8)(utc.tm_mday);
        dt.hour   = (u8)(utc.tm_hour);
        dt.minute = (u8)(utc.tm_min);
        dt.second = (u8)(utc.tm_sec);

        return dt;
    }

    u64 platform_time_from_datetime(const platform_datetime* dt)
    {
        // TODO: Ограничение даты!
        // Быстрая проверка допустимых диапазоновы.
        if(!dt || dt->year < 1970 || dt->month < 1 || dt->month > 12 || dt->day < 1 || dt->day > 31
        || dt->hour > 23 || dt->minute > 59 || dt->second > 59)
        {
            return 0;
        }

        struct tm time = {
            .tm_sec   = dt->second,
            .tm_min   = dt->minute,
            .tm_hour  = dt->hour,
            .tm_mday  = dt->day,
            .tm_mon   = dt->month - 1,    // struct tm использует 0-11.
            .tm_year  = dt->year - 1900,  // struct tm использует год с 1900.
            .tm_isdst = -1                // автоматическое определение летнего времени.
        };

        time_t t = mktime(&time);
        return t == (time_t)-1 ? 0 : (u64)t;
    }

#endif
