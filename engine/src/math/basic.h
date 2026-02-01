#pragma once

#include <math/constants.h>
#include <platform/math.h>

// ============================================================================
// Базовые тригонометрические функции
// ============================================================================

/*
    @brief Вычисляет синус угла.
    @param x Угол в радианах.
    @return Синус угла в диапазоне [-1.0, 1.0].
*/
INLINE f32 math_sin(const f32 x)
{
    return platform_math_sin(x);
}

/*
    @brief Вычисляет арксинус числа.
    @warning Поведение не определено при |x| > 1.0.
    @param x Число в диапазоне [-1.0, 1.0].
    @return Угол в радианах в диапазоне [-π/2, π/2].
*/
INLINE f32 math_asin(const f32 x)
{
    return platform_math_asin(x);
}

/*
    @brief Вычисляет косинус угла.
    @param x Угол в радианах.
    @return Косинус угла в диапазоне [-1.0, 1.0].
*/
INLINE f32 math_cos(const f32 x)
{
    return platform_math_cos(x);
}

/*
    @brief Вычисляет арккосинус числа.
    @warning Поведение не определено при |x| > 1.0.
    @param x Число в диапазоне [-1.0, 1.0].
    @return Угол в радианах в диапазоне [0, π].
*/
INLINE f32 math_acos(const f32 x)
{
    return platform_math_acos(x);
}

/*
    @brief Вычисляет тангенс угла.
    @warning Поведение не определено при x = π/2 + kπ (деление на ноль).
    @param x Угол в радианах. Не должен быть равен (π/2 + kπ), где k - целое число.
    @return Тангенс угла.
*/
INLINE f32 math_tan(const f32 x)
{
    return platform_math_tan(x);
}

/*
    @brief Вычисляет арктангенс числа.
    @note Используйте math_atan2 для получения угла во всех квадрантах.
    @param x Число в диапазоне [-INF, +INF].
    @return Угол в радианах в диапазоне (-π/2, π/2).
*/
INLINE f32 math_atan(const f32 x)
{
    return platform_math_atan(x);
}

/*
    @brief Вычисляет арктангенс отношения y/x с учетом квадранта.
    @warning Не передавать оба параметра равными 0.
    @note Корректно обрабатывает случай x = 0.
    @param y Координата Y (противоположная сторона).
    @param x Координата X (прилежащая сторона).
    @return Угол в радианах в диапазоне (-π, π].
*/
INLINE f32 math_atan2(const f32 y, const f32 x)
{
    return platform_math_atan2(y, x);
}

/*
    @brief Вычисляет косеканс угла (обратный синус).
    @warning Поведение не определено при sin(x) = 0 (деление на ноль).
    @param x Угол в радианах.
    @return Косеканс угла.
*/
INLINE f32 math_csc(const f32 x)
{
    return 1.0f / platform_math_sin(x);
}

/*
    @brief Вычисляет секанс угла (обратный косинус).
    @warning Поведение не определено при cos(x) = 0 (деление на ноль).
    @param x Угол в радианах.
    @return Секанс угла.
*/
INLINE f32 math_sec(const f32 x)
{
    return 1.0f / platform_math_cos(x);
}

/*
    @brief Вычисляет котангенс угла (обратный тангенс).
    @warning Поведение не определено при tan(x) = 0 (деление на ноль).
    @param x Угол в радианах.
    @return Котангенс угла.
*/
INLINE f32 math_ctg(const f32 x)
{
    return 1.0f / platform_math_tan(x);
}

/*
    @brief Вычисляет гиперболический синус.
    @note Вычисляется как (e^x - e^(-x)) / 2.
    @param x Аргумент.
    @return Гиперболический синус.
*/
INLINE f32 math_sinh(const f32 x)
{
    const f32 ex  = platform_math_pow(MATH_E, x);  // math_exp(x)
    const f32 emx = platform_math_pow(MATH_E, -x); // math_exp(-x)
    return (ex - emx) * 0.5f;
}

/*
    @brief Вычисляет гиперболический косинус.
    @note Вычисляется как (e^x + e^(-x)) / 2.
    @param x Аргумент.
    @return Гиперболический косинус.
*/
INLINE f32 math_cosh(const f32 x)
{
    const f32 ex  = platform_math_pow(MATH_E, x);  // math_exp(x)
    const f32 emx = platform_math_pow(MATH_E, -x); // math_exp(-x)
    return (ex + emx) * 0.5f;
}

