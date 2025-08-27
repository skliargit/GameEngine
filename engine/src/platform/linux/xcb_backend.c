#include "platform/linux/xcb_backend.h"

#ifdef PLATFORM_LINUX_FLAG

    #include "core/logger.h"
    #include "core/string.h"
    #include "core/memory.h"
    #include <xcb/xcb.h>
    #include <stdlib.h>

    // Для включения отладки для оконной системы индивидуально.
    #ifndef DEBUG_WINDOW_FLAG
        #undef LOG_DEBUG
        #undef LOG_TRACE
        #define LOG_DEBUG(...) UNUSED(0)
        #define LOG_TRACE(...) UNUSED(0)
    #endif

    typedef struct platform_window {
        xcb_window_t id;                  // Идентификатор окна XCB.
        xcb_atom_t wm_delete;             // Атом для обработки закрытия окна.
        char* title;                      // Текущий заголовок окна.
        i32 width;                        // Текущая ширина окна в пикселях.
        i32 height;                       // Текущая высота окна в пикселях.
        i32 width_pending;                // Новая ширина окна, ожидающая применения.
        i32 height_pending;               // Новая высота окна, ожидающая применения.
        bool resize_pending;              // Флаг ожидания обработки изменения размера.
        bool should_close;                // TODO: Флаг запроса на закрытие окна (пока что вылет с ошибкой).
        bool focused;                     // Фокус ввода на этом окне.
    } platform_window;

    typedef struct xcb_client {
        xcb_connection_t* connection;     // Соединение с X сервером.
        xcb_screen_t* screen;             // Информация о экране.
        platform_window* focused_window;  // TODO: Окно с текущим фокусом ввода.
        platform_window* window;          // TODO: Динамический массив всех окон (пока только одно окно).
    } xcb_client;

    static xcb_screen_t* get_screen(xcb_connection_t *connection, int screen_num)
    {
        xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(connection));
        for(int i = 0; i < screen_num; i++)
        {
            xcb_screen_next(&iter);
        }
        return iter.data;
    }

    bool xcb_backend_initialize(void* internal_data)
    {
        xcb_client* client = internal_data;

        // Установка соединение с X сервером.
        client->connection = xcb_connect(nullptr, nullptr);
        if(xcb_connection_has_error(client->connection))
        {
            LOG_ERROR("Failed to connect to X server.");
            return false;
        }
        LOG_TRACE("Connected to X server.");

        // Получение первого экрана.
        client->screen = get_screen(client->connection, 0);
        if(!client->screen)
        {
            LOG_ERROR("Failed to get screen from X server.");
            xcb_backend_shutdown(client);
            return false;
        }

        LOG_TRACE("XCB backend initialized successfully.");
        client->window = nullptr;
        return true;
    }

    void xcb_backend_shutdown(void* internal_data)
    {
        xcb_client* client = internal_data;

        LOG_TRACE("Disconnecting from X server...");

        // Уничтожение всех окон (пока только одно окно).
        if(client->window)
        {
            LOG_WARN("Window was not properly destroyed before shutdown.");
            xcb_backend_window_destroy(client->window, client);
            // NOTE: Осовобождение client->window в xcb_backend_window_destroy().
        }

        if(client->connection)
        {
            xcb_disconnect(client->connection);
        }

        mzero(client, sizeof(xcb_client));
        LOG_TRACE("XCB backend shutdown complete.");
    }

    bool xcb_backend_poll_events(void* internal_data)
    {
        xcb_client* client = internal_data;
        xcb_generic_event_t* event;

        // TODO: Временная мера!
        // Проверка требования закрыть окно. Завершить работу!
        if(client->window->should_close)
        {
            LOG_TRACE("Window close requested, stopping event loop.");
            return false;
        }

        while((event = xcb_poll_for_event(client->connection)))
        {
            u8 response_type = event->response_type & ~0x80;

            // Выбор обрабатываемого события.
            switch(response_type)
            {
                // Обработка клавиш клавиатуры.
                case XCB_KEY_PRESS:
                case XCB_KEY_RELEASE: {
                    // TODO:
                    // xcb_key_press_event_t* key = (xcb_key_press_event_t*)event;
                    // if(window->id == key->event) ...
                    // bool pressed = event->response_type == XCB_KEY_PRESS;
                    // xcb_keycode_t code = key->detail;
                    // ...
                    LOG_TRACE("Key event.");
                } break;

                // Обработка изменения раскладки клавиатуры.
                case XCB_MAPPING_NOTIFY: {
                } break;

                // Обработка кнопок мышки.
                case XCB_BUTTON_PRESS:
                case XCB_BUTTON_RELEASE: {
                    // TODO:
                    // xcb_button_press_event_t* button = (xcb_button_press_event_t*)event;
                    // if(window->id == button->event) ...
                    // bool pressed = event->response_type == XCB_BUTTON_PRESS;
                    // ...
                    LOG_TRACE("Button event.");
                } break;

                // Обработка положения указателя.
                case XCB_MOTION_NOTIFY: {
                    // TODO:
                    // xcb_motion_notify_event_t* position = (xcb_motion_notify_event_t*)event;
                    // if(window->id == position->event) ...
                    // ...
                    // LOG_TRACE("Motion event.");
                } break;

                // Обработка изменения размера окна.
                case XCB_CONFIGURE_NOTIFY: {
                    xcb_configure_notify_event_t* e = (xcb_configure_notify_event_t*)event;
                    if(client->window->width != e->width || client->window->height != e->height)
                    {
                        client->window->width_pending  = e->width;
                        client->window->height_pending = e->height;
                        client->window->resize_pending = true;
                        LOG_TRACE("Window resize pending: %dx%d.", e->width, e->height);
                    }
                } break;

                // Обработка перерисовки окна.
                case XCB_EXPOSE: {
                    LOG_TRACE("Resize event for window '%s' to %dx%d.", client->window->title,
                        client->window->width_pending, client->window->height_pending
                    );

                    // TODO: Вызов обработчика изменения размера буфера.

                    client->window->width = client->window->width_pending;
                    client->window->height = client->window->height_pending;
                    client->window->resize_pending = false;
                } break;

                // Обработка события фокуса окна.
                case XCB_ENTER_NOTIFY: {
                    // xcb_enter_notify_event_t* e = (xcb_enter_notify_event_t*)event;
                    // TODO: Найти окно которому принадлежит id!
                    // if(client->window->id == e->event)
                    // {
                    //     client->focused_window = client->window;
                    //     client->window->focused = true;
                    // }
                    LOG_TRACE("Window '%s' focused.", client->window->title);
                } break;

                case XCB_LEAVE_NOTIFY: {
                    // xcb_enter_notify_event_t* e = (xcb_enter_notify_event_t*)event;
                    // TODO: Найти окно которому принадлежит id!
                    // if(client->window->id == e->event)
                    // {
                    //     client->window->focused = false;
                    //     if(client->focused_window == client->window)
                    //     {
                    //         client->focused_window = nullptr;
                    //     }
                    // }
                    LOG_TRACE("Window '%s' has lost focus.", client->window->title);
                } break;

                // Обработка запроса о закрытии окна.
                case XCB_CLIENT_MESSAGE: {
                    xcb_client_message_event_t* e = (xcb_client_message_event_t*)event;
                    client->window->should_close = (e->data.data32[0] == client->window->wm_delete);
                    LOG_TRACE("Close event for window '%s'.", client->window->title);
                } break;

                // Обработка появления окна.
                case XCB_MAP_NOTIFY: {
                } break;

                // Обработка неизвестных событий.
                default:
                    LOG_TRACE("Unhandled event type: %d.", response_type);
            }

            // Удаление обработанного события.
            free(event);
        }

        return true;
    }

    bool xcb_backend_is_supported()
    {
        xcb_connection_t* connection = xcb_connect(nullptr, nullptr);
        if(xcb_connection_has_error(connection))
        {
            return false;
        }
        xcb_disconnect(connection);
        return true;
    }

    u64 xcb_backend_memory_requirement()
    {
        return sizeof(xcb_client);
    }

    platform_window* xcb_backend_window_create(const platform_window_config* config, void* internal_data)
    {
        xcb_client* client = internal_data;

        LOG_TRACE("Creating window '%s' (size: %dx%d)...", config->title, config->width, config->height);

        if(client->window)
        {
            LOG_WARN("Window has already been created (only one window supported for now).");
            return nullptr;
        }

        if(!client->connection || xcb_connection_has_error(client->connection))
        {
            LOG_ERROR("XCB connection is not available.");
            return nullptr;
        }

        platform_window* window = mallocate(sizeof(platform_window), MEMORY_TAG_APPLICATION);
        mzero(window, sizeof(platform_window));
        window->title = string_duplicate(config->title);
        window->width = config->width;
        window->height = config->height;

        // Генерация ID для нового окна.
        window->id = xcb_generate_id(client->connection);
        LOG_TRACE("XCB window ID: %u.", window->id);

        // Настройка событий окна.
        u32 value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;                // Установка фона и событий для окна.
        u32 value_list[] = {
            client->screen->black_pixel,
            XCB_EVENT_MASK_KEY_PRESS      | XCB_EVENT_MASK_KEY_RELEASE      |  // События клавиш клавиатуры.
            XCB_EVENT_MASK_BUTTON_PRESS   | XCB_EVENT_MASK_BUTTON_RELEASE   |  // События кнопок мышки.
            XCB_EVENT_MASK_ENTER_WINDOW   | XCB_EVENT_MASK_LEAVE_WINDOW     |  // События фокуса окна.
            XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY |  // События движение мышки и изменениеы размера окна.
            XCB_EVENT_MASK_EXPOSURE                                            // Событие перерисовка окна.
        };

        // Создание окна.
        xcb_create_window(
            client->connection, XCB_COPY_FROM_PARENT, window->id, client->screen->root, 0, 0, config->width, config->height,
            0, XCB_WINDOW_CLASS_INPUT_OUTPUT, client->screen->root_visual, value_mask, value_list
        );

        // Установка заголовка окна.
        xcb_change_property(
            client->connection, XCB_PROP_MODE_REPLACE, window->id, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
            string_length(window->title), window->title
        );

        // Установка класса окна (WM_CLASS).
        // NOTE: Согласно ICCCM и EWMH, свойство WM_CLASS состоит из двух частей: "instance_name\0class_name\0".
        char wm_class[50];
        u32 wm_class_length = string_format(wm_class, 50, "Window%d%cGameEngineWindow", window->id, '\0') + 1;
        xcb_change_property(
            client->connection, XCB_PROP_MODE_REPLACE, window->id, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8,
            wm_class_length, wm_class
        );

        // Подписка окна на протокол WM_DELETE_WINDOW.
        // NOTE: Без него программа не получает события о закрытии окна, сервер просто грохнет окно
        //       и приложение уйдет в ошибки при очередном цикле обработки событий.
        // Запрос.
        xcb_intern_atom_cookie_t cookies[2] = {
            xcb_intern_atom(client->connection, 0, 16, "WM_DELETE_WINDOW"),
            xcb_intern_atom(client->connection, 0, 12, "WM_PROTOCOLS")
        };

        // Ответ.
        xcb_intern_atom_reply_t* replies[2] = {
            xcb_intern_atom_reply(client->connection, cookies[0], nullptr), // wm_delete_window.
            xcb_intern_atom_reply(client->connection, cookies[1], nullptr)  // wm_protocols.
        };

        // Проверка ответов.
        if(!replies[0] || !replies[1])
        {
            if(replies[0]) free(replies[0]);
            if(replies[1]) free(replies[1]);

            LOG_ERROR("Failed to get WM protocol atoms.");
            xcb_backend_window_destroy(window, client);
            return nullptr;
        }

        // Сохранение атомов.
        xcb_atom_t wm_delete = replies[0]->atom;
        xcb_atom_t wm_protocols = replies[1]->atom;
        free(replies[0]);
        free(replies[1]);

        // Подписка на WM_DELETE_WINDOW.
        xcb_change_property(
            client->connection, XCB_PROP_MODE_REPLACE, window->id, wm_protocols, XCB_ATOM_ATOM, 32, 1, &wm_delete
        );

        // Сохранение атома(числа) для дальнейшего использования.
        window->wm_delete = wm_delete;

        // Показать окно.
        xcb_map_window(client->connection, window->id);
        if(xcb_flush(client->connection) <= 0)
        {
            LOG_ERROR("Failed to flush XCB connection (server disconnected).");
            xcb_backend_window_destroy(window, client);
            return false;
        }

        LOG_TRACE("Window '%s' created successfully.", window->title);

        client->window = window;
        return client->window;
    }

    void xcb_backend_window_destroy(platform_window* window, void* internal_data)
    {
        xcb_client* client = internal_data;

        LOG_TRACE("Destroying window '%s'...", window->title);

        // TODO: Удаление окна из списка окон.

        if(window->id >= 0) xcb_destroy_window(client->connection, window->id);
        if(window->title)   string_free(window->title);
        xcb_flush(client->connection);

        mfree(window, sizeof(platform_window), MEMORY_TAG_APPLICATION);
        client->window = nullptr;
        LOG_TRACE("Window destroy complete.");
    }

#endif
