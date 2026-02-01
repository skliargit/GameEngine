/*
    @file defines.h
    @brief Базовые определения платформы, компилятора и типов данных.
    @author Дмитрий Скляр.
    @version 1.1
    @date 28-01-2026

    @license Лицензия Apache, версия 2.0 («Лицензия»);
          Вы не имеете права использовать этот файл без соблюдения условий Лицензии.
          Копию Лицензии можно получить по адресу http://www.apache.org/licenses/LICENSE-2.0
          Если иное не предусмотрено действующим законодательством или не согласовано в письменной форме,
          программное обеспечение, распространяемое по Лицензии, распространяется на условиях «КАК ЕСТЬ»,
          БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ ИЛИ УСЛОВИЙ, явных или подразумеваемых. См. Лицензию для получения
          информации о конкретных языках, регулирующих разрешения и ограничения по Лицензии.

    @note Предоставляет:
            - Определения платформы (Windows/Linux)
            - Определения компилятора (MSVC/Clang/GCC)
            - Базовые типы данных фиксированного размера
            - Макросы для работы с памятью и выравнивания
            - Макросы для манипуляции с указателями
            - Вспомогательные макросы и утилиты
            - Статические проверки размеров типов

    @note Файл должен быть первым в цепочке включения заголовочных файлов.
          Не требует предварительной инициализации систем и подсистем.
*/

#pragma once

// ============================================================================
// Определение компилятора
// ============================================================================

#if !defined(COMPILER_MSC_FLAG) && !defined(COMPILER_CLANG_FLAG) && !defined(COMPILER_GCC_FLAG)
    #if defined(_MSC_VER)
        #define COMPILER_MSC_FLAG   1
    #elif defined(__clang__)
        #define COMPILER_CLANG_FLAG 1
    #elif defined(__GNUC__)
        #define COMPILER_GCC_FLAG   1
    #else
        #error "Unknown or unsupported compiler"
    #endif
#endif

// ============================================================================
// Определение платформы и ее разрядности
// ============================================================================

#if !defined(PLATFORM_LINUX_FLAG) && !defined(PLATFORM_WINDOWS_FLAG)
    #if defined(__linux__) || defined(__gnu_linux__)
        #define PLATFORM_LINUX_FLAG   1
    #elif defined(_WIN32) || defined(_WIN64)
        #define PLATFORM_WINDOWS_FLAG 1
    #else
        #error "Unknown or unsupported platform"
    #endif
#endif

#if !defined(PLATFORM_32BIT_FLAG) && !defined(PLATFORM_64BIT_FLAG)
    #if defined(__x86_64__) || defined(__LP64__) || defined(_WIN64) || defined(_M_X64)
        #define PLATFORM_64BIT_FLAG 1
        #define PLATFORM_BIT_DEPTH  64
    #elif defined(__i386__) || defined(__ILP32__) || defined(_WIN32) || defined(_M_IX86)
        #define PLATFORM_32BIT_FLAG 1
        #define PLATFORM_BIT_DEPTH  32
    #else
        #error "Cannot determine platform bit depth"
    #endif
#endif

// ============================================================================
// Базовые макросы для компиляции
// ============================================================================

// Макрос объявления функции с C-линковкой.
#if defined(__cplusplus)
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C
#endif

// Макрос для объявления импортируемых/экспортируемых функций.
// Для библиотек: определите LIB_EXPORT_FLAG при компиляции библиотеки.
#if defined(LIB_EXPORT_FLAG)
    #if defined(COMPILER_MSC_FLAG)
        #define API EXTERN_C __declspec(dllexport)
    #elif defined(COMPILER_CLANG_FLAG) || defined(COMPILER_GCC_FLAG)
        #define API EXTERN_C __attribute__((visibility("default")))
    #endif
#else
    #if defined(COMPILER_MSC_FLAG)
        #define API EXTERN_C __declspec(dllimport)
    #elif defined(COMPILER_CLANG_FLAG) || defined(COMPILER_GCC_FLAG)
        #define API EXTERN_C
    #endif
#endif

// Макросы для принудительного встраивания функции.
#if defined(COMPILER_MSC_FLAG)
    #define INLINE   static __forceinline
    #define NOINLINE __declspec(noinline)
