#include <platform/console.h>

int main()
{

    // Вывод тестовых сообщений для всех цветов
    platform_console_write(CONSOLE_COLOR_DEFAULT, "Test message 0 (Default)\n");
    platform_console_write(CONSOLE_COLOR_RED,     "Test message 1 (Light Red) (Error)\n");
    platform_console_write(CONSOLE_COLOR_ORANGE,  "Test message 2 (Light Orange)\n");
    platform_console_write(CONSOLE_COLOR_GREEN,   "Test message 3 (Light Green) (Information)\n");
    platform_console_write(CONSOLE_COLOR_YELLOW,  "Test message 4 (Light Yellow) (Warning)\n");
    platform_console_write(CONSOLE_COLOR_BLUE,    "Test message 5 (Light Blue) (Debug)\n");
    platform_console_write(CONSOLE_COLOR_MAGENTA, "Test message 6 (Light Magenta) (Fatal/Assertion)\n");
    platform_console_write(CONSOLE_COLOR_CYAN,    "Test message 7 (Light Cyan)\n");
    platform_console_write(CONSOLE_COLOR_WHITE,   "Test message 8 (White) (Trace)\n");
    platform_console_write(CONSOLE_COLOR_GRAY,    "Test message 9 (Gray)\n");

    return 0;
}
