#include "core/input.h"
#include "core/memory.h"
#include "core/logger.h"
#include "debug/assert.h"

typedef struct input_state {
    // Состояние всех клавиш.
    bool keys[KEY_COUNT];
    // Состояние всех кнопок.
    bool buttons[BUTTON_COUNT];
} input_state;

typedef struct input_system_context {
    struct {
        // Состояние ввода на текущем кадре.
        input_state current;
        // Состояние ввода на предыдущем кадре.
        input_state previous;
        // Абсолютная позиция курсора по x.
        i32 position_x;
        // Абсолютная позиция курсора по y.
        i32 position_y;
        // Относительная позиция курсора по x.
        i32 delta_x;
        // Относительная позиция курсора по y.
        i32 delta_y;
        // Дельта вертикальной прокрутки колесика.
        i32 wheel_delta_v;
        // Дельта горизонтальной прокрутки колесика.
        i32 wheel_delta_h;
    } state;
} input_system_context;

static input_system_context* context = nullptr;

bool input_system_initialize()
{
    ASSERT(context == nullptr, "Input system is already initialized.");

    context = mallocate(sizeof(input_system_context), MEMORY_TAG_SYSTEM);
    if(!context)
    {
        LOG_ERROR("Failed to allocate memory for input system to initialize.");
        return false;
    }
    mzero(context, sizeof(input_system_context));

    return true;
}

void input_system_shutdown()
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");

    mfree(context, sizeof(input_system_context), MEMORY_TAG_SYSTEM);
    context = nullptr;
}

bool input_system_is_initialized()
{
    return context != nullptr;
}

void input_system_update()
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");

    context->state.delta_x = 0;
    context->state.delta_y = 0;
    context->state.wheel_delta_v = 0.0f;
    context->state.wheel_delta_h = 0.0f;
    mcopy(&context->state.previous, &context->state.current, sizeof(input_state));
}

void input_keyboard_key_update(keyboard_key key, bool state)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");
    ASSERT(key > KEY_UNKNOWN && key < KEY_COUNT, "Key code must be between 0 and KEY_COUNT.");

    context->state.current.keys[key] = state;
}

void input_mouse_button_update(mouse_button button, bool state)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");
    ASSERT(button > BUTTON_UNKNOWN && button < BUTTON_COUNT, "Button code must be between 0 and BUTTON_COUNT.");

    context->state.current.buttons[button] = state;
}

void input_mouse_position_update(i32 x, i32 y)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");

    context->state.position_x = x;
    context->state.position_y = y;
}

void input_mouse_delta_update(i32 x, i32 y)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");

    context->state.delta_x = x;
    context->state.delta_y = y;
}

void input_mouse_wheel_update(i32 vertical_delta, i32 horizontal_delta)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");

    context->state.wheel_delta_v += vertical_delta;
    context->state.wheel_delta_h += horizontal_delta;
}

bool input_key_down(keyboard_key key)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");
    ASSERT(key > KEY_UNKNOWN && key < KEY_COUNT, "Key code must be between 0 and KEY_COUNT.");

    return context->state.current.keys[key] && !context->state.previous.keys[key];
}

bool input_key_up(keyboard_key key)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");
    ASSERT(key > KEY_UNKNOWN && key < KEY_COUNT, "Key code must be between 0 and KEY_COUNT.");

    return !context->state.current.keys[key] && context->state.previous.keys[key];
}

bool input_key_held(keyboard_key key)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");
    ASSERT(key > KEY_UNKNOWN && key < KEY_COUNT, "Key code must be between 0 and KEY_COUNT.");

    return context->state.current.keys[key];
}

