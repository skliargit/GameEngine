#include "platform/time.h"

#if PLATFORM_WINDOWS_FLAG

    #include <windows.h>
    #include <time.h>

    // Для преобразования к unix времени.
    #define SECONDS_FROM_1601_TO_1970 11644473600ULL

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

    datetime platform_time_to_local(u64 time_sec)
    {
        static u64 time_last = 0;
        static datetime dt = { 1970, 01, 01, 00, 00, 00 };

        // TODO: Потокобезопасным.
        // Возвращение кэшированного значения.
        if(time_last >= time_sec)
        {
            return dt;
        }

        time_t t = (time_t)time_sec;
        struct tm local;
        // NOTE: При первом вызове выполняется медленно, т.к. читает из файла TZ,
        //       после чего кеширует его, и последующие вызовы работают быстрее.
        localtime_s(&local, &t);

        dt.year   = (u16)(local.tm_year + 1900);
        dt.month  = (u8)(local.tm_mon + 1);
        dt.day    = (u8)(local.tm_mday);
        dt.hour   = (u8)(local.tm_hour);
        dt.minute = (u8)(local.tm_min);
        dt.second = (u8)(local.tm_sec);

        time_last = time_sec;
        return dt;
    }

    datetime platform_time_to_utc(u64 time_sec)
    {
        static u64 time_last = 0;
        static datetime dt = { 1970, 01, 01, 00, 00, 00 };

        // TODO: Потокобезопасным.
        // Возвращение кэшированного значения.
        if(time_last >= time_sec)
        {
            return dt;
        }

        time_t t = (time_t)time_sec;
        struct tm utc;
        gmtime_s(&utc, &t);

        dt.year   = (u16)(utc.tm_year + 1900);
        dt.month  = (u8)(utc.tm_mon + 1);
        dt.day    = (u8)(utc.tm_mday);
        dt.hour   = (u8)(utc.tm_hour);
        dt.minute = (u8)(utc.tm_min);
        dt.second = (u8)(utc.tm_sec);

        time_last = time_sec;
        return dt;
    }

    u64 platform_time_from_datetime(const datetime* dt)
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