#elif defined(COMPILER_CLANG_FLAG) || defined(COMPILER_GCC_FLAG)
    #define INLINE   static __attribute__((always_inline)) inline
    #define NOINLINE __attribute__((noinline))
#endif

// Макрос для указания устаревших частей кода.
#if defined(COMPILER_CLANG_FLAG) || defined(COMPILER_GCC_FLAG)
    /*
        @brief Помечате элемент как устаревший с сообщением.
        @param msg Сообщение об устаревании и рекомендации по замене.
    */
    #define DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(COMPILER_MSC_FLAG)
    /*
        @brief Помечате элемент как устаревший с сообщением.
        @param msg Сообщение об устаревании и рекомендации по замене.
    */
    #define DEPRECATED(msg) __declspec(deprecated(msg))
#else
    /*
        @brief Помечате элемент как устаревший с сообщением.
        @param msg Сообщение об устаревании и рекомендации по замене.
    */
    #define DEPRECATED(msg)
#endif

// Макросы для подсказки оптимизатору о вероятности ветвления.
#if defined(COMPILER_CLANG_FLAG) || defined(COMPILER_GCC_FLAG)
    // @brief Указывает, что условие, скорее всего, истинно.
    #define LIKELY(expr)   __builtin_expect(!!(expr), 1)
    // @brief Указывает, что условие, скорее всего, ложно.
    #define UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
    // @brief Указывает, что условие, скорее всего, истинно.
    #define LIKELY(expr)   (expr)
    // @brief Указывает, что условие, скорее всего, ложно.
    #define UNLIKELY(expr) (expr)
#endif

// Макрос для программной остановки в отладчике (debug breakpoint).
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
        #if defined(COMPILER_CLANG_FLAG)
            #define DEBUG_BREAK() __builtin_trap()
        #elif defined(COMPILER_MSC_FLAG)
            #include <intrin.h>
            #define DEBUG_BREAK() __debugbreak()
        #else
            #define DEBUG_BREAK() asm volatile("int $0x03")
        #endif
    #endif
#endif

// Макрос для выполнения статической провеки на этапе компиляции.
#if defined(__cplusplus)
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

// ============================================================================
// Фиксированные целочисленные типы (одинаковые на всех платформах)
// ============================================================================

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

// ============================================================================
// Типы с плавающей точкой (одинаковые на всех платформах)
// ============================================================================

// @brief 32-битное число c плавающей точкой.
typedef float f32;

// @brief 64-битное число с плавающей точкой.
typedef double f64;

// ============================================================================
// Платформенно-зависимые типы размера указателя
// ============================================================================

// Linux (POSIX) стандарт.
#if defined(PLATFORM_LINUX_FLAG)

    // LP64, где long = 8 байт, pointer = 8 байт.
    #if defined(PLATFORM_64BIT_FLAG)
        // @brief Беззнаковое целое, используется для размера памяти и смещения.
        typedef unsigned long usize;
        // @brief Знаковое целое, используется для смещения.
        typedef signed long isize;

    // ILP32, где int = 4 байта, long = 4 байта, pointer = 4 байта.
    #else
        // @brief Беззнаковое целое, используется для размера памяти и смещения.
        typedef unsigned int usize;
        // @brief Знаковое целое, используется для смещения.
        typedef signed int isize;
    #endif

// Windows стандарт.
#elif PLATFORM_WINDOWS_FLAG

    // LLP64, где long long = 8 байт, pointer = 8 байт, long = 4 байта.
    #if defined(PLATFORM_64BIT_FLAG)
        // @brief Беззнаковое целое, используется для размера памяти и смещения.
        typedef unsigned long long usize;
        // @brief Знаковое целое, используется для смещения.
        typedef signed long long isize;

    // ILP32, где int = 4 байта, long = 4 байта, pointer = 4 байта.
    #else
        // @brief Беззнаковое целое, используется для размера памяти и смещения.
        typedef unsigned int usize;
        // @brief Знаковое целое, используется для смещения.
        typedef signed int isize;
    #endif

#endif

// ============================================================================
// Другие типы
// ============================================================================

