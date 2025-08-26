#include <application.h>

int main()
{
    application_config config = {0};
    config.window.title  = "Simple window";
    config.window.width  = 1024;
    config.window.height = 768;

    application_initialize(&config);
    application_run();
    application_shutdown();

    return 0;
}
