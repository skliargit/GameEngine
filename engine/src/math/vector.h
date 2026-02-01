#pragma once

#include <math/types.h>
#include <math/constants.h>
#include <math/basic.h>

// ============================================================================
// Двумерные векторы (vec2)
// ============================================================================

/*
    @brief Создает новый 2D вектор.
    @param x Координата по оси X.
    @param y Координата по оси Y.
    @return Вектор с указанными координатами.
*/
INLINE vec2 vec2_create(const f32 x, const f32 y)
{
    return (vec2){{x, y}};
}

/*
    @brief Создает нулевой вектор (0, 0).
    @return Нулевой вектор.
*/
INLINE vec2 vec2_zero(void)
{
    return (vec2){{0.0f, 0.0f}};
}

/*
    @brief Создает единичный вектор (1, 1).
    @return Единичный вектор.
*/
INLINE vec2 vec2_one(void)
{
    return (vec2){{1.0f, 1.0f}};
}

/*
    @brief Создает вектор, направленный вверх (0, 1).
    @return Вектор направления вверх.
*/
INLINE vec2 vec2_up(void)
{
    return (vec2){{0.0f, 1.0f}};
}

/*
    @brief Создает вектор, направленный вниз (0, -1).
    @return Вектор направления вниз.
*/
INLINE vec2 vec2_down(void)
{
    return (vec2){{0.0f, -1.0f}};
}

/*
    @brief Создает вектор, направленный влево (-1, 0).
    @return Вектор направления влево.
*/
INLINE vec2 vec2_left(void)
{
    return (vec2){{-1.0f, 0.0f}};
}

/*
    @brief Создает вектор, направленный вправо (1, 0).
    @return Вектор направления вправо.
*/
INLINE vec2 vec2_right(void)
{
    return (vec2){{1.0f, 0.0f}};
}

/*
    @brief Возвращает минимальные компоненты двух векторов.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Вектор с минимальными компонентами (min(a.x, b.x), min(a.y, b.y)).
*/
INLINE vec2 vec2_min(const vec2 a, const vec2 b)
{
    return (vec2){{
        (a.x < b.x) ? a.x : b.x,
        (a.y < b.y) ? a.y : b.y
    }};
}

/*
    @brief Возвращает максимальные компоненты двух векторов.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Вектор с максимальными компонентами (max(a.x, b.x), max(a.y, b.y)).
*/
INLINE vec2 vec2_max(const vec2 a, const vec2 b)
{
    return (vec2){{
        (a.x > b.x) ? a.x : b.x,
        (a.y > b.y) ? a.y : b.y
    }};
}

/*
    @brief Складывает два вектора.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Сумма векторов (a + b).
*/
INLINE vec2 vec2_add(const vec2 a, const vec2 b)
{
    return (vec2){{
        a.x + b.x,
        a.y + b.y
    }};
}

/*
    @brief Вычитает второй вектор из первого.
    @param a Уменьшаемое.
    @param b Вычитаемое.
    @return Разность векторов (a - b).
*/
INLINE vec2 vec2_sub(const vec2 a, const vec2 b)
{
    return (vec2){{
        a.x - b.x,
        a.y - b.y
    }};
}

/*
    @brief Умножает два вектора покомпонентно.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Покомпонентное произведение (a.x * b.x, a.y * b.y).
*/
INLINE vec2 vec2_mul(const vec2 a, const vec2 b)
{
    return (vec2){{
        a.x * b.x,
        a.y * b.y
    }};
}

/*
    @brief Умножает вектор на скаляр.
    @param vector Вектор.
    @param scalar Скаляр.
    @return Вектор, умноженный на скаляр (vector * scalar).
*/
INLINE vec2 vec2_mul_scalar(const vec2 vector, const f32 scalar)
{
    return (vec2){{
        vector.x * scalar,
        vector.y * scalar
    }};
}

/*
    @brief Вычисляет a * b + c.
    @param a Первый множитель.
    @param b Второй множитель.
    @param c Слагаемое.
    @return Результат a * b + c.
*/
INLINE vec2 vec2_mul_add(const vec2 a, const vec2 b, const vec2 c)
{
    return (vec2){{
        a.x * b.x + c.x,
        a.y * b.y + c.y
    }};
}