// Определение байтового типа.
#if defined(__cplusplus) && (__cplusplus >= 201703L)
    // @brief Байтовый тип (аналог std::byte в C++).
    using byte = unsigned char;
#else
    // @brief Байтовый тип.
    typedef unsigned char byte;
#endif

// Определение логического типа.
#if !defined(__cplusplus) && defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
    // @brief Логический тип.
    typedef _Bool bool;
#else
    // @brief Логический тип.
    typedef u8 bool;
#endif

// @brief Определяет диапазон памяти (зависит от платформы).
typedef struct range {
    // @brief Смещение.
    usize offset;
    // @brief Размер.
    usize size;
} range;

// ============================================================================
// Константы для целочисленных типов (одинаковые на всех платформах)
// ============================================================================

// Максимальные беззнаковые значения.
#define U8_MAX  ((u8)-1)
#define U16_MAX ((u16)-1)
#define U32_MAX ((u32)-1)
#define U64_MAX ((u64)-1)

// Максимальные знаковые значения.
#define I8_MAX  ((i8)(U8_MAX >> 1))
#define I16_MAX ((i16)(U16_MAX >> 1))
#define I32_MAX ((i32)(U32_MAX >> 1))
#define I64_MAX ((i64)(U64_MAX >> 1))

// Минимальные знаковые значения.
#define I8_MIN  (-I8_MAX  - 1)
#define I16_MIN (-I16_MAX - 1)
#define I32_MIN (-I32_MAX - 1)
#define I64_MIN (-I64_MAX - 1)

// Недействительные идентификаторы.
#define INVALID_ID8  U8_MAX
#define INVALID_ID16 U16_MAX
#define INVALID_ID32 U32_MAX
#define INVALID_ID64 U64_MAX

// ============================================================================
// Константы для целочисленных типов (платформенно-зависимые)
// ============================================================================

// Константы для usize/isize (зависят от платформы).
#define USIZE_MAX ((usize)-1)
#define ISIZE_MAX ((isize)(USIZE_MAX >> 1))
#define ISIZE_MIN (-ISIZE_MAX - 1)

// Недействительные идентификаторы.
#define INVALID_ID_USIZE USIZE_MAX

// ============================================================================
// Константы для чисел с плавающей точкой (IEEE 754)
// ============================================================================

// Максимальное конечное положительное значение для чисел с плавающей точкой.
#define F32_POSITIVE_MAX 3.402823466e+38f
#define F64_POSITIVE_MAX 1.7976931348623157e+308

// Минимальное положительное нормализованное значение для чисел с плавающей точкой.
#define F32_POSITIVE_MIN 1.175494351e-38f
#define F64_POSITIVE_MIN 2.2250738585072014e-308

// Наименьшее положительное нормализованное значение для чисел с плавающей точкой (полезно для алгоритмов, где важна каждая степень).
#define F32_TRUE_MIN 1.401298464e-45f
#define F64_TRUE_MIN 4.9406564584124654e-324

// Максимальный положительный порог для чисел с плавающей точкой.
#define F32_RANGE_MAX (F32_POSITIVE_MAX)
#define F64_RANGE_MAX (F64_POSITIVE_MAX)

// Максимальный отрицательный порог для чисел с плавающей точкой.
#define F32_RANGE_MIN (-F32_POSITIVE_MAX)
#define F64_RANGE_MIN (-F64_POSITIVE_MAX)

// Машинный эпсилон для чисел с плавающей точкой, такое что 1.0 + EPSILON != 1.0 (ТОЛЬКО ДЛЯ ПРОВЕРОК ОКОЛО 1.0).
// !!! НЕ используйте для сравнения чисел и для сравнения с нулем !!!
#define F32_EPSILON 1.192092896e-07f
#define F64_EPSILON 2.2204460492503131e-16

// Эпсилон для сравнения ЛЮБЫХ ЧИСЕЛ с плавающей точкой.
// Используйте для сравнения: abs(a - b) <= EPSILON_CMP.
#define F32_EPSILON_CMP 1e-6f
#define F64_EPSILON_CMP 1e-12

