#include "platform/time.h"

#ifdef PLATFORM_WINDOWS_FLAG

    #include "debug/assert.h"
    #include "core/logger.h"
    #include <windows.h>
    #include <time.h>

    // Для преобразования к unix времени.
    #define SECONDS_FROM_1601_TO_1970 11644473600ULL

    // Необходимо для таймера высокого разрешения.
    static LARGE_INTEGER start = {0};
    static LARGE_INTEGER freq  = {0};
    static bool initialized    = false;

    bool platform_time_initialize()
    {
        ASSERT(initialized == false, "Time subsystem is already initialized.");

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

    void platform_time_shutdown()
    {
        start.QuadPart = 0;
        freq.QuadPart = 0;
        initialized = false;
    }

    bool platform_time_is_initialized()
    {
        return initialized;
    }

    u64 platform_time_now()
    {
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);

        ULARGE_INTEGER uli;
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;

        // Конвертация из 100-наносекундных интервалов с 1601 в секунды с 1970.
        return uli.QuadPart / 10000000ULL - SECONDS_FROM_1601_TO_1970;
    }

    u64 platform_time_seed(void)
    {
        // TODO: Добавить энтропии на основе PID процесса и адреса стека.
        // seed ^= (u64)getpid() << 32
        // seed ^= (u64)((usize)&stack);
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return counter.QuadPart;
    }

    f64 platform_time_uptime()
    {
        ASSERT(initialized == true, "Time subsystem not initialized. Call platform_time_initialize() first.");

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (f64)(now.QuadPart - start.QuadPart) / (f64)freq.QuadPart;
    }

    platform_datetime platform_time_to_local(u64 time_sec)
    {
        time_t t = (time_t)time_sec;
        struct tm local;
        // NOTE: При первом вызове выполняется медленно, т.к. читает из файла TZ,
        //       после чего кеширует его, и последующие вызовы работают быстрее.
        localtime_s(&local, &t);

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
        gmtime_s(&utc, &t);

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

        SYSTEMTIME st = {
            .wYear         = dt->year,
            .wMonth        = dt->month,
            .wDay          = dt->day,
            .wHour         = dt->hour,
            .wMinute       = dt->minute,
            .wSecond       = dt->second,
            .wMilliseconds = 0
        };

        FILETIME ft;
        if(!SystemTimeToFileTime(&st, &ft))
        {
            return 0;
        }

        ULARGE_INTEGER uli;
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;

        // Конвертация из 100-наносекундных интервалов с 1601 в секунды с 1970.
        return uli.QuadPart / 10000000ULL - SECONDS_FROM_1601_TO_1970;
    }

#endif
