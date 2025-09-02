/*
    @file defines.h
    @brief Базовые определения платформы, компилятора и типов данных.
    @author Дмитрий Скляр.
    @version 1.0
    @date 27-08-2025

    @license Лицензия Apache, версия 2.0 («Лицензия»);
          Вы не имеете права использовать этот файл без соблюдения условий Лицензии.
          Копию Лицензии можно получить по адресу http://www.apache.org/licenses/LICENSE-2.0
          Если иное не предусмотрено действующим законодательством или не согласовано в письменной форме,
          программное обеспечение, распространяемое по Лицензии, распространяется на условиях «КАК ЕСТЬ»,
          БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ ИЛИ УСЛОВИЙ, явных или подразумеваемых. См. Лицензию для получения
          информации о конкретных языках, регулирующих разрешения и ограничения по Лицензии.

    @note Предоставляет:
            - Определения платформы (Windows/Linux, 64-bit)
            - Определения компилятора (MSVC/Clang)
            - Базовые типы данных фиксированного размера
            - Макросы для работы с памятью и выравнивания
            - Вспомогательные макросы и утилиты
            - Статические проверки размеров типов

    @note Файл должен быть первым в цепочке включения заголовочных файлов.
          Не требует предварительной инициализации систем и подсистем.
*/

#pragma once

//--------------------------------------------------- Определения платформы ---------------------------------------------------

// Определение разрядности платформы.
#if defined __x86_64__ || defined __LP64__ || defined _WIN64 || defined _M_X64
    #define PLATFORM_64BIT_FLAG 1
#else
    #error "Unsupported system bit depth. 32-bit platforms are not supported."
#endif

// Определение операционной системы платформы (обычно на уровне makefile).
#if !defined PLATFORM_LINUX_FLAG && !defined PLATFORM_WINDOWS_FLAG
    // Linux платформа.
    #if defined __linux__ || defined __gnu_linux__
        #define PLATFORM_LINUX_FLAG   1
    // Windows платформа.
    #elif defined _WIN32 || defined _WIN64
        #define PLATFORM_WINDOWS_FLAG 1
    // Неизвестная платформа.
    #else
        #error "Unknown or unsupported platform."
    #endif
#endif

//-------------------------------------------------- Определения компилятора --------------------------------------------------

// Определение используемого компилятора.
#if defined _MSC_VER
    #define COMPILER_MSC_FLAG   1
#elif defined __clang__
    #define COMPILER_CLANG_FLAG 1
#else
    #error "Unknown or unsupported compiler."
#endif

// Определение квалификатора экспорта кода.
#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C
#endif

// Определение квалификатора экспорта/импорта функций.
#ifdef LIB_EXPORT_FLAG
    #if defined COMPILER_MSC_FLAG
        #define API EXTERN_C __declspec(dllexport)
    #elif defined COMPILER_CLANG_FLAG
        #define API EXTERN_C __attribute__((visibility("default")))
    #endif
#else
    #if defined COMPILER_MSC_FLAG
        #define API EXTERN_C __declspec(dllimport)
    #elif defined COMPILER_CLANG_FLAG
        #define API EXTERN_C
    #endif
#endif

// Определение квалификаторов INLINE/NOINLINE.
#if defined COMPILER_MSC_FLAG
    #define INLINE   static __forceinline
    #define NOINLINE __declspec(noinline)
#elif defined COMPILER_CLANG_FLAG
    #define INLINE   static __attribute__((always_inline)) inline
    #define NOINLINE __attribute__((noinline))
#endif

// Макрос для выполнения статической провеки на этапе компиляции.
#ifdef __cplusplus
    /*
        @brief Выполняет проверку утверждения во время компиляции, и выводит сообщение если оно ложно.
        @param assertion Проверяемое во время компиляции утверждение.
        @param message Сообщение которое будет выведено если утверждение ложно.
    */
    #define STATIC_ASSERT(assertion, message) static_assert(assertion, message)
#else
    /*
        @brief Выполняет проверку утверждения во время компиляции, и выводит сообщение если оно ложно.
        @param assertion Проверяемое во время компиляции утверждение.
        @param message Сообщение которое будет выведено если утверждение ложно.
    */
    #define STATIC_ASSERT(assertion, message) _Static_assert(assertion, message)
#endif

// Макрос для программной остановки в отладчике (debug breakpoint).
#ifndef DEBUG_BREAK
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
        #if defined COMPILER_CLANG_FLAG
            #define DEBUG_BREAK() __builtin_trap()
        #elif defined COMPILER_MSC_FLAG
            #include <intrin.h>
            #define DEBUG_BREAK() __debugbreak()
        #else
            #define DEBUG_BREAK() asm volatile("int $0x03")
        #endif
    #endif
#endif