// Порог для сравнения с нулем для чисел с плавающей точкой.
// Используйте для проверки: abs(value) <= ZERO_THRESHOLD.
#define F32_ZERO_THRESHOLD 1e-12f
#define F64_ZERO_THRESHOLD 1e-14

// Специальные значения для чисел с плавающей точкой.
// Используйте для сравнения: value != value (только это сравнение работает).
#define F32_NAN     ((f32)(0.0f / 0.0f))
#define F32_INF     ((f32)(1.0f / 0.0f))
#define F32_NEG_INF (-F32_INF)

#define F64_NAN     ((f64)(0.0 / 0.0))
#define F64_INF     ((f64)(1.0 / 0.0))
#define F64_NEG_INF (-F64_INF)

// Максимальное количество значащих десятичных цифр для чисел с плавающей точкой.
#define F32_DIG 6
#define F64_DIG 15

// ============================================================================
// Константы для других типов
// ============================================================================

// Определение логических значений.
#ifndef __cplusplus
    // @brief Логическое значение "ложь".
    #define false 0
    // @brief Логическое значение "истина".
    #define true  1
#endif

// Определение значения недействительного указателя.
#ifndef __cplusplus
    // @brief Недействительный указатель.
    #define nullptr ((void*)0)
#endif

// ============================================================================
// Макросы преобразования типов без проверки диапазонов
// ============================================================================

/*
    @brief Простое приведение к u8 (без проверки диапазона).
    @warning Может вызывать неопределенное поведение при переполнении.
    @param value Значение для приведения.
    @return Приведенное значение.
*/
#define CAST_U8(value) ((u8)(value))

/*
    @brief Простое приведение к u16 (без проверки диапазона).
    @warning Может вызывать неопределенное поведение при переполнении.
    @param value Значение для приведения.
    @return Приведенное значение.
*/
#define CAST_U16(value) ((u16)(value))

/*
    @brief Простое приведение к u32 (без проверки диапазона).
    @warning Может вызывать неопределенное поведение при переполнении.
    @param value Значение для приведения.
    @return Приведенное значение.
*/
#define CAST_U32(value) ((u32)(value))

/*
    @brief Простое приведение к u64 (без проверки диапазона).
    @warning Может вызывать неопределенное поведение при переполнении.
    @param value Значение для приведения.
    @return Приведенное значение.
*/
#define CAST_U64(value) ((u64)(value))

/*
    @brief Простое приведение к i8 (без проверки диапазона).
    @warning Может вызывать неопределенное поведение при переполнении.
    @param value Значение для приведения.
    @return Приведенное значение.
*/
#define CAST_I8(value) ((i8)(value))

/*
    @brief Простое приведение к i16 (без проверки диапазона).
    @warning Может вызывать неопределенное поведение при переполнении.
    @param value Значение для приведения.
    @return Приведенное значение.
*/
#define CAST_I16(value) ((i16)(value))

/*
    @brief Простое приведение к i32 (без проверки диапазона).
    @warning Может вызывать неопределенное поведение при переполнении.
    @param value Значение для приведения.
    @return Приведенное значение.
*/
#define CAST_I32(value) ((i32)(value))

/*
    @brief Простое приведение к i64 (без проверки диапазона).
    @warning Может вызывать неопределенное поведение при переполнении.
    @param value Значение для приведения.
    @return Приведенное значение.
*/
#define CAST_I64(value) ((i64)(value))

/*
    @brief Простое приведение к f32 (без проверки диапазона).
    @warning Может вызывать потерю точности или переполнение.
    @param value Значение для приведения.
    @return Приведенное значение.
*/
#define CAST_F32(value) ((f32)(value))

/*
    @brief Простое приведение к f64 (без проверки диапазона).
    @param value Значение для приведения.
    @return Приведенное значение.
*/
#define CAST_F64(value) ((f64)(value))

/*
    @brief Простое приведение к usize (без проверки диапазона).
    @warning Может вызывать неопределенное поведение при переполнении.
    @param value Значение для приведения.
    @return Приведенное значение.
*/
#define CAST_USIZE(value) ((usize)(value))

/*
    @brief Простое приведение к isize (без проверки диапазона).
    @warning Может вызывать неопределенное поведение при переполнении.
    @param value Значение для приведения.
    @return Приведенное значение.
*/
#define CAST_ISIZE(value) ((isize)(value))