/*
    @brief Делит два вектора покомпонентно.
    @warning Поведение не определено при b.x = 0 или b.y = 0.
    @param a Делимое.
    @param b Делитель.
    @return Покомпонентное частное (a.x / b.x, a.y / b.y).
*/
INLINE vec2 vec2_div(const vec2 a, const vec2 b)
{
    return (vec2){{
        a.x / b.x,
        a.y / b.y
    }};
}

/*
    @brief Делит вектор на скаляр.
    @warning Поведение не определено при scalar = 0.
    @param vector Делимое.
    @param scalar Делитель.
    @return Вектор, деленный на скаляр (vector / scalar).
*/
INLINE vec2 vec2_div_scalar(const vec2 vector, const f32 scalar)
{
    const f32 inv_scalar = 1.0f / scalar;
    return vec2_mul_scalar(vector, inv_scalar);
}

/*
    @brief Вычисляет квадрат длины вектора.
    @note Быстрее, чем вычисление длины, так как не требует извлечения корня.
    @param vector Вектор.
    @return Квадрат длины вектора (x^2 + y^2).
*/
INLINE f32 vec2_length_squared(const vec2 vector)
{
    return vector.x * vector.x + vector.y * vector.y;
}

/*
    @brief Вычисляет длину вектора (норму).
    @param vector Вектор.
    @return Длина вектора.
*/
INLINE f32 vec2_length(const vec2 vector)
{
    return math_sqrt(vec2_length_squared(vector));
}

/*
    @brief Нормализует предоставленный вектор.
    @note Если длина вектора меньше F32_EPSILON_CMP, вектор остается без изменений.
    @param vector Указатель на вектор для нормализации.
*/
INLINE void vec2_normalize(vec2* vector)
{
    const f32 length = vec2_length(*vector);
    if(length > F32_EPSILON_CMP)
    {
        const f32 inv_length = 1.0f / length;
        vector->x *= inv_length;
        vector->y *= inv_length;
    }
}

/*
    @brief Возвращает нормализованную копию вектора.
    @note Если длина вектора меньше F32_EPSILON_CMP, вектор остается без изменений.
    @param vector Входной вектор.
    @return Нормализованный вектор.
*/
INLINE vec2 vec2_normalized(vec2 vector)
{
    vec2_normalize(&vector);
    return vector;
}

/*
    @brief Проверяет равенство двух векторов с заданной точностью.
    @note Для сравнения используйте значение F32_EPSILON_CMP или большее.
    @param a Первый вектор.
    @param b Второй вектор.
    @param tolerance Допустимая погрешность.
    @return true - векторы равны с заданной точностью, false - не равны.
*/
INLINE bool vec2_equals(const vec2 a, const vec2 b, const f32 tolerance)
{
    return (math_abs(a.x - b.x) <= tolerance)
        && (math_abs(a.y - b.y) <= tolerance);
}

/*
    @brief Вычисляет расстояние между двумя точками.
    @param a Первая точка.
    @param b Вторая точка.
    @return Евклидово расстояние между точками.
*/
INLINE f32 vec2_distance(const vec2 a, const vec2 b)
{
    const vec2 diff = vec2_sub(a, b);
    return vec2_length(diff);
}

/*
    @brief Вычисляет квадрат расстояния между двумя точками.
    @note Быстрее, чем вычисление расстояния, так как не требует извлечения корня.
    @param a Первая точка.
    @param b Вторая точка.
    @return Квадрат расстояния между точками.
*/
INLINE f32 vec2_distance_squared(const vec2 a, const vec2 b)
{
    const vec2 diff = vec2_sub(a, b);
    return vec2_length_squared(diff);
}

