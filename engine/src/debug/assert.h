#pragma once

#include <core/defines.h>
#include <core/logger.h>

/*
    @brief Макрос для программной остановки в отладчике (debug breakpoint).
    
    Стратегия выбора реализации:
    1. Приоритет: современные встроенные функции компилятора
    2. Запасной вариант: компилятор-специфичные методы
    3. Крайний случай: ассемблерная вставка (x86)
    
    Особенности:
    - Поддерживает Clang, GCC, MSVC и другие компиляторы
    - Автоматически выбирает оптимальный метод для платформы
*/

#if !defined(DEBUG_BREAK)
    // Современные компиляторы с поддержкой __has_builtin.
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_debugtrap)
            #define DEBUG_BREAK() __builtin_debugtrap()
        #elif __has_builtin(__debugbreak)
            #define DEBUG_BREAK() __debugbreak()
        #endif
    #endif

    // Запасные варианты для компиляторов без __has_builtin.
    #if !defined(DEBUG_BREAK)
        #if COMPILER_CLANG_FLAG
            #define DEBUG_BREAK() __builtin_trap()
        #elif COMPILER_MSC_FLAG
            #include <intrin.h>
            #define DEBUG_BREAK() __debugbreak()
        #else
            #define DEBUG_BREAK() asm volatile("int $0x03")
        #endif
    #endif
#endif

/*
    @brief Проверка утверждения с остановкой программы при ошибке.
    @note Если выражение ложно: выводит фатальную ошибку с текстом условия, сообщением, файлом и строкой
          и останавливает выполнение в отладчике (если он подключен) или аварийно завершает программу.
    @param expr Проверяемое выражение (должно быть true в нормальной ситуации).
    @param message Сообщение об ошибке для вывода в лог.
*/
#define ASSERT(expr, message)                                                    \
do {                                                                             \
    if(!(expr))                                                                  \
    {                                                                            \
        LOG_FATAL("Assertion failed: '%s' with message: '%s'.", #expr, message); \
        DEBUG_BREAK();                                                           \
    }                                                                            \
} while(false)