// ============================================================================
// Макросы для работы с размерами памяти
// ============================================================================

/*
    @brief Конвертирует кибибайты в байты (стандарт МЭК, 1024 байта).
    @param n Количество кибибайт.
    @return Количество байт.
*/
#define KIBIBYTES(n) ((usize)(n) * 1024ULL)

/*
    @brief Конвертирует мебибайты в байты (стандарт МЭК, 1024*1024 байт).
    @param n Количество мебибайт.
    @return Количество байт.
*/
#define MEBIBYTES(n) ((usize)(n) * 1024ULL * 1024ULL)

/*
    @brief Конвертирует гибибайты в байты (стандарт МЭК, 1024*1024*1024 байт).
    @param n Количество гибибайт.
    @return Количество байт.
*/
#define GIBIBYTES(n) ((usize)(n) * 1024ULL * 1024ULL * 1024ULL)

/*
    @brief Конвертирует килобайты в байты (стандарт СИ, 1000 байт).
    @param n Количество килобайт.
    @return Количество байт.
*/
#define KILOBYTES(n) ((usize)(n) * 1000ULL)

/*
    @brief Конвертирует мегабайты в байты (стандарт СИ, 1000*1000 байт).
    @param n Количество мегабайт.
    @return Количество байт.
*/
#define MEGABYTES(n) ((usize)(n) * 1000ULL * 1000ULL)

/*
    @brief Конвертирует гигабайты в байты (стандарт СИ, 1000*1000*1000 байт).
    @param n Количество гигабайт.
    @return Количество байт.
*/
#define GIGABYTES(n) ((usize)(n) * 1000ULL * 1000ULL * 1000ULL)

// ============================================================================
// Макросы для работы с указателями и памятью
// ============================================================================

/*
    @brief Добавляет смещение к указателю.
    @param ptr Исходный указатель.
    @param offset Смещение в байтах.
    @return Указатель с добавленным смещением.
*/
#define POINTER_ADD_OFFSET(ptr, offset) ((void*)((byte*)(ptr) + (offset)))

/*
    @brief Вычитает смещение из указателя.
    @param ptr Исходный указатель.
    @param offset Смещение в байтах.
    @return Указатель с вычтенным смещением.
*/
#define POINTER_SUB_OFFSET(ptr, offset) ((void*)((byte*)(ptr) - (offset)))

/*
    @brief Проверяет, выровнен ли указатель на заданную границу.
    @note alignment должно быть степенью двойки: 1, 2, 4, 8, 16, ...
    @param ptr Проверяемый указатель.
    @param alignment Ожидаемое выравнивание (должно быть степенью двойки).
    @return true если указатель выровнен, иначе false.
*/
#define POINTER_IS_ALIGNED(ptr, alignment) (((usize)(ptr) & ((usize)(alignment) - 1)) == 0)

/*
    @brief Получает выравнивание указателя вверх до заданной границы.
    @note Пример: POINTER_ALIGN_UP(0x1003, 16) = 0x1010.
    @param ptr Исходный указатель для выравнивания.
    @param alignment Требуемое выравнивание (степень двойки).
    @return Выравненный указатель (больший или равный исходному).
*/
#define POINTER_ALIGN_UP(ptr, alignment) ((void*)(((usize)(ptr) + ((usize)(alignment) - 1)) & ~((usize)(alignment) - 1)))

/*
    @brief Получает выравнивание указателя вниз до заданной границы.
    @note Пример: POINTER_ALIGN_DOWN(0x100F, 16) = 0x1000.
    @param ptr Исходный указатель для выравнивания.
    @param alignment Требуемое выравнивание (степень двойки).
    @return Выравненный указатель (меньший или равный исходному).
*/
#define POINTER_ALIGN_DOWN(ptr, alignment) ((void*)((usize)(ptr) & ~((usize)(alignment) - 1)))

/*
    @brief Получает значение члена структуры через указатель.
    @param type Тип структуры.
    @param pointer Указатель на структуру.
    @param member Имя члена структуры.
    @return Значение члена структуры.
*/
#define MEMBER_GET_VALUE(type, pointer, member) (((type*)(pointer))->member)

