#include "core/event.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/containers/darray.h"
#include "debug/assert.h"

typedef struct event_listener {
    // Указатель на объект слушатель.
    void* instance;
    // Обработчик события.
    on_event_callback handler;
} event_listener;

typedef struct event {
    // Слушатели события (используется darray).
    event_listener* listeners;
} event;

typedef struct event_system_context {
    event events[EVENT_CODE_COUNT];
    bool is_running;
} event_system_context;

static event_system_context* context = nullptr;

bool event_system_initialize()
{
    ASSERT(context == nullptr, "Event system is already initialized.");

    context = mallocate(sizeof(event_system_context), MEMORY_TAG_SYSTEM);
    if(!context)
    {
        LOG_ERROR("Failed to allocate memory for event system to initialize.");
        return false;
    }
    mzero(context, sizeof(event_system_context));

    context->is_running = true;
    return true;
}

void event_system_shutdown()
{
    ASSERT(context != nullptr, "Event system not initialized. Call event_system_initialize() first.");

    // Запрещает работу функций, до уничтожения контекста (актуально для многопоточного приложения).
    context->is_running = false;

    // Освобождение выделенных ресурсов.
    for(u32 i = 0; i < EVENT_CODE_COUNT; ++i)
    {
        if(context->events[i].listeners != nullptr)
        {
            darray_destroy(context->events[i].listeners);
            context->events[i].listeners = nullptr;
        }
    }

    mfree(context, sizeof(event_system_context), MEMORY_TAG_SYSTEM);
    context = nullptr;
}

bool event_system_is_initialized()
{
    return context != nullptr && context->is_running;
}

bool event_register(event_code code, void* listener, on_event_callback handler)
{
    ASSERT(context != nullptr, "Event system not initialized. Call event_system_initialize() first.");
    ASSERT(code >= 0 && code < EVENT_CODE_COUNT, "Event code must be between 0 and EVENT_CODE_COUNT.");
    ASSERT(handler != nullptr, "Handler pointer must be non-null.");

    if(!context->is_running)
    {
        LOG_ERROR("Event system is not running. Cannot register event handler.");
        return false;
    }

    // Создание списка слушателей, если его нет.
    if(context->events[code].listeners == nullptr)
    {
        context->events[code].listeners = darray_create(event_listener);
        LOG_TRACE("Created new listener array for event code: %d.", code);
    }

    // Получение текущего количества слушателей.
    u64 listener_count = darray_length(context->events[code].listeners);

    // Проверка наличия слушателя в списке.
    for(u64 i = 0; i < listener_count; ++i)
    {
        event_listener* entry = &context->events[code].listeners[i];
        if(entry->instance == listener && entry->handler == handler)
        {
            LOG_WARN("Event is already registered with code: %d.", code);
            return false;
        }
    }

    // Регистрация нового слушателя.
    event_listener entry = {listener, handler};
    darray_push(context->events[code].listeners, entry);
    LOG_TRACE("Registered event handler for event code: %d, listener: %p.", code, listener);
    return true;
}

bool event_unregister(event_code code, void* listener, on_event_callback handler)
{
    ASSERT(context != nullptr, "Event system not initialized. Call event_system_initialize() first.");
    ASSERT(code >= 0 && code < EVENT_CODE_COUNT, "Event code must be between 0 and EVENT_CODE_COUNT.");
    ASSERT(handler != nullptr, "Handler pointer must be non-null.");

    if(!context->is_running)
    {
        LOG_ERROR("Event system is not running. Cannot unregister event handler.");
        return false;
    }

    if(context->events[code].listeners == nullptr)
    {
        LOG_WARN("Event code %d has no listeners to unregister from.", code);
        return false;
    }

    // Получение текущего количества слушателей для удаления.
    u64 listener_count = darray_length(context->events[code].listeners);

    for(u64 i = 0; i < listener_count; ++i)
    {
        event_listener* entry = &context->events[code].listeners[i];
        if(entry->instance == listener && entry->handler == handler)
        {
            darray_remove(context->events[code].listeners, i, nullptr);
            LOG_TRACE("Unregistered event handler for event code: %d, listener: %p.", code, listener);
            return true;
        }
    }

    LOG_WARN("Event handler not found for unregistration. Event code: %d, listener: %p.", code, listener);
    return false;
}

bool event_send(event_code code, void* sender, event_context* data)
{
    ASSERT(context != nullptr, "Event system not initialized. Call event_system_initialize() first.");
    ASSERT(code >= 0 && code < EVENT_CODE_COUNT, "Event code must be between 0 and EVENT_CODE_COUNT.");

    if(!context->is_running)
    {
        LOG_ERROR("Event system is not running. Cannot send event.");
        return false;
    }

    if(context->events[code].listeners == nullptr)
    {
        // LOG_TRACE("Event code %d has no listeners. Event not processed.", code);
        return false;
    }

    // Получение текущего количества слушателей для удаления.
    u64 listener_count = darray_length(context->events[code].listeners);
    LOG_TRACE("Dispatching event code: %d to %llu listeners.", code, listener_count);

    bool event_handled = false;
    for(u64 i = 0; i < listener_count; ++i)
    {
        event_listener* entry = &context->events[code].listeners[i];
        if(entry->handler(code, sender, entry->instance, data))
        {
            LOG_TRACE("Event code: %d handled by listener: %p and propagation stopped.", code, entry->instance);
            event_handled = true;
            break;
        }
    }

    if(!event_handled)
    {
        LOG_TRACE("Event code: %d processed by all listeners without stopping propagation.", code);
    }

    return event_handled;
}
