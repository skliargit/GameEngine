#include "platform/linux/backend_xcb.h"

#if PLATFORM_LINUX_FLAG

    #include "core/logger.h"
    #include "platform/string.h"

    #include <xcb/xcb.h>
    #include <stdlib.h>

    typedef struct xcb_internal_data {
        xcb_connection_t* connection;  // Соединение с X сервером.
        xcb_window_t window;           // Идентификатор окна.
        xcb_atom_t wm_delete_win;      // Атом для обработки закрытия окна.
        bool should_close;             // Флаг запроса на закрытие.
    } xcb_internal_data;

    bool xcb_backend_create(const platform_window_config* config, void* internal_data)
    {
        xcb_internal_data* context = internal_data;

        // Установка соединение с X сервером.
        context->connection = xcb_connect(nullptr, nullptr);
        if(xcb_connection_has_error(context->connection))
        {
            LOG_ERROR("%s: Failed to connect to X11 display.", __func__);
            return false;
        }

        // Получение первого экрана.
        const xcb_setup_t* setup = xcb_get_setup(context->connection);
        xcb_screen_t* screen = xcb_setup_roots_iterator(setup).data;

        // Получение id для нового окна.
        context->window = xcb_generate_id(context->connection);
        if(context->window < 0)
        {
            LOG_ERROR("%s: Failed to get newly XID.", __func__);
            return false;
        }

        // Настройка событий окна.
        u32 value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;                // Установка фона и событий для окна.
        u32 value_list[] = {
            screen->black_pixel,
            XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |            // Обработка событий клавиатуры.
            XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |      // Обработка событий мышки.
            XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY |  // Обработка перемещения мышки и изменение размера окна.
            XCB_EVENT_MASK_EXPOSURE                                            // Обработка перерисовки окна после перекрытие другими окнами или скрытия.
        };

        // Создание окна.
        xcb_create_window(
            context->connection, XCB_COPY_FROM_PARENT, context->window, screen->root, 0, 0, config->width, config->height, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, value_mask, value_list
        );

        // Изменение заголовка окна.
        xcb_change_property(
            context->connection, XCB_PROP_MODE_REPLACE, context->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
            platform_string_length(config->title), config->title
        );

        // Подписка окна на протокол WM_DELETE_WINDOW.
        // NOTE: Без него программа не получает события о закрытии окна, сервер просто грохнет окно
        //       и приложение уйдет в ошибки при очередном цикле обработки событий.
        // Запрос.
        xcb_intern_atom_cookie_t cookies[2] = {
            xcb_intern_atom(context->connection, 0, 16, "WM_DELETE_WINDOW"),
            xcb_intern_atom(context->connection, 0, 12, "WM_PROTOCOLS")
        };
        // Ответ.
        xcb_intern_atom_reply_t* replies[2] = {
            xcb_intern_atom_reply(context->connection, cookies[0], nullptr), // wm_delete_win.
            xcb_intern_atom_reply(context->connection, cookies[1], nullptr)  // wm_protocols.
        };
        // Сохранение атомов.
        xcb_atom_t wm_delete_win = replies[0]->atom;
        xcb_atom_t wm_protocols  = replies[1]->atom;
        free(replies[0]);
        free(replies[1]);
        // Подписка на WM_DELETE_WINDOW.
        xcb_change_property(
            context->connection, XCB_PROP_MODE_REPLACE, context->window, wm_protocols, XCB_ATOM_ATOM, 32, 1, &wm_delete_win
        );

        // Сохранение атома(числа) для дальнейшего использования (сравнения в сообытии с установлением запроса).
        context->wm_delete_win = wm_delete_win;

        // Показать окно.
        xcb_map_window(context->connection, context->window);
        if(xcb_flush(context->connection) <= 0)
        {
            LOG_ERROR("%s: Failed to flush XCB connection (server disconnected or protocol error).", __func__);
            return false;
        }

        context->should_close = false;
        return true;
    }

    void xcb_backend_destroy(void* internal_data)
    {
        xcb_internal_data* context = internal_data;
        if(context->connection)
        {
            // NOTE: Уничтожение в обратном порядке.

            xcb_destroy_window(context->connection, context->window);
            context->window = 0;

            xcb_disconnect(context->connection);
            context->connection = nullptr;
        }
    }

    bool xcb_backend_poll_events(void* internal_data)
    {
        xcb_internal_data* context = internal_data;
        xcb_generic_event_t* event;

        while((event = xcb_poll_for_event(context->connection)))
        {
            // Выбор обрабатываемого события.
            switch(event->response_type & ~0x80)
            {
                // Обработка клавиш клавиатуры.
                case XCB_KEY_PRESS:
                case XCB_KEY_RELEASE: {
                    // TODO:
                    // xcb_key_press_event_t* key = (xcb_key_press_event_t*)event;
                    // bool pressed = event->response_type == XCB_KEY_PRESS;
                    // xcb_keycode_t code = key->detail;
                    // ...
                    LOG_TRACE("%s: Key event.", __func__);
                } break;

                // Обработка кнопок мышки.
                case XCB_BUTTON_PRESS:
                case XCB_BUTTON_RELEASE: {
                    // TODO:
                    // xcb_button_press_event_t* button = (xcb_button_press_event_t*)event;
                    // bool pressed = event->response_type == XCB_BUTTON_PRESS;
                    // ...
                    LOG_TRACE("%s: Button event.", __func__);
                } break;

                // Обработка положения указателя.
                case XCB_MOTION_NOTIFY: {
                    // TODO:
                    // xcb_motion_notify_event_t* position = (xcb_motion_notify_event_t*)event;
                    // ...
                    // LOG_TRACE("%s: Motion event.", __func__);
                } break;

                // Обработка изменения размера окна.
                case XCB_CONFIGURE_NOTIFY: {
                    // TODO:
                    // xcb_configure_notify_event_t* configure = (xcb_configure_notify_event_t*)event;
                    // u16 width  = configure->width;
                    // u16 height = configure->height;
                    // ...
                    // TODO: Проверять кешированные размеры с новыми. При перетаскивании окна вызывается событие!
                    LOG_TRACE("%s: Resize event.", __func__);
                } break;

                // Обработка запроса о закрытии окна.
                case XCB_CLIENT_MESSAGE: {
                    xcb_client_message_event_t* cm = (xcb_client_message_event_t*)event;
                    context->should_close = (cm->data.data32[0] == context->wm_delete_win);
                    LOG_TRACE("%s: Close event.", __func__);
                } break;

                // Обработка неизвестных событий.
                default:
                    LOG_TRACE("%s: Unsupported event: %hhu", __func__, event->response_type);
                    // TODO: xcb_backend_poll_events: Unsupported event: 12 - XCB_EXPOSE.
                    //       Генерируется, когда окно или его часть должны быть перерисованы.
                    //       Важно для Vulkan, так как может указывать на необходимость перерисовки поверхности.
                    //
                    // TODO: xcb_backend_poll_events: Unsupported event: 19 - XCB_MAPPING_NOTIFY.
                    //       Уведомляет об изменении раскладки клавиатуры.
                    //
                    // TODO: xcb_backend_poll_events: Unsupported event: 34 - XCB_LEAVE_NOTIFY.
                    //       Происходит, когда курсор мыши покидает окно.
            }

            // Удаление обработанного события.
            free(event);
        }

        // Проверка требования закрыть окно. Завершить работу!
        if(context->should_close) return false;

        // Продолжить работу!
        return true;
    }

    bool xcb_backend_is_supported()
    {
        xcb_connection_t* connection = xcb_connect(nullptr, nullptr);
        bool result = !xcb_connection_has_error(connection);
        if(result) xcb_disconnect(connection);
        return result;
    }

    u64 xcb_backend_instance_memory_requirement()
    {
        return sizeof(xcb_internal_data);
    }

#endif
