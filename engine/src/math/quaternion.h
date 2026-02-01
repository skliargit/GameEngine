#pragma once

#include <math/types.h>
#include <math/constants.h>
#include <math/basic.h>
#include <math/vector.h>
#include <math/matrix.h>

// ============================================================================
// КВАТЕРНИОНЫ (quat)
// ============================================================================

/*
    @brief Создает единичный кватернион (без вращения).
    @return Единичный кватернион.
*/
INLINE quat quat_identity(void)
{
    return (quat){{0.0f, 0.0f, 0.0f, 1.0f}};
}

/*
    @brief Вычисляет квадрат нормы кватерниона.
    @param q Кватернион.
    @return Квадрат нормы кватерниона.
*/
INLINE f32 quat_normal_squared(const quat q)
{
    return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}

/*
    @brief Вычисляет норму кватерниона.
    @param q Кватернион.
    @return Норма кватерниона.
*/
INLINE f32 quat_normal(const quat q)
{
    return math_sqrt(quat_normal_squared(q));
}

/*
    @brief Нормализует кватернион на месте.
    @note Если норма кватерниона меньше F32_EPSILON_CMP, кватернион остается без изменений.
    @param q Указатель на кватернион для нормализации.
*/
INLINE void quat_normalize(quat* q)
{
    const f32 normal = quat_normal(*q);
    if(normal > F32_EPSILON_CMP)
    {
        const f32 inv_normal = 1.0f / normal;
        q->x *= inv_normal;
        q->y *= inv_normal;
        q->z *= inv_normal;
        q->w *= inv_normal;
    }
}

/*
    @brief Возвращает нормализованную копию кватерниона.
    @note Если норма кватерниона меньше F32_EPSILON_CMP, кватернион остается без изменений.
    @param q Входной кватернион.
    @return Нормализованный кватернион.
 */
INLINE quat quat_normalized(quat q)
{
    quat_normalize(&q);
    return q;
}

/*
    @brief Вычисляет сопряженный кватернион.
    @param q Исходный кватернион.
    @return Сопряженный кватернион.
*/
INLINE quat quat_conjugate(const quat q)
{
    return (quat){{-q.x, -q.y, -q.z, q.w}};
}

/*
    @brief Вычисляет обратный кватернион.
    @warning Для нулевого кватерниона возвращается единичный.
    @param q Исходный кватернион.
    @return Обратный кватернион.
*/
INLINE quat quat_inverse(const quat q)
{
    return quat_normalized(quat_conjugate(q));
}

/*
    @brief Умножает два кватерниона.
    @note Умножение соответствует композиции вращений.
    @param a Первый кватернион.
    @param b Второй кватернион.
    @return Произведение кватернионов (a × b).
*/
INLINE quat quat_mul(const quat a, const quat b)
{
    return (quat){{
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    }};
}