/*
    @brief Вычисляет гиперболический тангенс.
    @warning Возможна потеря точности при больших |x|.
    @note Вычисляется как sinh(x) / cosh(x).
    @param x Аргумент.
    @return Гиперболический тангенс.
*/
INLINE f32 math_tanh(const f32 x)
{
    const f32 ex  = platform_math_pow(MATH_E, x);  // math_exp(x)
    const f32 emx = platform_math_pow(MATH_E, -x); // math_exp(-x)
    return (ex - emx) / (ex + emx);
}

/*
    @brief Вычисляет квадратный корень числа.
    @warning Поведение не определено при x < 0.0.
    @note Для отрицательных чисел использовать math_sqrt_abs.
    @param x Неотрицательное число.
    @return Квадратный корень числа.
*/
INLINE f32 math_sqrt(const f32 x)
{
    return platform_math_sqrt(x);
}

/*
    @brief Вычисляет квадратный корень числа.
    @param x Число (может быть отрицательным).
    @return Квадратный корень из |x|.
*/
INLINE f32 math_sqrt_abs(const f32 x)
{
    return platform_math_sqrt(platform_math_absf(x));
}

/*
    @brief Вычисляет обратный квадратный корень числа.
    @warning Поведение не определено при x <= 0.0.
    @param x Положительное число.
    @return Обратный квадратный корень.
*/
INLINE f32 math_inv_sqrt(const f32 x)
{
    return 1.0f / platform_math_sqrt(x);
}

/*
    @brief Вычисляет абсолютное значение числа (модуль).
    @param x Число с плавающей точкой.
    @return Абсолютное значение числа (|x|).
*/
INLINE f32 math_abs(const f32 x)
{
    return platform_math_absf(x);
}

/*
    @brief Округляет число вниз до ближайшего целого.
    @note Отрицательные числа округляются в сторону уменьшения (-3.7 → -4).
    @param x Число с плавающей точкой.
    @return Наибольшее целое число, меньшее или равное x.
*/
INLINE f32 math_floor(const f32 x)
{
    return platform_math_floor(x);
}

/*
    @brief Округляет число вверх до ближайшего целого.
    @note Отрицательные числа округляются в сторону увеличения (-3.2 → -3).
    @param x Число с плавающей точкой.
    @return Наименьшее целое число, большее или равное x.
*/
INLINE f32 math_ceil(const f32 x)
{
    return platform_math_ceil(x);
}

/*
    @brief Округляет число до ближайшего целого (банковское округление).
    @note Половинные значения округляются до ближайшего четного числа.
    @param x Число с плавающей точкой.
    @return Ближайшее целое число к x.
*/
INLINE f32 math_round(const f32 x)
{
    return platform_math_floor(x + 0.5f);
}

/*
    @brief Отбрасывает дробную часть числа.
    @note Для положительных чисел эквивалентно floor, для отрицательных - ceil.
    @param x Число с плавающей точкой.
    @return Целая часть числа.
*/
INLINE f32 math_trunc(const f32 x)
{
    return (x >= 0.0f) ? platform_math_floor(x) : platform_math_ceil(x);
}

/*
    @brief Возвращает дробную часть числа.
    @note Результат всегда неотрицательный.
    @param x Число с плавающей точкой.
    @return Дробная часть в диапазоне [0, 1).
*/
INLINE f32 math_fract(const f32 x)
{
    return x - platform_math_floor(x);
}

/*
    @brief Вычисляет остаток от деления.
    @warning Поведение не определено при y = 0.
    @note Результат имеет тот же знак, что и x.
    @param x Делимое.
    @param y Делитель.
    @return Остаток от деления x на y.
*/
INLINE f32 math_mod(const f32 x, const f32 y)
{
    return x - y * platform_math_floor(x / y);
}

/*
    @brief Вычисляет двоичный логарифм числа (логарифм по основанию 2).
    @warning Поведение не определено при x <= 0.0.
    @param x Положительное число.
    @return Логарифм числа x по основанию 2.
*/
INLINE f32 math_log2(const f32 x)
{
    return platform_math_log2(x);
}

/*
    @brief Вычисляет десятичный логарифм числа (логарифм по основанию 10).
    @warning Поведение не определено при x <= 0.0.
    @param x Положительное число.
    @return Логарифм числа x по основанию 10.
*/
INLINE f32 math_log10(const f32 x)
{
    return platform_math_log2(x) * MATH_LOG10_E;
}

