#include "core/string.h"
#include "core/memory.h"

#include "debug/assert.h"
#include "platform/string.h"

char* string_duplicate(const char* str)
{
    ASSERT(str != nullptr, "Pointer to string must be non-null.");

    u64 length = platform_string_length(str) + 1;
    void* newstr = mallocate(length, MEMORY_TAG_STRING);
    mcopy(newstr, str, length);

    return newstr;
}

void string_free(const char* str)
{
    ASSERT(str != nullptr, "Pointer to string must be non-null.");

    u64 length = platform_string_length(str) + 1;
    mfree(str, length, MEMORY_TAG_STRING);
}
