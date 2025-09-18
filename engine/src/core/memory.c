#include "core/memory.h"
#include "core/logger.h"
#include "core/string.h"
#include "core/timer.h"
#include "debug/assert.h"
#include "platform/memory.h"

typedef struct memory_stats {
    // Пиковое значение использования памяти.
    u64 peak_allocated;
    // Обшее использование памяти в данный момент.
    u64 total_allocated;
    // Использование памяти по тегам в данный момент.
    u64 tagged_allocated[MEMORY_TAG_COUNT];
    // Количество выделенных блоков памяти в данный момент.
    u64 allocation_count;
} memory_stats;

typedef struct memory_system_context {
    // Статистика памяти.
    memory_stats stats;
} memory_system_context;

static memory_system_context* context = nullptr;

bool memory_system_initialize()
{
    ASSERT(context == nullptr, "Memory system is already initialized.");

    context = platform_memory_allocate(sizeof(memory_system_context));
    if(!context)
    {
        timer_format requested;
        timer_get_format(sizeof(memory_system_context), &requested);
        LOG_ERROR("Memory subsystem did not allocate the requested %.2f%s of memory.", requested.amount, requested.unit);
        return false;
    }
    platform_memory_zero(context, sizeof(memory_system_context));

    return true;
}

void memory_system_shutdown()
{
    ASSERT(context != nullptr, "Memory system not initialized. Call memory_system_initialize() first.");

    if(context->stats.total_allocated && context->stats.allocation_count)
    {
        LOG_WARN("Detecting memory leaks...");
        const char* meminfo = memory_system_usage_str();
        LOG_WARN(meminfo);
        string_free(meminfo);
    }

    platform_memory_free(context);
    context = nullptr;
}

bool memory_system_is_initialized()
{
    return context != nullptr;
}

const char* memory_system_usage_str()
{
    ASSERT(context != nullptr, "Memory system not initialized. Call memory_system_initialize() first.");

    static const char* tag_names[MEMORY_TAG_COUNT] = {
        "UNKNOWN        ",
        "DARRAY         ",
        "STRING         ",
        "APPLICATION    ",
        "SYSTEM         ",
        "RENDERER       ",
    };

    //-----------------------------------------------------------------------------------------------------------------------

    // Буфер для вывода информации о памяти.
    char buffer[8000] = "Memory information:\n\n";
    const u16 buffer_length = sizeof(buffer);
    u64 offset = string_length(buffer);

    //-----------------------------------------------------------------------------------------------------------------------

    memory_format used;
    memory_get_format(context->stats.total_allocated, &used);

    // Запись статистики использования памяти в буфер.
    i32 length = string_format(buffer + offset, buffer_length, "Total memory usage: %.2f %s\n", used.amount, used.unit);

    // Обновление смещения для записи следующей строки.
    offset += length;

    //-----------------------------------------------------------------------------------------------------------------------

    memory_format peak;
    memory_get_format(context->stats.peak_allocated, &peak);

    // Запись пикового использования памяти.
    length = string_format(buffer + offset, buffer_length, "Peak memory usgae: %.2f %s\n", peak.amount, peak.unit);

    // Обновление смещения для записи следующей строки.
    offset += length;

    //-----------------------------------------------------------------------------------------------------------------------

    // Запись заглавной строки тегов.
    length = string_format(buffer + offset, buffer_length, "Memory usege by tags:\n");

    // Обновление смещения для записи следующей строки.
    offset += length;

    //-----------------------------------------------------------------------------------------------------------------------

    for(u32 i = 0; i < MEMORY_TAG_COUNT; ++i)
    {
        memory_format mtag;
        memory_get_format(context->stats.tagged_allocated[i], &mtag);

        // Запись строки тега и его значение в буфер.
        length = string_format(buffer + offset, buffer_length, "  %s: %7.2f %s\n", tag_names[i], mtag.amount, mtag.unit);

        // Обновление смещения для записи следующей строки.
        offset += length;
    }

    // Вернуть копию строки. Не забыть удалить после использование с использованием 'string_free'.
    return string_duplicate(buffer);
}

void* memory_allocate(u64 size, u16 alignment, memory_tag tag)
{
    UNUSED(alignment);

    ASSERT(context != nullptr, "Memory system not initialized. Call memory_system_initialize() first.");
    ASSERT(size > 0, "Size must be greater than zero.");
    ASSERT(alignment > 0, "Alignment must be greater than zero.");
    ASSERT(tag < MEMORY_TAG_COUNT, "Tag must be between 0 and MEMORY_TAG_COUNT.");

    void* block = platform_memory_allocate(size);
    if(!block)
    {
        timer_format requested;
        timer_get_format(size, &requested);
        LOG_ERROR("Memory system did not allocate the requested %.2f%s of memory.", requested.amount, requested.unit);
        LOG_FATAL("Failed to allocate memory. Stopping for debugging.");
        return nullptr;
    }

    context->stats.total_allocated += size;
    context->stats.tagged_allocated[tag] += size;
    context->stats.allocation_count++;

    if(context->stats.peak_allocated < context->stats.total_allocated)
    {
        context->stats.peak_allocated = context->stats.total_allocated;
    }

    return block;
}

void memory_free(void* block, u64 size, memory_tag tag)
{
    ASSERT(context != nullptr, "Memory system not initialized. Call memory_system_initialize() first.");
    ASSERT(block != nullptr, "Pointer to block must be non-null.");
    ASSERT(size > 0, "Size must be greater than zero.");
    ASSERT(tag < MEMORY_TAG_COUNT, "Tag must be between 0 and MEMORY_TAG_COUNT.");

    platform_memory_free(block);

    context->stats.total_allocated -= size;
    context->stats.tagged_allocated[tag] -= size;
    context->stats.allocation_count--;
}

void memory_get_format(u64 size, memory_format* out_format)
{
    if(size < 1 KiB)
    {
        out_format->unit = "B";
        out_format->amount = (f32)size;
    }
    else if(size < 1 MiB)
    {
        out_format->unit = "KiB";
        out_format->amount = (f32)size / (1 KiB);
    }
    else if(size < 1 GiB)
    {
        out_format->unit = "MiB";
        out_format->amount = (f32)size / (1 MiB);
    }
    else
    {
        out_format->unit = "GiB";
        out_format->amount = (f32)size / (1 GiB);
    }
}