/*
    @brief Вычисляет скалярное произведение двух кватернионов.
    @param a Первый кватернион.
    @param b Второй кватернион.
    @return Скалярное произведение.
*/
INLINE f32 quat_dot(const quat a, const quat b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

/*
    @brief Создает кватернион из углов Эйлера (в радианах).
    @note Порядок вращений: Yaw (Y) → Pitch (X) → Roll (Z).
    @param pitch Вращение вокруг оси X (тангаж) в радианах.
    @param yaw Вращение вокруг оси Y (рыскание) в радианах.
    @param roll Вращение вокруг оси Z (крен) в радианах.
    @return Кватернион, представляющий вращение.
*/
INLINE quat quat_from_euler(const f32 pitch, const f32 yaw, const f32 roll)
{
    const f32 half_pitch = 0.5f * pitch;
    const f32 half_yaw   = 0.5f * yaw;
    const f32 half_roll  = 0.5f * roll;

    const f32 cos_pitch  = math_cos(half_pitch);
    const f32 sin_pitch  = math_sin(half_pitch);
    const f32 cos_yaw    = math_cos(half_yaw);
    const f32 sin_yaw    = math_sin(half_yaw);
    const f32 cos_roll   = math_cos(half_roll);
    const f32 sin_roll   = math_sin(half_roll);

    return (quat){{
        sin_roll * cos_pitch * cos_yaw - cos_roll * sin_pitch * sin_yaw,
        cos_roll * sin_pitch * cos_yaw + sin_roll * cos_pitch * sin_yaw,
        cos_roll * cos_pitch * sin_yaw - sin_roll * sin_pitch * cos_yaw,
        cos_roll * cos_pitch * cos_yaw + sin_roll * sin_pitch * sin_yaw
    }};
}

/*
    @brief Создает кватернион из оси и угла вращения.
    @warning Ось должна быть единичной для корректного результата.
    @param axis Ось вращения (должна быть единичной).
    @param angle_radians Угол вращения в радианах.
    @return Кватернион, представляющий вращение.
*/
INLINE quat quat_from_axis_angle(const vec3 axis, const f32 angle_radians)
{
    const f32 half_angle = angle_radians * 0.5f;
    const f32 sin_half   = math_sin(half_angle);

    return (quat){{
        axis.x * sin_half,
        axis.y * sin_half,
        axis.z * sin_half,
        math_cos(half_angle)
    }};
}

/*
    @brief Создает кватернион из матрицы вращения 3x3 или 4x4.
    @param matrix Матрица вращения.
    @return Кватернион, представляющий вращение.
*/
INLINE quat quat_from_mat4(const mat4 matrix)
{
    const f32* m = matrix.data;
    const f32 trace = m[0] + m[5] + m[10];
    
    if(trace > 0.0f)
    {
        const f32 s = 0.5f / math_sqrt(trace + 1.0f);
        return (quat){{
            (m[9] - m[6]) * s,
            (m[2] - m[8]) * s,
            (m[4] - m[1]) * s,
            0.25f / s
        }};
    }
    else if(m[0] > m[5] && m[0] > m[10])
    {
        const f32 s = 2.0f * math_sqrt(1.0f + m[0] - m[5] - m[10]);
        const f32 inv_s = 1.0f / s;
        return (quat){{
            0.25f * s,
            (m[4] + m[1]) * inv_s,
            (m[2] + m[8]) * inv_s,
            (m[9] - m[6]) * inv_s
        }};
    }
    else if(m[5] > m[10])
    {
        const f32 s = 2.0f * math_sqrt(1.0f + m[5] - m[0] - m[10]);
        const f32 inv_s = 1.0f / s;
        return (quat){{
            (m[4] + m[1]) * inv_s,
            0.25f * s,
            (m[9] + m[6]) * inv_s,
            (m[2] - m[8]) * inv_s
        }};
    }
    else
    {
        const f32 s = 2.0f * math_sqrt(1.0f + m[10] - m[0] - m[5]);
        const f32 inv_s = 1.0f / s;
        return (quat){{
            (m[2] + m[8]) * inv_s,
            (m[9] + m[6]) * inv_s,
            0.25f * s,
            (m[4] - m[1]) * inv_s
        }};
    }
}

/*
    @brief Преобразует кватернион в матрицу вращения 4x4.
    @param q Кватернион вращения.
    @return Матрица вращения.
*/
INLINE mat4 quat_to_mat4(const quat q)
{
    const f32 x = q.x;
    const f32 y = q.y;
    const f32 z = q.z;
    const f32 w = q.w;

    const f32 x2 = x + x;
    const f32 y2 = y + y;
    const f32 z2 = z + z;

    const f32 xx = x * x2;
    const f32 xy = x * y2;
    const f32 xz = x * z2;
    const f32 yy = y * y2;
    const f32 yz = y * z2;
    const f32 zz = z * z2;
    const f32 wx = w * x2;
    const f32 wy = w * y2;
    const f32 wz = w * z2;

    // FIX: mat4_identity!
    mat4 result = (mat4){{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }};

    result.data[ 0] = 1.0f - (yy + zz);
    result.data[ 1] = xy + wz;
    result.data[ 2] = xz - wy;

    result.data[ 4] = xy - wz;
    result.data[ 5] = 1.0f - (xx + zz);
    result.data[ 6] = yz + wx;

    result.data[ 8] = xz + wy;
    result.data[ 9] = yz - wx;
    result.data[10] = 1.0f - (xx + yy);

    return result;
}

/*
    @brief Сферическая линейная интерполяция между двумя кватернионами.
    @note Кватернионы должны быть единичными.
    @param a Начальный кватернион.
    @param b Конечный кватернион.
    @param t Параметр интерполяции в диапазоне [0, 1].
    @return Интерполированный кватернион.
*/
INLINE quat quat_slerp(const quat a, const quat b, const f32 t)
{
    quat q0 = quat_normalized(a);
    quat q1 = quat_normalized(b);

    f32 cos_theta = quat_dot(q0, q1);

    // Если dot < 0, выбираем короткий путь.
    if(cos_theta < 0.0f)
    {
        q1.x = -q1.x;
        q1.y = -q1.y;
        q1.z = -q1.z;
        q1.w = -q1.w;
        cos_theta = -cos_theta;
    }

    const f32 DOT_THRESHOLD = 0.9995f;
    if (cos_theta > DOT_THRESHOLD)
    {
        // Близкие кватернионы, используем линейную интерполяцию.
        quat result = {{
            q0.x + (q1.x - q0.x) * t,
            q0.y + (q1.y - q0.y) * t,
            q0.z + (q1.z - q0.z) * t,
            q0.w + (q1.w - q0.w) * t
        }};
        return quat_normalized(result);
    }

    const f32 theta = math_acos(cos_theta);
    const f32 sin_theta = math_sin(theta);
    const f32 inv_sin_theta = 1.0f / sin_theta;

    const f32 factor0 = math_sin((1.0f - t) * theta) * inv_sin_theta;
    const f32 factor1 = math_sin(t * theta) * inv_sin_theta;

    return (quat){{
        q0.x * factor0 + q1.x * factor1,
        q0.y * factor0 + q1.y * factor1,
        q0.z * factor0 + q1.z * factor1,
        q0.w * factor0 + q1.w * factor1
    }};
}

/*
    @brief Нормализованная линейная интерполяция.
    @note Быстрее, чем SLERP, но не сохраняет постоянную угловую скорость.
    @param a Начальный кватернион.
    @param b Конечный кватернион.
    @param t Параметр интерполяции в диапазоне [0, 1].
    @return Интерполированный кватернион.
*/
INLINE quat quat_nlerp(const quat a, const quat b, const f32 t)
{
    quat result = {{
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    }};

    return quat_normalized(result);
}

/*
    @brief Поворачивает вектор с помощью кватерниона.
    @param v Вектор для поворота.
    @param q Кватернион вращения.
    @return Повернутый вектор.
*/
INLINE vec3 quat_rotate_vec3(const vec3 v, const quat q)
{
    const quat p = {{v.x, v.y, v.z, 0.0f}};
    const quat q_conj = quat_conjugate(q);
    const quat rotated = quat_mul(quat_mul(q, p), q_conj);
    
    return (vec3){{rotated.x, rotated.y, rotated.z}};
}
