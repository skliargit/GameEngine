#include "platform/memory.h"

#ifdef PLATFORM_LINUX_FLAG

    #include "debug/assert.h"
    #include <stdlib.h>
    #include <string.h>

    static bool initialized = false;

    bool platform_memory_initialize()
    {
        ASSERT(initialized == false, "Memory subsystem is already initialized.");

        initialized = true;
        return true;
    }

    void platform_memory_shutdown()
    {
        initialized = false;
    }

    bool platform_memory_is_initialized()
    {
        return initialized;
    }

    void* platform_memory_allocate(u64 size)
    {
        ASSERT(initialized == true, "Memory subsystem not initialized. Call platform_memory_initialize() first.");
        ASSERT(size > 0, "Size must be greater than zero.");

        return malloc((size_t)size);
    }

    void platform_memory_free(void* block)
    {
        ASSERT(initialized == true, "Memory subsystem not initialized. Call platform_memory_initialize() first.");
        ASSERT(block != nullptr, "Block pointer must be non-null.");

        free(block);
    }

    void platform_memory_zero(void* block, u64 size)
    {
        ASSERT(initialized == true, "Memory subsystem not initialized. Call platform_memory_initialize() first.");
        ASSERT(block != nullptr, "Block pointer must be non-null.");
        ASSERT(size > 0, "Size must be greater than zero.");

        memset(block, 0, (size_t)size);
    }

    void platform_memory_set(void* block, u64 size, u8 value)
    {
        ASSERT(initialized == true, "Memory subsystem not initialized. Call platform_memory_initialize() first.");
        ASSERT(block != nullptr, "Block pointer must be non-null.");
        ASSERT(size > 0, "Size must be greater than zero.");

        memset(block, (int)value, (size_t)size);
    }

    void platform_memory_copy(void* dst, const void* src, u64 size)
    {
        ASSERT(initialized == true, "Memory subsystem not initialized. Call platform_memory_initialize() first.");
        ASSERT(dst != nullptr, "Destination buffer pointer must be non-null.");
        ASSERT(src != nullptr, "Source buffer pointer must be non-null.");
        ASSERT(size > 0, "Size must be greater than zero.");

        memcpy(dst, src, (size_t)size);
    }

    void platform_memory_move(void* dst, const void* src, u64 size)
    {
        ASSERT(initialized == true, "Memory subsystem not initialized. Call platform_memory_initialize() first.");
        ASSERT(dst != nullptr, "Destination buffer pointer must be non-null.");
        ASSERT(src != nullptr, "Source buffer pointer must be non-null.");
        ASSERT(size > 0, "Size must be greater than zero.");

        memmove(dst, src, (size_t)size);
    }

#endif
