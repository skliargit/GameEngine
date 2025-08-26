#include "platform/string.h"

#ifdef PLATFORM_WINDOWS_FLAG

    #include "debug/assert.h"
    #include <stdio.h>
    #include <stdarg.h>
    #include <string.h>

    u64 platform_string_length(const char* str)
    {
        ASSERT(str != nullptr, "String pointer must be non-null.");

        return (u64)strlen(str);
    }

    i32 platform_string_format_va(char* dst, u64 length, const char* format, void* args)
    {
        ASSERT(format != nullptr, "Format string pointer must be non-null.");
        ASSERT(args != nullptr, "Variadic arguments pointer must be non-null.");

        va_list copy = args;
        // NOTE: Параметры args могут использоваться только для одного вызова vsnprintf, для повторного использования
        //       необходима копия va_list или последовательность va_end -> va_start -> vsnprintf -> va_end.
        i32 result = vsnprintf(dst, length, format, copy);

        return result;
    }

    i32 platform_string_format(char* dst, u64 length, const char* format, ...)
    {
        ASSERT(format != nullptr, "Format string pointer must be non-null.");

        va_list args;
        va_start(args, format);
        i32 result = vsnprintf(dst, length, format, args);
        va_end(args);

        return result;
    }

    bool platform_string_equal(const char* lstr, const char* rstr)
    {
        ASSERT(lstr != nullptr, "Left string pointer must be non-null.");
        ASSERT(rstr != nullptr, "Right string pointer must be non-null.");

        return (lstr == rstr) || strcmp(lstr, rstr) == 0;
    }

    bool platform_string_equali(const char* lstr, const char* rstr)
    {
        ASSERT(lstr != nullptr, "Left string pointer must be non-null.");
        ASSERT(rstr != nullptr, "Right string pointer must be non-null.");

        return (lstr == rstr) || _stricmp(lstr, rstr) == 0;
    }

    bool platform_string_nequal(const char* lstr, const char* rstr, u64 length)
    {
        ASSERT(lstr != nullptr, "Left string pointer must be non-null.");
        ASSERT(rstr != nullptr, "Right string pointer must be non-null.");

        return (lstr == rstr) || strncmp(lstr, rstr, length) == 0;
    }

    bool platform_string_nequali(const char* lstr, const char* rstr, u64 length)
    {
        ASSERT(lstr != nullptr, "Left string pointer must be non-null.");
        ASSERT(rstr != nullptr, "Right string pointer must be non-null.");

        return (lstr == rstr) || _strnicmp(lstr, rstr, length) == 0;
    }

    char* platform_string_copy(char* dst, const char* src)
    {
        ASSERT(dst != nullptr, "Destination buffer pointer must be non-null.");
        ASSERT(src != nullptr, "Source string pointer must be non-null.");

        strcpy_s(dst, strlen(src) + 1, src);
        return dst;
    }

    char* platform_string_ncopy(char* dst, const char* src, u64 length)
    {
        ASSERT(dst != nullptr, "Destination buffer pointer must be non-null.");
        ASSERT(src != nullptr, "Source string pointer must be non-null.");
        ASSERT(length != 0, "Length must be greater than zero.");

        u64 copy_length = (u64)strnlen(src, length - 1);
        memcpy(dst, src, copy_length);
        dst[copy_length] = '\0';

        return dst;
    }

#endif