/*
    @brief Вычисляет смещение члена в структуре.
    @param type Тип структуры.
    @param member Имя члена структуры.
    @return Смещение члена в байтах от начала структуры.
*/
#define OFFSET_OF(type, member) ((usize)&(((type*)0)->member))

/*
    @brief Вычисляет размер массива во время компиляции.
    @param array Массив (не указатель!).
    @return Количество элементов в массиве.
*/
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

/*
    @brief Проверяет, является ли значение степенью двойки.
    @note Число 0 не считается степенью двойки.
    @param value Число для проверки.
    @return true если value > 0 и является степенью двойки, иначе false.
*/
#define IS_POWER_OF_TWO(value) (((value) > 0) && (((value) & ((value) - 1)) == 0))

/*
    @brief Возвращает ближайшую степень двойки, не менее заданного числа.
    @warning Поведение не определено при value <= 0.
    @param value Положительное число (u8, u16, u32, u64).
    @return Наименьшая степень двойки, не менее заданного числа.
*/
#define NEXT_POWER_OF_TWO(value)             \
    ({                                       \
        __typeof__(value) _result = (value); \
        _result--;                           \
        _result |= _result >> 1;             \
        _result |= _result >> 2;             \
        _result |= _result >> 4;             \
        _result |= _result >> 8;             \
        _result |= _result >> 16;            \
        _result |= _result >> 32;            \
        _result++;                           \
        _result;                             \
    })

// ============================================================================
// Вспомогательные макросы
// ============================================================================

/*
    @brief Возвращает минимальное из двух значений.
    @param a Первое значение.
    @param b Второе значение.
    @return Минимальное значение.
*/
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/*
    @brief Возвращает максимальное из двух значений.
    @param a Первое значение.
    @param b Второе значение.
    @return Максимальное значение.
*/
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/*
    @brief Ограничивает значение указанным диапазоном.
    @param value Исходное значение.
    @param min Нижняя граница диапазона.
    @param max Верхняя граница диапазона.
    @return Ограниченное значение в диапазоне [min, max].
*/
#define CLAMP(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))

/*
    @brief Линейно преобразует значение из одного диапазона в другой.
    @warning Поведение не определено при from_min == from_max (деление на ноль).
    @param value Исходное значение.
    @param from_min Минимальное значение исходного диапазона.
    @param from_max Максимальное значение исходного диапазона.
    @param to_min Минимальное значение целевого диапазона.
    @param to_max Максимальное значение целевого диапазона.
    @return Преобразованное значение.
*/
#define REMAP(value, from_min, from_max, to_min, to_max) \
    ((((value) - (from_min)) * ((to_max) - (to_min))) / ((from_max) - (from_min))) + (to_min)

/*
    @brief Обменивает значения двух переменных.
    @param a Первое значение для обмена.
    @param b Второе значение для обмена.
*/
#define SWAP(a, b) do {            \
        __typeof__(a) _temp = (a); \
        (a) = (b);                 \
        (b) = _temp;               \
    } while(false)

/*
    @brief Подавляет предупреждения о неиспользуемых переменных.
    @param x Переменная, которую нужно пометить как неиспользуемую.
*/
#define UNUSED(x) ((void)x)

// ============================================================================
// Проверка утверждений базовых типов
// ============================================================================

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
STATIC_ASSERT(sizeof(byte) == 1, "Size of byte must be 1 byte.");

// Проверка соответствия зависимых от платформы типов.
STATIC_ASSERT(sizeof(usize) == sizeof(void*), "Size of usize must be equal to address size.");
STATIC_ASSERT(sizeof(isize) == sizeof(void*), "Size of isize must be equal to address size.");

// Проверка что типы беззнаковые.
STATIC_ASSERT(U8_MAX    > 0, "u8 must be unsigned");
STATIC_ASSERT(U16_MAX   > 0, "u16 must be unsigned");
STATIC_ASSERT(U32_MAX   > 0, "u32 must be unsigned");
STATIC_ASSERT(U64_MAX   > 0, "u64 must be unsigned");
STATIC_ASSERT(USIZE_MAX > 0, "usize must be unsigned");