/*
    @brief Вычисляет натуральный логарифм числа (логарифм по основанию e).
    @warning Поведение не определено при x <= 0.0.
    @param x Положительное число.
    @return Натуральный логарифм числа x.
*/
INLINE f32 math_ln(const f32 x)
{
    return platform_math_log2(x) * MATH_LN_2;
}

/*
    @brief Вычисляет экспоненту числа (e в степени x).
    @note Использует тождество: e^x = 2^(x * log2(e)).
    @param x Показатель степени.
    @return Значение экспоненты e^x.
*/
INLINE f32 math_exp(const f32 x)
{
    return platform_math_pow(MATH_E, x);
}

/*
    @brief Возводит число в заданную степень.
    @warning Поведение не определено при base < 0.0 и нецелом exponent, возможна потеря точности при больших exponent.
    @param base Основание степени.
    @param exponent Показатель степени.
    @return Результат возведения base в степень exponent.
*/
INLINE f32 math_pow(const f32 base, const f32 exponent)
{
    return platform_math_pow(base, exponent);
}

/*
    @brief Линейная интерполяция между двумя значениями.
    @warning t не ограничивается диапазоном [0, 1] (можно использовать для экстраполяции).
    @note При t=0 возвращает a, при t=1 возвращает b.
    @param a Начальное значение.
    @param b Конечное значение.
    @param t Параметр интерполяции в диапазоне [0, 1].
    @return Интерполированное значение.
*/
INLINE f32 math_lerp(const f32 a, const f32 b, const f32 t)
{
    return a + (b - a) * t;
}

/*
    @brief Обратная линейная интерполяция.
    @warning Поведение не определено при a = b (деление на ноль).
    @param a Начальное значение диапазона.
    @param b Конечное значение диапазона.
    @param value Значение внутри диапазона [a, b].
    @return Параметр t в диапазоне [0, 1], соответствующий value.
*/
INLINE f32 math_inverse_lerp(const f32 a, const f32 b, const f32 value)
{
    return (value - a) / (b - a);
}

// ============================================================================
// Вспомогательные функции
// ============================================================================

/*
    @brief Преобразует градусы в радианы.
    @param degrees Угол в градусах.
    @return Угол в радианах.
*/
INLINE f32 math_deg_to_rad(const f32 degrees)
{
    return degrees * MATH_DEG_TO_RAD;
}

/*
    @brief Преобразует радианы в градусы.
    @param radians Угол в радианах.
    @return Угол в градусах.
*/
INLINE f32 math_rad_to_deg(const f32 radians)
{
    return radians * MATH_RAD_TO_DEG;
}

// @brief Типичные диапазоны для нормализации углов.
typedef enum angle_range {
    // @brief Диапазон (-π, π]. Основной (интерполяция вращения, управление камерой, скелетная анимация, ...).
    ANGLE_RANGE_MINUS_PI_TO_PI,
    // @brief Диапазон [0, 2π). Дополнительный (векторые углы, uv-координаты, циклические системы, ...).
    ANGLE_RANGE_ZERO_TO_2PI,
} angle_range;

/*
    @brief Нормализует угол в заданный диапазон.
    @warning Для безопасности возвращает 0 при неизвестном range.
    @note При очень больших значениях angle > 1e6 может привести к потере точности.
    @param angle Угол в радианах.
    @param range Целевой диапазон нормализации.
    @return Нормализованный угол в указанном диапазоне.
*/
INLINE f32 math_angle_normalize(const f32 angle, angle_range range)
{
    switch(range)
    {
        case ANGLE_RANGE_MINUS_PI_TO_PI:
            return angle - MATH_2PI * platform_math_floor((angle + MATH_PI) * MATH_INV_2PI);
        case ANGLE_RANGE_ZERO_TO_2PI:
            return angle - MATH_2PI * platform_math_floor(angle * MATH_INV_2PI);
        default:
            // Никогда не должно происходить.
            return 0.0f;
    }
}

/*
    @brief Возвращает абсолютное значение с сохранением знака.
    @param value Исходное значение.
    @param sign Знак (положительный или отрицательный).
    @return Абсолютное значение с заданным знаком.
*/
INLINE f32 math_sign(const f32 value)
{
    if(value == 0.0f)
    {
        return 0.0f;
    }

    return (value > 0.0f) ? 1.0f : -1.0f;
}