const char* input_key_to_str(keyboard_key key)
{
    ASSERT(key > KEY_UNKNOWN && key < KEY_COUNT, "Key code must be between 0 and KEY_COUNT.");

    static const char* key_strings[KEY_COUNT] = {
        [KEY_BACKSPACE]    = "BACKSPACE",        [KEY_TAB]          = "TAB",              [KEY_RETURN]       = "ENTER",
        [KEY_PAUSE]        = "PAUSE",            [KEY_CAPSLOCK]     = "CAPSLOCK",         [KEY_ESCAPE]       = "ESCAPE",
        [KEY_SPACE]        = "SPACE",            [KEY_PAGEUP]       = "PAGEUP",           [KEY_PAGEDOWN]     = "PAGEDOWN",
        [KEY_END]          = "END",              [KEY_HOME]         = "HOME",             [KEY_LEFT]         = "LEFT",
        [KEY_UP]           = "UP",               [KEY_RIGHT]        = "RIGHT",            [KEY_DOWN]         = "DOWN",
        [KEY_PRINTSCREEN]  = "PRINTSCREEN",      [KEY_INSERT]       = "INSERT",           [KEY_DELETE]       = "DELETE",
        [KEY_0]            = "0",                [KEY_1]            = "1",                [KEY_2]            = "2",
        [KEY_3]            = "3",                [KEY_4]            = "4",                [KEY_5]            = "5",
        [KEY_6]            = "6",                [KEY_7]            = "7",                [KEY_8]            = "8",
        [KEY_9]            = "9",                [KEY_A]            = "A",                [KEY_B]            = "B",
        [KEY_C]            = "C",                [KEY_D]            = "D",                [KEY_E]            = "E",
        [KEY_F]            = "F",                [KEY_G]            = "G",                [KEY_H]            = "H",
        [KEY_I]            = "I",                [KEY_J]            = "J",                [KEY_K]            = "K",
        [KEY_L]            = "L",                [KEY_M]            = "M",                [KEY_N]            = "N",
        [KEY_O]            = "O",                [KEY_P]            = "P",                [KEY_Q]            = "Q",
        [KEY_R]            = "R",                [KEY_S]            = "S",                [KEY_T]            = "T",
        [KEY_U]            = "U",                [KEY_V]            = "V",                [KEY_W]            = "W",
        [KEY_X]            = "X",                [KEY_Y]            = "Y",                [KEY_Z]            = "Z",
        [KEY_LSUPER]       = "LSUPER",           [KEY_RSUPER]       = "RSUPER",           [KEY_SLEEP]        = "SLEEP",
        [KEY_NUMPAD0]      = "NUMPAD0",          [KEY_NUMPAD1]      = "NUMPAD1",          [KEY_NUMPAD2]      = "NUMPAD2",
        [KEY_NUMPAD3]      = "NUMPAD3",          [KEY_NUMPAD4]      = "NUMPAD4",          [KEY_NUMPAD5]      = "NUMPAD5",
        [KEY_NUMPAD6]      = "NUMPAD6",          [KEY_NUMPAD7]      = "NUMPAD7",          [KEY_NUMPAD8]      = "NUMPAD8",
        [KEY_NUMPAD9]      = "NUMPAD9",          [KEY_MULTIPLY]     = "MULTIPLY",         [KEY_ADD]          = "ADD",
        [KEY_SUBTRACT]     = "SUBTRACT",         [KEY_DECIMAL]      = "DECIMAL",          [KEY_DIVIDE]       = "DIVIDE",
        [KEY_F1]           = "F1",               [KEY_F2]           = "F2",               [KEY_F3]           = "F3",
        [KEY_F4]           = "F4",               [KEY_F5]           = "F5",               [KEY_F6]           = "F6",
        [KEY_F7]           = "F7",               [KEY_F8]           = "F8",               [KEY_F9]           = "F9",
        [KEY_F10]          = "F10",              [KEY_F11]          = "F11",              [KEY_F12]          = "F12",
        [KEY_F13]          = "F13",              [KEY_F14]          = "F14",              [KEY_F15]          = "F15",
        [KEY_F16]          = "F16",              [KEY_F17]          = "F17",              [KEY_F18]          = "F18",
        [KEY_F19]          = "F19",              [KEY_F20]          = "F20",              [KEY_F21]          = "F21",
        [KEY_F22]          = "F22",              [KEY_F23]          = "F23",              [KEY_F24]          = "F24",
        [KEY_NUMLOCK]      = "NUMLOCK",          [KEY_SCROLLLOCK]   = "SCROLLOCK",        [KEY_LSHIFT]       = "LSHIFT",
        [KEY_RSHIFT]       = "RSHIFT",           [KEY_LCONTROL]     = "LCONTROL",         [KEY_RCONTROL]     = "RCONTROL",
        [KEY_LALT]         = "LALT",             [KEY_RALT]         = "RALT",             [KEY_SEMICOLON]    = "SEMICOLON",
        [KEY_APOSTROPHE]   = "APOSTROPHE/QUOTE", [KEY_EQUAL]        = "EQUAL/PLUS",       [KEY_COMMA]        = "COMMA",
        [KEY_MINUS]        = "MINUS",            [KEY_DOT]          = "DOT",              [KEY_SLASH]        = "SLASH",
        [KEY_GRAVE]        = "GRAVE",            [KEY_LBRACKET]     = "LBRACKET",         [KEY_BACKSLASH]    = "BACKSLASH/PIPE",
        [KEY_RBRACKET]     = "RBRACKET",         [KEY_MENU]         = "CONTEXT MENU"
    };

    if(key <= KEY_UNKNOWN || key >= KEY_COUNT || !key_strings[key])
    {
        return "UNKNOWN";
    }

    return key_strings[key];
}

