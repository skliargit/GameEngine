#include "platform/string.h"

#if PLATFORM_LINUX_FLAG

    #include <stdio.h>
    #include <stdarg.h>
    #include <string.h>

    u64 platform_string_length(const char* str)
    {
        if(!str) return 0;
        return (u64)strlen(str);
    }

    i32 platform_string_format_length(const char* format, void* args)
    {
        // TODO: Заменить на assert.
        if(!format || !args) return -1;
        va_list copy;
        va_copy(copy, args);

        // NOTE: Параметры args могут использоваться только для одного вызова vsnprintf, для повторного использования
        //       необходима копия va_list или последовательность va_end -> va_start -> vsnprintf -> va_end.
        i32 result = vsnprintf(nullptr, 0, format, copy);

        va_end(copy);
        return result;
    }

    i32 platform_string_format_va(char* dst, u64 length, const char* format, void* args)
    {
        // TODO: Заменить на assert.
        if(!dst || !length || !format || !args) return -1;
        return vsnprintf(dst, length, format, args);
    }

    i32 platform_string_format(char* dst, u64 length, const char* format, ...)
    {
        // TODO: Заменить на assert.
        if(!dst || !length || !format) return -1;
        va_list args;
        va_start(args, format);

        i32 result = vsnprintf(dst, length, format, args);

        va_end(args);
        return result;
    }

#endif