/*
    @brief Вычисляет скалярное произведение двух векторов.
    @note Равно произведению длин векторов на косинус угла между ними.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Скалярное произведение (a·b).
*/
INLINE f32 vec2_dot(const vec2 a, const vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

/*
    @brief Вычисляет псевдоскалярное произведение (2D кросс-произведение).
    @note Модуль результата равен площади параллелограмма, натянутого на вектора.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Псевдоскалярное произведение (a.x * b.y - a.y * b.x).
*/
INLINE f32 vec2_cross(const vec2 a, const vec2 b)
{
    return a.x * b.y - a.y * b.x;
}

/*
    @brief Вычисляет угол между двумя векторами.
    @warning Поведение не определено для нулевых векторов.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Угол между векторами в радианах в диапазоне [0, π].
*/
INLINE f32 vec2_angle(const vec2 a, const vec2 b)
{
    const f32 len_a = vec2_length(a);
    const f32 len_b = vec2_length(b);

    if(len_a <= F32_EPSILON_CMP || len_b < F32_EPSILON_CMP)
    {
        return 0.0f;
    }

    const f32 cos_angle = vec2_dot(a, b) / (len_a * len_b);
    return math_acos(CLAMP(cos_angle, -1.0f, 1.0f));
}

/*
    @brief Проецирует вектор a на вектор b.
    @note Если b - нулевой вектор, возвращается нулевой вектор.
    @param a Проецируемый вектор.
    @param b Вектор, на который производится проекция.
    @return Проекция вектора a на направление b.
*/
INLINE vec2 vec2_project(const vec2 a, const vec2 b)
{
    const f32 len_sq = vec2_length_squared(b);
    if(len_sq < F32_EPSILON_CMP)
    {
        return vec2_zero();
    }

    const f32 scale = vec2_dot(a, b) / len_sq;
    return vec2_mul_scalar(b, scale);
}

/*
    @brief Отклоняет вектор a от направления b.
    @param a Исходный вектор.
    @param b Вектор направления.
    @return Компонент вектора a, ортогональный b.
*/
INLINE vec2 vec2_reject(const vec2 a, const vec2 b)
{
    return vec2_sub(a, vec2_project(a, b));
}

/*
    @brief Отражает вектор относительно нормали.
    @warning Нормаль должна быть единичной для корректного результата.
    @param incident Падающий вектор.
    @param normal Единичный вектор нормали.
    @return Отраженный вектор.
*/
INLINE vec2 vec2_reflect(const vec2 incident, const vec2 normal)
{
    const f32 dot = vec2_dot(incident, normal);
    return vec2_sub(incident, vec2_mul_scalar(normal, 2.0f * dot));
}

/*
    @brief Линейная интерполяция между двумя векторами.
    @note При t=0 возвращает a, при t=1 возвращает b.
    @param a Начальный вектор.
    @param b Конечный вектор.
    @param t Параметр интерполяции в диапазоне [0, 1].
    @return Интерполированный вектор.
*/
INLINE vec2 vec2_lerp(const vec2 a, const vec2 b, const f32 t)
{
    return (vec2){{
        math_lerp(a.x, b.x, t),
        math_lerp(a.y, b.y, t)
    }};
}

/*
    @brief Сферическая линейная интерполяция между двумя единичными векторами.
    @warning Входные векторы должны быть единичными.
    @param a Начальный единичный вектор.
    @param b Конечный единичный вектор.
    @param t Параметр интерполяции в диапазоне [0, 1].
    @return Интерполированный единичный вектор.
*/
INLINE vec2 vec2_slerp(const vec2 a, const vec2 b, const f32 t)
{
    const f32 dot = CLAMP(vec2_dot(a, b), -1.0f, 1.0f);
    const f32 theta = math_acos(dot) * t;
    const vec2 relative = vec2_normalized(vec2_sub(b, vec2_mul_scalar(a, dot)));

    return vec2_add(
        vec2_mul_scalar(a, math_cos(theta)),
        vec2_mul_scalar(relative, math_sin(theta))
    );
}

/*
    @brief Вращает вектор на заданный угол.
    @param vector Исходный вектор.
    @param angle_radians Угол поворота в радианах.
    @return Повернутый вектор.
*/
INLINE vec2 vec2_rotate(const vec2 vector, const f32 angle_radians)
{
    const f32 cos_angle = math_cos(angle_radians);
    const f32 sin_angle = math_sin(angle_radians);

    return (vec2){{
        vector.x * cos_angle - vector.y * sin_angle,
        vector.x * sin_angle + vector.y * cos_angle
    }};
}

/*
    @brief Вычисляет перпендикулярный вектор (поворот на 90° против часовой стрелки).
    @param vector Исходный вектор.
    @return Перпендикулярный вектор (-y, x).
*/
INLINE vec2 vec2_perpendicular(const vec2 vector)
{
    return (vec2){{-vector.y, vector.x}};
}

// ============================================================================
// Трехмерные векторы (vec3)
// ============================================================================

/*
    @brief Создает новый 3D вектор.
    @param x Координата по оси X.
    @param y Координата по оси Y.
    @param z Координата по оси Z.
    @return Вектор с указанными координатами.
*/
INLINE vec3 vec3_create(const f32 x, const f32 y, const f32 z)
{
    return (vec3){{x, y, z}};
}

/*
    @brief Создает нулевой вектор (0, 0, 0).
    @return Нулевой вектор.
*/
INLINE vec3 vec3_zero(void)
{
    return (vec3){{0.0f, 0.0f, 0.0f}};
}

/*
    @brief Создает единичный вектор (1, 1, 1).
    @return Единичный вектор.
*/
INLINE vec3 vec3_one(void)
{
    return (vec3){{1.0f, 1.0f, 1.0f}};
}

/*
    @brief Создает вектор, направленный вверх (0, 1, 0).
    @return Вектор направления вверх.
*/
INLINE vec3 vec3_up(void)
{
    return (vec3){{0.0f, 1.0f, 0.0f}};
}

/*
    @brief Создает вектор, направленный вниз (0, -1, 0).
    @return Вектор направления вниз.
*/
INLINE vec3 vec3_down(void)
{
    return (vec3){{0.0f, -1.0f, 0.0f}};
}

/*
    @brief Создает вектор, направленный влево (-1, 0, 0).
    @return Вектор направления влево.
*/
INLINE vec3 vec3_left(void)
{
    return (vec3){{-1.0f, 0.0f, 0.0f}};
}

/*
    @brief Создает вектор, направленный вправо (1, 0, 0).
    @return Вектор направления вправо.
*/
INLINE vec3 vec3_right(void)
{
    return (vec3){{1.0f, 0.0f, 0.0f}};
}

/*
    @brief Создает вектор, направленный вперед (0, 0, -1).
    @return Вектор направления вперед.
*/
INLINE vec3 vec3_forward(void)
{
    return (vec3){{0.0f, 0.0f, -1.0f}};
}

/*
    @brief Создает вектор, направленный назад (0, 0, 1).
    @return Вектор направления назад.
*/
INLINE vec3 vec3_backward(void)
{
    return (vec3){{0.0f, 0.0f, 1.0f}};
}

/*
    @brief Возвращает минимальные компоненты двух векторов.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Вектор с минимальными компонентами.
*/
INLINE vec3 vec3_min(const vec3 a, const vec3 b)
{
    return (vec3){{
        (a.x < b.x) ? a.x : b.x,
        (a.y < b.y) ? a.y : b.y,
        (a.z < b.z) ? a.z : b.z
    }};
}

/*
    @brief Возвращает максимальные компоненты двух векторов.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Вектор с максимальными компонентами.
*/
INLINE vec3 vec3_max(const vec3 a, const vec3 b)
{
    return (vec3){{
        (a.x > b.x) ? a.x : b.x,
        (a.y > b.y) ? a.y : b.y,
        (a.z > b.z) ? a.z : b.z
    }};
}

/*
    @brief Складывает два вектора.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Сумма векторов (a + b).
*/
INLINE vec3 vec3_add(const vec3 a, const vec3 b)
{
    return (vec3){{
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    }};
}

/*
    @brief Вычитает второй вектор из первого.
    @param a Уменьшаемое.
    @param b Вычитаемое.
    @return Разность векторов (a - b).
*/
INLINE vec3 vec3_sub(const vec3 a, const vec3 b)
{
    return (vec3){{
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    }};
}

/*
    @brief Умножает два вектора покомпонентно.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Покомпонентное произведение.
*/
INLINE vec3 vec3_mul(const vec3 a, const vec3 b)
{
    return (vec3){{
        a.x * b.x,
        a.y * b.y,
        a.z * b.z
    }};
}

/*
    @brief Умножает вектор на скаляр.
    @param vector Вектор.
    @param scalar Скаляр.
    @return Вектор, умноженный на скаляр.
*/
INLINE vec3 vec3_mul_scalar(const vec3 vector, const f32 scalar)
{
    return (vec3){{
        vector.x * scalar,
        vector.y * scalar,
        vector.z * scalar
    }};
}

/*
    @brief Вычисляет a * b + c.
    @param a Первый множитель.
    @param b Второй множитель.
    @param c Слагаемое.
    @return Результат a * b + c.
*/
INLINE vec3 vec3_mul_add(const vec3 a, const vec3 b, const vec3 c)
{
    return (vec3){{
        a.x * b.x + c.x,
        a.y * b.y + c.y,
        a.z * b.z + c.z
    }};
}

/*
    @brief Делит два вектора покомпонентно.
    @warning Поведение не определено при нулевых компонентах делителя.
    @param a Делимое.
    @param b Делитель.
    @return Покомпонентное частное.
*/
INLINE vec3 vec3_div(const vec3 a, const vec3 b)
{
    return (vec3){{
        a.x / b.x,
        a.y / b.y,
        a.z / b.z
    }};
}

/*
    @brief Делит вектор на скаляр.
    @warning Поведение не определено при scalar = 0.
    @param vector Делимое.
    @param scalar Делитель.
    @return Вектор, деленный на скаляр.
*/
INLINE vec3 vec3_div_scalar(const vec3 vector, const f32 scalar)
{
    const f32 inv_scalar = 1.0f / scalar;
    return vec3_mul_scalar(vector, inv_scalar);
}

/*
    @brief Вычисляет квадрат длины вектора.
    @param vector Вектор.
    @return Квадрат длины вектора (x^2 + y^2 + z^2).
*/
INLINE f32 vec3_length_squared(const vec3 vector)
{
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
}

/*
    @brief Вычисляет длину вектора (норму).
    @param vector Вектор.
    @return Длина вектора.
*/
INLINE f32 vec3_length(const vec3 vector)
{
    return math_sqrt(vec3_length_squared(vector));
}

/*
    @brief Нормализует предоставленный вектор.
    @note Если длина вектора меньше F32_EPSILON_CMP, вектор остается без изменений.
    @param vector Указатель на вектор для нормализации.
*/
INLINE void vec3_normalize(vec3* vector)
{
    const f32 length = vec3_length(*vector);
    if(length > F32_EPSILON_CMP)
    {
        const f32 inv_length = 1.0f / length;
        vector->x *= inv_length;
        vector->y *= inv_length;
        vector->z *= inv_length;
    }
}

/*
    @brief Возвращает нормализованную копию вектора.
    @note Если длина входного вектора меньше F32_EPSILON_CMP, возвращается нулевой вектор.
    @param vector Входной вектор.
    @return Нормализованный вектор.
*/
INLINE vec3 vec3_normalized(vec3 vector)
{
    vec3_normalize(&vector);
    return vector;
}

/*
    @brief Проверяет равенство двух векторов с заданной точностью.
    @param a Первый вектор.
    @param b Второй вектор.
    @param tolerance Допустимая погрешность.
    @return true - векторы равны с заданной точностью, false - не равны.
*/
INLINE bool vec3_equals(const vec3 a, const vec3 b, const f32 tolerance)
{
    return (math_abs(a.x - b.x) <= tolerance)
        && (math_abs(a.y - b.y) <= tolerance)
        && (math_abs(a.z - b.z) <= tolerance);
}

/*
    @brief Вычисляет расстояние между двумя точками.
    @param a Первая точка.
    @param b Вторая точка.
    @return Евклидово расстояние между точками.
*/
INLINE f32 vec3_distance(const vec3 a, const vec3 b)
{
    const vec3 diff = vec3_sub(a, b);
    return vec3_length(diff);
}

/*
    @brief Вычисляет квадрат расстояния между двумя точками.
    @param a Первая точка.
    @param b Вторая точка.
    @return Квадрат расстояния между точками.
*/
INLINE f32 vec3_distance_squared(const vec3 a, const vec3 b)
{
    const vec3 diff = vec3_sub(a, b);
    return vec3_length_squared(diff);
}

/*
    @brief Вычисляет скалярное произведение двух векторов.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Скалярное произведение (a·b).
*/
INLINE f32 vec3_dot(const vec3 a, const vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/*
    @brief Вычисляет векторное произведение двух векторов.
    @note Длина результата равна площади параллелограмма, натянутого на вектора. Результат ортогонален обоим векторам.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Векторное произведение (a × b).
*/
INLINE vec3 vec3_cross(const vec3 a, const vec3 b)
{
    return (vec3){{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    }};
}

/*
    @brief Вычисляет угол между двумя векторами.
    @warning Поведение не определено для нулевых векторов.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Угол между векторами в радианах в диапазоне [0, π].
*/
INLINE f32 vec3_angle(const vec3 a, const vec3 b)
{
    const f32 len_a = vec3_length(a);
    const f32 len_b = vec3_length(b);

    if(len_a < F32_EPSILON_CMP || len_b < F32_EPSILON_CMP)
    {
        return 0.0f;
    }

    const f32 cos_angle = vec3_dot(a, b) / (len_a * len_b);
    return math_acos(CLAMP(cos_angle, -1.0f, 1.0f));
}

/*
    @brief Проецирует вектор a на вектор b.
    @param a Проецируемый вектор.
    @param b Вектор, на который производится проекция.
    @return Проекция вектора a на направление b.
*/
INLINE vec3 vec3_project(const vec3 a, const vec3 b)
{
    const f32 len_sq = vec3_length_squared(b);

    if(len_sq < F32_EPSILON_CMP)
    {
        return vec3_zero();
    }

    const f32 scale = vec3_dot(a, b) / len_sq;
    return vec3_mul_scalar(b, scale);
}

/*
    @brief Отклоняет вектор a от направления b.
    @param a Исходный вектор.
    @param b Вектор направления.
    @return Компонент вектора a, ортогональный b.
*/
INLINE vec3 vec3_reject(const vec3 a, const vec3 b)
{
    return vec3_sub(a, vec3_project(a, b));
}

/*
    @brief Отражает вектор относительно нормали.
    @warning Нормаль должна быть единичной для корректного результата.
    @param incident Падающий вектор.
    @param normal Единичный вектор нормали.
    @return Отраженный вектор.
*/
INLINE vec3 vec3_reflect(const vec3 incident, const vec3 normal)
{
    const f32 dot = vec3_dot(incident, normal);
    return vec3_sub(incident, vec3_mul_scalar(normal, 2.0f * dot));
}

/*
    @brief Преломляет вектор на границе раздела сред.
    @warning Векторы должны быть единичными.
    @note При полном внутреннем отражении возвращает нулевой вектор.
    @param incident Падающий вектор.
    @param normal Единичный вектор нормали.
    @param eta Отношение коэффициентов преломления (n1/n2).
    @return Преломленный вектор.
*/
INLINE vec3 vec3_refract(const vec3 incident, const vec3 normal, const f32 eta)
{
    const f32 cos_i = -vec3_dot(incident, normal);
    const f32 sin_t2 = eta * eta * (1.0f - cos_i * cos_i);

    // Полное внутреннее отражение.
    if(sin_t2 > 1.0f)
    {
        return vec3_zero();
    }

    const f32 cos_t = math_sqrt(1.0f - sin_t2);

    return vec3_add(
        vec3_mul_scalar(incident, eta),
        vec3_mul_scalar(normal, eta * cos_i - cos_t)
    );
}

/*
    @brief Линейная интерполяция между двумя векторами.
    @param a Начальный вектор.
    @param b Конечный вектор.
    @param t Параметр интерполяции в диапазоне [0, 1].
    @return Интерполированный вектор.
*/
INLINE vec3 vec3_lerp(const vec3 a, const vec3 b, const f32 t)
{
    return (vec3){{
        math_lerp(a.x, b.x, t),
        math_lerp(a.y, b.y, t),
        math_lerp(a.z, b.z, t)
    }};
}

/*
    @brief Сферическая линейная интерполяция между двумя единичными векторами.
    @warning Входные векторы должны быть единичными.
    @param a Начальный единичный вектор.
    @param b Конечный единичный вектор.
    @param t Параметр интерполяции в диапазоне [0, 1].
    @return Интерполированный единичный вектор.
*/
INLINE vec3 vec3_slerp(const vec3 a, const vec3 b, const f32 t)
{
    const f32 dot = CLAMP(vec3_dot(a, b), -1.0f, 1.0f);
    const f32 theta = math_acos(dot) * t;
    const vec3 relative = vec3_normalized(vec3_sub(b, vec3_mul_scalar(a, dot)));

    return vec3_add(
        vec3_mul_scalar(a, math_cos(theta)),
        vec3_mul_scalar(relative, math_sin(theta))
    );
}

/*
    @brief Преобразует вектор с помощью матрицы 4x4.
    @note Для преобразования направлений используйте w = 0.0, а точек используйте w = 1.0.
    @param v Входной вектор.
    @param w Компонент однородных координат (1.0 для точки, 0.0 для направления).
    @param m Матрица преобразования.
    @return Преобразованный вектор.
*/
INLINE vec3 vec3_transform(const vec3 v, const f32 w, const mat4 m)
{
    return (vec3){{
        v.x * m.data[0] + v.y * m.data[4] + v.z * m.data[8]  + w * m.data[12], // x
        v.x * m.data[1] + v.y * m.data[5] + v.z * m.data[9]  + w * m.data[13], // y
        v.x * m.data[2] + v.y * m.data[6] + v.z * m.data[10] + w * m.data[14]  // z
    }};
}

/*
    @brief Поворачивает вектор с помощью кватерниона.
    @warning Кватернион должен быть единичным.
    @param v Входной вектор.
    @param q Кватернион вращения.
    @return Повернутый вектор.
*/
INLINE vec3 vec3_rotate(const vec3 v, const quat q)
{
    const vec3 u = {{q.x, q.y, q.z}};
    const f32 s = q.w;

    // v' = v + 2.0 * cross(u, cross(u, v) + s * v)
    const vec3 cross1 = vec3_cross(u, v);
    const vec3 temp   = vec3_add(cross1, vec3_mul_scalar(v, s));
    const vec3 cross2 = vec3_cross(u, temp);

    return vec3_add(v, vec3_mul_scalar(cross2, 2.0f));
}

/*
    @brief Создает 3D вектор из 4D вектора (отбрасывает компонент w).
    @param vector Входной 4D вектор.
    @return 3D вектор (x, y, z).
*/
INLINE vec3 vec3_from_vec4(const vec4 vector)
{
    return (vec3){{vector.x, vector.y, vector.z}};
}

/*
    @brief Создает 4D вектор из 3D вектора с добавлением компонента w.
    @param vector Входной 3D вектор.
    @param w Компонент w.
    @return 4D вектор (x, y, z, w).
*/
INLINE vec4 vec3_to_vec4(const vec3 vector, const f32 w)
{
    return (vec4){{vector.x, vector.y, vector.z, w}};
}

// ============================================================================
// Четырехмерные векторы (vec4)
// ============================================================================

/*
    @brief Создает новый 4D вектор.
    @param x Компонент X.
    @param y Компонент Y.
    @param z Компонент Z.
    @param w Компонент W.
    @return Вектор с указанными компонентами.
*/
INLINE vec4 vec4_create(const f32 x, const f32 y, const f32 z, const f32 w)
{
    return (vec4){{x, y, z, w}};
}

/*
    @brief Создает нулевой вектор (0, 0, 0, 0).
    @return Нулевой вектор.
*/
INLINE vec4 vec4_zero(void)
{
    return (vec4){{0.0f, 0.0f, 0.0f, 0.0f}};
}

/*
    @brief Создает единичный вектор (1, 1, 1, 1).
    @return Единичный вектор.
*/
INLINE vec4 vec4_one(void)
{
    return (vec4){{1.0f, 1.0f, 1.0f, 1.0f}};
}

/*
    @brief Складывает два вектора.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Сумма векторов (a + b).
*/
INLINE vec4 vec4_add(const vec4 a, const vec4 b)
{
    return (vec4){{
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
        a.w + b.w
    }};
}

/*
    @brief Вычитает второй вектор из первого.
    @param a Уменьшаемое.
    @param b Вычитаемое.
    @return Разность векторов (a - b).
*/
INLINE vec4 vec4_sub(const vec4 a, const vec4 b)
{
    return (vec4){{
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
        a.w - b.w
    }};
}

/*
    @brief Умножает два вектора покомпонентно.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Покомпонентное произведение.
*/
INLINE vec4 vec4_mul(const vec4 a, const vec4 b)
{
    return (vec4){{
        a.x * b.x,
        a.y * b.y,
        a.z * b.z,
        a.w * b.w
    }};
}

/*
    @brief Умножает вектор на скаляр.
    @param vector Вектор.
    @param scalar Скаляр.
    @return Вектор, умноженный на скаляр.
*/
INLINE vec4 vec4_mul_scalar(const vec4 vector, const f32 scalar)
{
    return (vec4){{
        vector.x * scalar,
        vector.y * scalar,
        vector.z * scalar,
        vector.w * scalar
    }};
}

/*
    @brief Вычисляет a * b + c.
    @param a Первый множитель.
    @param b Второй множитель.
    @param c Слагаемое.
    @return Результат a * b + c.
*/
INLINE vec4 vec4_mul_add(const vec4 a, const vec4 b, const vec4 c)
{
    return (vec4){{
        a.x * b.x + c.x,
        a.y * b.y + c.y,
        a.z * b.z + c.z,
        a.w * b.w + c.w
    }};
}

/*
    @brief Делит два вектора покомпонентно.
    @warning Поведение не определено при нулевых компонентах делителя.
    @param a Делимое.
    @param b Делитель.
    @return Покомпонентное частное.
*/
INLINE vec4 vec4_div(const vec4 a, const vec4 b)
{
    return (vec4){{
        a.x / b.x,
        a.y / b.y,
        a.z / b.z,
        a.w / b.w
    }};
}

/*
    @brief Делит вектор на скаляр.
    @warning Поведение не определено при scalar = 0.
    @param vector Делимое.
    @param scalar Делитель.
    @return Вектор, деленный на скаляр.
*/
INLINE vec4 vec4_div_scalar(const vec4 vector, const f32 scalar)
{
    const f32 inv_scalar = 1.0f / scalar;
    return vec4_mul_scalar(vector, inv_scalar);
}

/*
    @brief Вычисляет квадрат длины вектора.
    @param vector Вектор.
    @return Квадрат длины вектора (x^2 + y^2 + z^2 + w^2).
*/
INLINE f32 vec4_length_squared(const vec4 vector)
{
    return vector.x * vector.x + vector.y * vector.y + 
           vector.z * vector.z + vector.w * vector.w;
}

/*
    @brief Вычисляет длину вектора (норму).
    @param vector Вектор.
    @return Длина вектора.
*/
INLINE f32 vec4_length(const vec4 vector)
{
    return math_sqrt(vec4_length_squared(vector));
}

/*
    @brief Нормализует вектор на месте.
    @note Если длина вектора меньше F32_EPSILON_CMP, вектор остается без изменений.
    @param vector Указатель на вектор для нормализации.
*/
INLINE void vec4_normalize(vec4* vector)
{
    const f32 length = vec4_length(*vector);
    if(length > F32_EPSILON_CMP)
    {
        const f32 inv_length = 1.0f / length;
        vector->x *= inv_length;
        vector->y *= inv_length;
        vector->z *= inv_length;
        vector->w *= inv_length;
    }
}

/*
    @brief Возвращает нормализованную копию вектора.
    @note Если длина входного вектора меньше F32_EPSILON_CMP, возвращается копия вектора.
    @param vector Входной вектор.
    @return Нормализованный вектор (единичный).
*/
INLINE vec4 vec4_normalized(const vec4 vector)
{
    vec4 result = vector;
    vec4_normalize(&result);
    return result;
}

/*
    @brief Проверяет равенство двух векторов с заданной точностью.
    @param a Первый вектор.
    @param b Второй вектор.
    @param tolerance Допустимая погрешность.
    @return true - векторы равны с заданной точностью, false - не равны.
*/
INLINE bool vec4_equals(const vec4 a, const vec4 b, const f32 tolerance)
{
    return (math_abs(a.x - b.x) <= tolerance)
        && (math_abs(a.y - b.y) <= tolerance)
        && (math_abs(a.z - b.z) <= tolerance)
        && (math_abs(a.w - b.w) <= tolerance);
}

/*
    @brief Вычисляет скалярное произведение двух векторов.
    @param a Первый вектор.
    @param b Второй вектор.
    @return Скалярное произведение (a·b).
*/
INLINE f32 vec4_dot(const vec4 a, const vec4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

/*
    @brief Создает 3D вектор из 4D вектора (отбрасывает компонент w).
    @param vector Входной 4D вектор.
    @return 3D вектор (x, y, z).
*/
INLINE vec3 vec4_to_vec3(const vec4 vector)
{
    return (vec3){{vector.x, vector.y, vector.z}};
}

/*
    @brief Создает 4D вектор из 3D вектора с добавлением компонента w.
    @param vector Входной 3D вектор.
    @param w Компонент w.
    @return 4D вектор (x, y, z, w).
*/
INLINE vec4 vec4_from_vec3(const vec3 vector, const f32 w)
{
    return (vec4){{vector.x, vector.y, vector.z, w}};
}
