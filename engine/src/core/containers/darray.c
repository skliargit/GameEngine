#include "core/containers/darray.h"
#include "core/logger.h"
#include "core/memory.h"
#include "debug/assert.h"

void* dynamic_array_create(u64 stride, u64 capacity)
{
    ASSERT(stride > 0, "Stride must be greater than zero.");
    ASSERT(capacity > 0, "Capacity must be greater than zero.");

    // Проверка на переполнение.
    if(capacity > (U64_MAX - sizeof(dynamic_array_header)) / stride)
    {
        LOG_ERROR("Requested capacity too large: %llu.", capacity);
        return nullptr;
    }

    u64 total_size = sizeof(dynamic_array_header) + stride * capacity;
    dynamic_array_header* header = mallocate(total_size, MEMORY_TAG_DARRAY);
    if(!header)
    {
        LOG_ERROR("Failed to allocate memory for array to create.");
        return nullptr;
    }
    mzero(header, sizeof(dynamic_array_header));

    header->stride = stride;
    header->capacity = capacity;
    header->length = 0;

    return header->internal_data;
}

void dynamic_array_destroy(void* array)
{
    ASSERT(array != nullptr, "Array pointer must be non-null.");

    // Получение указателя на заголовок.
    dynamic_array_header* header = (dynamic_array_header*)array - 1;

    u64 size = sizeof(dynamic_array_header) + header->stride * header->capacity;
    mfree(header, size, MEMORY_TAG_DARRAY);
}

u64 dynamic_array_stride(void* array)
{
    ASSERT(array != nullptr, "Array pointer must be non-null.");

    dynamic_array_header* header = (dynamic_array_header*)array - 1;
    return header->stride;
}

u64 dynamic_array_length(void* array)
{
    ASSERT(array != nullptr, "Array pointer must be non-null.");

    dynamic_array_header* header = (dynamic_array_header*)array - 1;
    return header->length;
}

u64 dynamic_array_capacity(void* array)
{
    ASSERT(array != nullptr, "Array pointer must be non-null.");

    dynamic_array_header* header = (dynamic_array_header*)array - 1;
    return header->capacity;
}

static dynamic_array_header* dynamic_array_resize(dynamic_array_header* old_header, u64 new_capacity)
{
    // Проверка на переполнение.
    if(new_capacity > (U64_MAX - sizeof(dynamic_array_header)) / old_header->stride)
    {
        LOG_ERROR("Requested capacity too large: %llu.", new_capacity);
        return nullptr;
    }

    if(old_header->capacity >= new_capacity)
    {
        LOG_WARN("New capacity (%llu) must be greater than current capacity (%llu).", new_capacity, old_header->capacity);
        return nullptr;
    }

    u64 new_total_size = sizeof(dynamic_array_header) + old_header->stride * new_capacity;
    dynamic_array_header* new_header = mallocate(new_total_size, MEMORY_TAG_DARRAY);
    if(!new_header)
    {
        LOG_ERROR("Failed to allocate memory for array to resize.");
        return nullptr;
    }

    // Копирование заголовка и данных.
    u64 old_data_size = sizeof(dynamic_array_header) + old_header->stride * old_header->length;
    mcopy(new_header, old_header, old_data_size);

    // Обновление емкость в новом заголовке.
    new_header->capacity = new_capacity;

    // Освобождение старой памяти.
    u64 old_total_size = sizeof(dynamic_array_header) + old_header->stride * old_header->capacity;
    mfree(old_header, old_total_size, MEMORY_TAG_DARRAY);

    return new_header;
}

void dynamic_array_push(void** array, const void* data)
{
    ASSERT(array != nullptr && *array != nullptr, "Array pointer and data pointer must be non-null.");
    ASSERT(data != nullptr, "Data pointer must be non-null.");

    dynamic_array_header* header = (dynamic_array_header*)(*array) - 1;
    if(header->length >= header->capacity)
    {
        u64 new_capacity = header->capacity * DARRAY_DEFAULT_RESIZE_FACTOR;
        header = dynamic_array_resize(header, new_capacity);
        if(!header)
        {
            LOG_ERROR("Failed to resize array during push operation.");
            return;
        }
    }

    void* addr = header->internal_data + header->stride * header->length;
    mcopy(addr, data, header->stride);

    header->length++;
    *array = header->internal_data;
}

void dynamic_array_pop(void* array, void* out_data)
{
    ASSERT(array != nullptr, "Array pointer must be non-null.");

    dynamic_array_header* header = (dynamic_array_header*)array - 1;
    if(header->length == 0)
    {
        LOG_WARN("Cannot pop from an empty array.");
        return;
    }

    if(out_data)
    {
        void* addr = header->internal_data + header->stride * (header->length - 1);
        mcopy(out_data, addr, header->stride);
    }

    header->length--;
}

void dynamic_array_insert(void** array, u64 index, const void* data)
{
    ASSERT(array != nullptr && *array != nullptr, "Array pointer and data pointer must be non-null.");
    ASSERT(data != nullptr, "Data pointer must be non-null.");

    dynamic_array_header* header = (dynamic_array_header*)(*array) - 1;
    if(index > header->length)
    {
        LOG_ERROR("Index out of bounds: %llu (length: %llu).", index, header->length);
        return;
    }

    if(header->length >= header->capacity)
    {
        u64 new_capacity = header->capacity * DARRAY_DEFAULT_RESIZE_FACTOR;
        header = dynamic_array_resize(header, new_capacity);
        if(!header)
        {
            LOG_ERROR("Failed to resize array during insert operation.");
            return;
        }
    }

    u8* addr = header->internal_data + header->stride * index;

    // Сдвиг элементов только, если вставка не в конец массива.
    if(index < header->length)
    {
        mmove(addr + header->stride, addr, header->stride * (header->length - index));
    }

    mcopy(addr, data, header->stride);

    header->length++;
    *array = header->internal_data;
}

void dynamic_array_remove(void* array, u64 index, void* out_data)
{
    ASSERT(array != nullptr, "Array pointer must be non-null.");

    dynamic_array_header* header = (dynamic_array_header*)array - 1;
    if(header->length == 0)
    {
        LOG_WARN("Cannot remove from an empty array.");
        return;
    }

    if(index >= header->length)
    {
        LOG_ERROR("Index out of bounds: %llu (length: %llu).", index, header->length);
        return;
    }

    u8* addr = header->internal_data + header->stride * index;

    if(out_data)
    {
        mcopy(out_data, addr, header->stride);
    }

    if(index < (header->length - 1))
    {
        mmove(addr, addr + header->stride, header->stride * (header->length - index - 1));
    }

    header->length--;
}

void dynamic_array_clear(void* array, bool zero_memory)
{
    ASSERT(array != nullptr, "Array pointer must be non-null.");

    dynamic_array_header* header = (dynamic_array_header*)array - 1;
    if(zero_memory)
    {
        mzero(header->internal_data, header->stride * header->capacity);
    }

    header->length = 0;
}

void dynamic_array_set_length(void* array, u64 length)
{
    ASSERT(array != nullptr, "Array pointer must be non-null.");
    ASSERT(length > 0, "Length must be greater than zero.");

    dynamic_array_header* header = (dynamic_array_header*)array - 1;
    if(length > header->capacity)
    {
        LOG_ERROR("Length (%llu) exceeds capacity (%llu).", length, header->capacity);
        return;
    }

    header->length = length;
}
