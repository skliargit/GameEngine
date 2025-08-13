#include "platform/string.h"

#if PLATFORM_WINDOWS_FLAG

    #include "debug/assert.h"

    #include <stdio.h>
    #include <stdarg.h>
    #include <string.h>

    u64 platform_string_length(const char* str)
    {
        ASSERT(str != nullptr, "String cannot be null.");
        return (u64)strlen(str);
    }

    i32 platform_string_format_length(const char* format, void* args)
    {
        ASSERT(format != nullptr, "Format string cannot be null.");
        ASSERT(args != nullptr, "Variadic arguments cannot be null.");

        va_list copy = args;
        // NOTE: Параметры args могут использоваться только для одного вызова vsnprintf, для повторного использования
        //       необходима копия va_list или последовательность va_end -> va_start -> vsnprintf -> va_end.
        i32 result = vsnprintf(nullptr, 0, format, copy);

        return result;
    }

    i32 platform_string_format_va(char* dst, u64 length, const char* format, void* args)
    {
        ASSERT(dst != nullptr, "Destination buffer cannot be null.");
        ASSERT(format != nullptr, "Format string cannot be null.");
        ASSERT(args != nullptr, "Variadic arguments cannot be null.");

        return vsnprintf(dst, length, format, args);
    }

    i32 platform_string_format(char* dst, u64 length, const char* format, ...)
    {
        ASSERT(dst != nullptr, "Destination buffer cannot be null.");
        ASSERT(format != nullptr, "Format string cannot be null.");

        va_list args;
        va_start(args, format);
        i32 result = vsnprintf(dst, length, format, args);
        va_end(args);

        return result;
    }

    bool platform_string_equal(const char* lstr, const char* rstr)
    {
        ASSERT(lstr != nullptr, "Left string (lstr) cannot be null.");
        ASSERT(rstr != nullptr, "Right string (rstr) cannot be null.");
        return (lstr == rstr) || strcmp(lstr, rstr) == 0;
    }

    bool platform_string_equali(const char* lstr, const char* rstr)
    {
        ASSERT(lstr != nullptr, "Left string (lstr) cannot be null.");
        ASSERT(rstr != nullptr, "Right string (rstr) cannot be null.");
        return (lstr == rstr) || _stricmp(lstr, rstr) == 0;
    }

    bool platform_string_nequal(const char* lstr, const char* rstr, u64 length)
    {
        ASSERT(lstr != nullptr, "Left string (lstr) cannot be null.");
        ASSERT(rstr != nullptr, "Right string (rstr) cannot be null.");
        return (lstr == rstr) || strncmp(lstr, rstr, length) == 0;
    }

    bool platform_string_nequali(const char* lstr, const char* rstr, u64 length)
    {
        ASSERT(lstr != nullptr, "Left string (lstr) cannot be null.");
        ASSERT(rstr != nullptr, "Right string (rstr) cannot be null.");
        return (lstr == rstr) || _strnicmp(lstr, rstr, length) == 0;
    }

    char* platform_string_copy(char* dst, const char* src)
    {
        ASSERT(dst != nullptr, "Destination buffer cannot be null.");
        ASSERT(src != nullptr, "Source string cannot be null.");
        strcpy_s(dst, strlen(src) + 1, src);
        return dst;
    }

    char* platform_string_ncopy(char* dst, const char* src, u64 length)
    {
        ASSERT(dst != nullptr, "Destination buffer cannot be null.");
        ASSERT(src != nullptr, "Source string cannot be null.");
        ASSERT(length != 0, "Length cannot be zero.");

        u64 copy_length = (u64)strnlen(src, length - 1);
        memcpy(dst, src, copy_length);
        dst[copy_length] = '\0';

        return dst;
    }

#endif