// Макрос подсказок ветвления LIKELY/UNLIKELY для компилятора.
#if defined COMPILER_CLANG_FLAG
    // @brief Указывает, что условие, скорее всего, истинно.
    #define LIKELY(expr)   __builtin_expect(!!(expr), 1)
    // @brief Указывает, что условие, скорее всего, ложно.
    #define UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#elif defined COMPILER_MSC_FLAG
    #include <intrin.h>
    // @brief Указывает, что условие, скорее всего, истинно.
    #define LIKELY(expr)   (__assume(!!(expr)), (expr))
    // @brief Указывает, что условие, скорее всего, ложно.
    #define UNLIKELY(expr) (__assume(!(expr)), (expr))
#else
    // @brief Указывает, что условие, скорее всего, истинно.
    #define LIKELY(expr)   (expr)
    // @brief Указывает, что условие, скорее всего, ложно.
    #define UNLIKELY(expr) (expr)
#endif

//----------------------------------------------------- Определения типов -----------------------------------------------------

// @brief 8-битное беззнаковое целое.
typedef unsigned char u8;

// @brief 16-битное беззнаковое целое.
typedef unsigned short u16;

// @brief 32-битное беззнаковое целое.
typedef unsigned int u32;

// @brief 64-битное беззнаковое целое.
typedef unsigned long long u64;

// @brief 8-битное знаковое целое.
typedef signed char i8;

// @brief 16-битное знаковое целое.
typedef signed short i16;

// @brief 32-битное знаковое целое.
typedef signed int i32;

// @brief 64-битное знаковое целое.
typedef signed long long i64;

// @brief 32-битное число c плавающей точкой.
typedef float f32;

// @brief 64-битное число с плавающей точкой.
typedef double f64;

// Типы размер которых зависит от платформы.
#if defined PLATFORM_64BIT_FLAG && defined PLATFORM_LINUX_FLAG
    // @brief Беззнаковое целое, используется для размера памяти и смещения (соответствует разрядности текущей платформы).
    typedef unsigned long usize;
    // @brief Знаковое целое, используется для смещения (соответствует разрядности текущей платформы).
    typedef signed long isize;
#elif defined PLATFORM_64BIT_FLAG && defined PLATFORM_WINDOWS_FLAG
    // @brief Беззнаковое целое, используется для размера памяти и смещения (соответствует разрядности текущей платформы).
    typedef unsigned long long usize;
    // @brief Знаковое целое, используется для смещения (соответствует разрядности текущей платформы).
    typedef signed long long isize;
#else
    // @brief Беззнаковое целое, используется для размера памяти и смещения (соответствует разрядности текущей платформы).
    typedef unsigned int usize;
    // @brief Знаковое целое, используется для смещения (соответствует разрядности текущей платформы).
    typedef signed int isize;
#endif

// Определение логического типа.
#ifndef __cplusplus
    #if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
        // @brief Логический тип.
        typedef _Bool bool;
    #else
        // @brief Логический тип.
        typedef u8 bool;
    #endif
#endif

// @brief Определяет диапазон.
typedef struct range {
    // @brief Смещение.
    u64 offset;
    // @brief Размер.
    u64 size;
} range;

//--------------------------------------------------- Определения констант ----------------------------------------------------

// Определение логических значений.
#ifndef __cplusplus
    // @brief Логический '0'.
    #define false 0
    // @brief Логический '1'.
    #define true  1
#endif

// Определение значения недействительного указателя.
#ifndef __cplusplus
    // @brief Недействительный указатель.
    #define nullptr ((void*)0)
#endif

// Максимальные беззнаковые значения независимых от платформы типов.
#define U8_MAX  255U
#define U16_MAX 65535U
#define U32_MAX 4294967295U
#define U64_MAX 18446744073709551615ULL

// Максимальные знаковые значения независимых от платформы типов.
#define I8_MAX  127
#define I16_MAX 32767
#define I32_MAX 2147483647
#define I64_MAX 9223372036854775807LL

// Минимальные знаковые значения независимых от платформы типов.
#define I8_MIN  (-I8_MAX  - 1)
#define I16_MIN (-I16_MAX - 1)
#define I32_MIN (-I32_MAX - 1)
#define I64_MIN (-I64_MAX - 1)

// Максимальные и минимальные значения зависимых от платформы типов.
#if PLATFORM_64BIT_FLAG 
    #define USIZE_MAX U64_MAX
    #define ISIZE_MAX I64_MAX
    #define ISIZE_MIN I64_MIN
#else
    #define USIZE_MAX U32_MAX
    #define ISIZE_MAX I32_MAX
    #define ISIZE_MIN I32_MIN
#endif

// Недействительные значения данных.
#define INVALID_ID8  U8_MAX
#define INVALID_ID16 U16_MAX
#define INVALID_ID32 U32_MAX
#define INVALID_ID64 U64_MAX

/*
    @brief Макрос-множитель, который переводит гибибайты в байты (1024*1024*1024). Стандарт МЭК.
    @note  (1 GiB) - здесь пробел обязательный, а скобки нужны исключительно при использовании
           в математических операциях.
*/
#define GiB * (1024ULL * 1024ULL * 1024ULL)

