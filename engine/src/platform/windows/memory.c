#include "platform/memory.h"

#ifdef PLATFORM_WINDOWS_FLAG

    #include "debug/assert.h"
    #include <windows.h>
    #include <heapapi.h>
    #include <string.h>

    static HANDLE process_heap = nullptr;
    static bool initialized = false;

    bool platform_memory_initialize()
    {
        if(initialized)
        {
            return true;
        }

        process_heap = GetProcessHeap();
        if(!process_heap)
        {
            LOG_FATAL("Windows system memory failed.");
            return false;
        }

        initialized = true;
        return true;
    }

    void platform_memory_shutdown()
    {
        process_heap = nullptr;
        initialized = false;
    }

    bool platform_memory_is_initialized()
    {
        return initialized;
    }

    void* platform_memory_allocate(u64 size)
    {
        ASSERT(initialized == true, "Platform subsystem memory not initialized. Call platform_memory_initialize() first.");
        ASSERT(size > 0, "Size must be greater than zero.");

        return HeapAlloc(process_heap, 0, size);
    }

    void platform_memory_free(void* block)
    {
        ASSERT(initialized == true, "Platform subsystem memory not initialized. Call platform_memory_initialize() first.");
        ASSERT(block != nullptr, "Block pointer must be non-null.");

        HeapFree(process_heap, 0, block);
    }

    void platform_memory_zero(void* block, u64 size)
    {
        ASSERT(initialized == true, "Platform subsystem memory not initialized. Call platform_memory_initialize() first.");
        ASSERT(block != nullptr, "Block pointer must be non-null.");
        ASSERT(size > 0, "Size must be greater than zero.");

        SecureZeroMemory(block, (size_t)size);
    }

    void platform_memory_set(void* block, u64 size, u8 value)
    {
        ASSERT(initialized == true, "Platform subsystem memory not initialized. Call platform_memory_initialize() first.");
        ASSERT(block != nullptr, "Block pointer must be non-null.");
        ASSERT(size > 0, "Size must be greater than zero.");

        memset(block, (int)value, (size_t)size);
    }

    void platform_memory_copy(void* dst, const void* src, u64 size)
    {
        ASSERT(initialized == true, "Platform subsystem memory not initialized. Call platform_memory_initialize() first.");
        ASSERT(dst != nullptr, "Destination buffer pointer must be non-null.");
        ASSERT(src != nullptr, "Source buffer pointer must be non-null.");
        ASSERT(size > 0, "Size must be greater than zero.");

        memcpy(dst, src, (size_t)size);
    }

    void platform_memory_move(void* dst, const void* src, u64 size)
    {
        ASSERT(initialized == true, "Platform subsystem memory not initialized. Call platform_memory_initialize() first.");
        ASSERT(dst != nullptr, "Destination buffer pointer must be non-null.");
        ASSERT(src != nullptr, "Source buffer pointer must be non-null.");
        ASSERT(size > 0, "Size must be greater than zero.");

        memmove(dst, src, (size_t)size);
    }

#endif