bool input_mouse_down(mouse_button button)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");
    ASSERT(button < BUTTON_COUNT, "Button code must be between 0 and BUTTON_COUNT.");

    return context->state.current.buttons[button] && !context->state.previous.buttons[button];
}

bool input_mouse_up(mouse_button button)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");
    ASSERT(button < BUTTON_COUNT, "Button code must be between 0 and BUTTON_COUNT.");

    return !context->state.current.buttons[button] && context->state.previous.buttons[button];
}

bool input_mouse_held(mouse_button button)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");
    ASSERT(button < BUTTON_COUNT, "Button code must be between 0 and BUTTON_COUNT.");

    return context->state.current.buttons[button];
}

void input_mouse_position(i32* out_x, i32* out_y)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");
    ASSERT(out_x != nullptr, "Position pointer must be non-null.");
    ASSERT(out_y != nullptr, "Position pointer must be non-null.");

    *out_x = context->state.position_x;
    *out_y = context->state.position_y;
}

bool input_mouse_delta(i32* out_dx, i32* out_dy)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");
    ASSERT(out_dx != nullptr, "Offset pointer must be non-null.");
    ASSERT(out_dy != nullptr, "Offset pointer must be non-null.");

    if(context->state.delta_x != 0 || context->state.delta_y != 0)
    {
        *out_dx = context->state.delta_x;
        *out_dy = context->state.delta_y;
        return true;
    }

    return false;
}

bool input_mouse_wheel_vertical(i32* out_delta)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");
    ASSERT(out_delta != nullptr, "Delta pointer must be non-null.");

    *out_delta = context->state.wheel_delta_v;
    return *out_delta != 0;
}

bool input_mouse_wheel_horizontal(i32* out_delta)
{
    ASSERT(context != nullptr, "Input system not initialized. Call input_system_initialize() first.");
    ASSERT(out_delta != nullptr, "Delta pointer must be non-null.");

    *out_delta = context->state.wheel_delta_h;
    return *out_delta != 0;
}

const char* input_mouse_button_to_str(mouse_button button)
{
    ASSERT(button > BUTTON_UNKNOWN && button < BUTTON_COUNT, "Button code must be between 0 and BUTTON_COUNT.");

    static const char* button_strings[BUTTON_COUNT] = {
        [BUTTON_LEFT]     = "LEFT",    [BUTTON_RIGHT]    = "RIGHT",    [BUTTON_MIDDLE]   = "MIDDLE",
        [BUTTON_FORWARD]  = "FORWARD", [BUTTON_BACKWARD] = "BACKWARD",
    };

    if(button <= BUTTON_UNKNOWN || button >= BUTTON_COUNT || !button_strings[button])
    {
        return "UNKNOWN";
    }

    return button_strings[button];
}