/*
    @brief Макрос-множитель, который переводит мебибайты в байты (1024*1024). Стандарт МЭК.
    @note  (1 MiB) - здесь пробел обязательный, а скобки нужны исключительно при использовании
           в математических операциях.
*/
#define MiB * (1024ULL * 1024ULL)

/*
    @brief Макрос-множитель, который переводит кибибайты в байты (1024). Стандарт МЭК.
    @note  (1 KiB) - здесь пробел обязательный, а скобки нужны исключительно при использовании
           в математических операциях.
*/
#define KiB * (1024ULL)

/*
    @brief Макрос-множитель, который переводит гигабайты в байты (1000*1000*1000). Единица измерения СИ.
    @note  (1 Gb) - здесь пробел обязательный, а скобки нужны исключительно при использовании
           в математических операциях.
*/
#define Gb * (1000ULL * 1000ULL * 1000ULL)

/*
    @brief Макрос-множитель, который переводит мегабайты в байты (1000*1000). Единица измерения СИ.
    @note  (1 Mb) - здесь пробел обязательный, а скобки нужны исключительно при использовании
           в математических операциях.
*/
#define Mb * (1000ULL * 1000ULL)

/*
    @brief Макрос-множитель, который переводит килобайты в байты (1000). Единица измерения СИ.
    @note  (1 Kb) - здесь пробел обязательный, а скобки нужны исключительно при использовании
           в математических операциях.
*/
#define Kb * (1000ULL)

//-------------------------------------- Определения вспомогательных функций и макросов ---------------------------------------

/*
    @brief Макрос получения минимального значения из двух заданных.
    @param v0 Первое значение.
    @param v1 Второе значение.
    @return Минимальное значение.
*/
#define MIN(v0, v1) (((v0) < (v1)) ? (v0) : (v1))

/*
    @brief Макрос получения максимального значения из двух заданных.
    @param v0 Первое значение.
    @param v1 Второе значение.
    @return Максимальное значение.
*/
#define MAX(v0, v1) (((v0) > (v1)) ? (v0) : (v1))

/*
    @brief Макрос усечения значения в указанных пределах.
    @param value Значение, которое нужно усечь.
    @param min Значение минимального предела включительно.
    @param max Значение максимального предела включительно.
    @return Значение в заданных пределах.
*/
#define CLAMP(value, min, max) (((value) <= (min)) ? (min) : (((value) >= (max)) ? (max) : (value)))

/*
    @brief Макрос вычисляет новый указатель относительно заданного указателя и смещения.
    @param pointer Указатель для вычисления нового.
    @param offset Смещение в байтах.
    @return Новое значение указателя.
*/
#define POINTER_GET_OFFSET(pointer, offset) (void*)((u8*)pointer + (offset))

/*
    @brief Макрос получает значение поля структуры заданного типом и указателем.
    @param type Тип структуры.
    @param pointer Указатель на память (структуру).
    @param member Поле структуры значение которого нужно получить.
    @return Значение поля структуры.
*/
#define MEMBER_GET_VALUE(type, pointer, member) (((type*)(pointer))->member)

/*
    @brief Подавляет предупреждения о неиспользуемых переменных путем явного
           приведения к void (без overhead).
*/
#define UNUSED(x) ((void)x)

/*
    @brief Получает ближайшее кратное число заданному с округлением в большую сторону.
    @param value Число для которого необходимо получить ближайщее кратное.
    @param granularity Необходимая кратность числа (должно быть степенью двойки!).
    @return Ближайшее кратное число, относительно заданного числа.
*/
INLINE u64 get_aligned(u64 value, u64 granularity)
{
    return ((value + (granularity - 1)) & ~(granularity - 1));
}

/*
    @brief Получает ближайшие кратные смещения и размера с округлением в большую сторону.
    @param offset Смещение для которого необходимо получить ближайшее кратное.
    @param size Размер для которого необходимо получить ближайшее кратное.
    @param granularity Необходимая кратность чисел (должно быть степенью двойки!).
    @return Диапазон с выровненными значениями к ближайшим кратным числам, относительно заданных чисел.
*/
INLINE range get_aligned_range(u64 offset, u64 size, u64 granularity)
{
    return (range){get_aligned(offset, granularity), get_aligned(size, granularity)};
}

//------------------------------------------- Проверока утвержений базовых типов ----------------------------------------------

// Проверка соответствия независимых от платформы типов.
STATIC_ASSERT(sizeof(u8)  == 1, "Size of u8 must be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Size of u16 must be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Size of u32 must be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Size of u64 must be 8 bytes.");
STATIC_ASSERT(sizeof(i8)  == 1, "Size of i8 must be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Size of i16 must be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Size of i32 must be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Size of i64 must be 8 bytes.");
STATIC_ASSERT(sizeof(f32) == 4, "Size of f32 must be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Size of f64 must be 8 bytes.");

// Проверка соответствия зависимых от платформы типов.
STATIC_ASSERT(sizeof(usize) == sizeof(void*), "Size of usize must be equal to address size.");
STATIC_ASSERT(sizeof(isize) == sizeof(void*), "Size of isize must be equal to address size.");
