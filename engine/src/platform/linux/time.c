#include "platform/time.h"

#if PLATFORM_LINUX_FLAG

    #include <time.h>

    // Для потокобезопасных версий.
    #define _POSIX_C_SOURCE 200809L

    u64 platform_time_now()
    {
        return (u64)time(nullptr);
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
        localtime_r(&t, &local);

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
        gmtime_r(&t, &utc);

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
