#pragma once

#include <math/types.h>
#include <math/constants.h>
#include <math/basic.h>
#include <math/vector.h>
#include <math/quaternion.h>

// ============================================================================
// Матрицы 4x4 (mat4)
// ============================================================================

/*
    @brief Создает единичную матрицу 4x4.
    @return Единичная матрица.
*/
INLINE mat4 mat4_identity(void)
{
    return (mat4){{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }};
}

/*
    @brief Умножает две матрицы 4x4.
    @note Умножение матриц не коммутативно: a × b != b × a.
    @param a Первая матрица.
    @param b Вторая матрица.
    @return Произведение матриц (a × b).
*/
INLINE mat4 mat4_mul(const mat4 a, const mat4 b)
{
    const f32* a_data = a.data;
    const f32* b_data = b.data;

    mat4 result;
    f32* r_data = result.data;

    for(u32 i = 0; i < 4; ++i)
    {
        for(u32 j = 0; j < 4; ++j)
        {
            r_data[i * 4 + j] = a_data[i * 4 + 0] * b_data[0 * 4 + j]
                              + a_data[i * 4 + 1] * b_data[1 * 4 + j]
                              + a_data[i * 4 + 2] * b_data[2 * 4 + j]
                              + a_data[i * 4 + 3] * b_data[3 * 4 + j];
        }
    }

    return result;
}

/*
    @brief Создает матрицу ортогональной проекции.
    @warning При near = far возникает деление на ноль.
    @note Используется для 2D-рендеринга и интерфейсов.
    @param left Левая граница области видимости.
    @param right Правая граница области видимости.
    @param bottom Нижняя граница области видимости.
    @param top Верхняя граница области видимости.
    @param near Ближняя плоскость отсечения.
    @param far Дальняя плоскость отсечения.
    @return Матрица ортогональной проекции.
*/
INLINE mat4 mat4_orthographic(const f32 left, const f32 right, const f32 bottom, const f32 top, const f32 near, const f32 far)
{
    const f32 rl = 1.0f / (right - left);
    const f32 tb = 1.0f / (top - bottom);
    const f32 fn = 1.0f / (far - near);

    mat4 result = mat4_identity();
    result.data[0]  = 2.0f * rl;             // X масштабирование.
    result.data[5]  = 2.0f * tb;             // Y масштабирование (положительно = Y вверх + инвертирование viewport для Vulkan).
    result.data[10] = fn;                    // Z масштабирование для [0,1].
    result.data[12] = -(right + left) * rl;  // X смещение.
    result.data[13] = -(top + bottom) * tb;  // Y cмещение (отрицательно = Y вверх).
    result.data[14] = -near * fn;            // Z смещение для [0,1], отображает near в 0.

    return result;
}

/*
    @brief Создает матрицу перспективной проекции.
    @warning При near = far или aspect_ratio = 0 возникает деление на ноль.
    @note Используется для 3D-рендеринга, использует правостороннюю систему координат.
    @param fov_radians Угол обзора по вертикали в радианах (обычно 45-90 градусов).
    @param aspect_ratio Соотношение сторон (ширина / высота).
    @param near Ближняя плоскость отсечения (не может быть 0).
    @param far Дальняя плоскость отсечения (не может быть 0).
    @return Матрица перспективной проекции.
*/
INLINE mat4 mat4_perspective(const f32 fov_radians, const f32 aspect_ratio, const f32 near, const f32 far)
{
    const f32 hf = math_ctg(fov_radians * 0.5f);
    const f32 nf = 1.0f / (near - far);

    mat4 result = {0};
    result.data[0]  = hf / aspect_ratio;     // X масштабирование.
    result.data[5]  = hf;                    // Y масштабирование (положительно = Y вверх + инвертирование viewport для Vulkan).
    result.data[10] = far * nf;              // Z масштабирование для [0,1].
    result.data[11] = -1.0f;                 // Запись -Z в W-компоненту вектора для перспективного деления.
    result.data[14] = far * near * nf;       // Z смещение.

    return result;
}

/*
    @brief Обновляет угол обзора (FOV) существующей матрицы перспективной проекции.
    @warning Не выполняет проверку на валидность входных параметров (near/far, aspect ratio).
    @note Изменяет только коэффициенты масштабирования X и Y, сохраняя остальные параметры проекции.
    @param proj Указатель на существующую матрицу проекции для модификации.
    @param new_fov_radians Новый угол обзора по вертикали в радианах.
*/
INLINE void mat4_perspective_update_fov(mat4* proj, const f32 new_fov_radians)
{
    const f32 hf = math_ctg(new_fov_radians * 0.5f);
    const f32 inv_aspect = proj->data[0] / proj->data[5];
    proj->data[0] = hf * inv_aspect;         // X масштабирование.
    proj->data[5] = hf;                      // Y масштабирование (положительно = Y вверх + инвертирование viewport для Vulkan).
}

/*
    @brief Обновляет соотношение сторон существующей матрицы перспективной проекции.
    @warning При new_aspect_ratio = 0 возникает деление на ноль.
    @note Корректирует только масштабирование по оси X для сохранения угла обзора по вертикали.
    @param proj Указатель на существующую матрицу проекции для модификации.
    @param new_aspect_ratio Новое соотношение сторон (ширина / высота).
*/
INLINE void mat4_perspective_update_aspect(mat4* proj, const f32 new_aspect_ratio)
{
    proj->data[0] = proj->data[5] / new_aspect_ratio; // X масштабирование.
}

/*
    @brief Обновляет ближнюю и дальнюю плоскости отсечения существующей матрицы перспективной проекции.
    @warning При new_near = new_far возникает деление на ноль.
    @note Изменяет только коэффициенты, связанные с преобразованием глубины (Z-компонента).
    @param proj Указатель на существующую матрицу проекции для модификации.
    @param new_near Новая ближняя плоскость отсечения (не может быть 0).
    @param new_far Новая дальняя плоскость отсечения (не может быть 0).
*/
INLINE void mat4_perspective_update_clip(mat4* proj, const f32 new_near, const f32 new_far)
{
    const f32 nf = 1.0f / (new_near - new_far);
    proj->data[10] = new_far * nf;                    // Z масштабирование для [0,1].
    proj->data[14] = new_far * new_near * nf;         // Z смещение.
}

/*
    @brief Создает матрицу вида (look-at).
    @note Вектор up не обязан быть ортогональным направлению взгляда.
    @param position Позиция камеры.
    @param target Точка, на которую смотрит камера.
    @param up Вектор, направленный вверх.
    @return Матрица вида.
*/
INLINE mat4 mat4_look_at(const vec3 position, const vec3 target, const vec3 up)
{
    const vec3 f = vec3_normalized(vec3_sub(target, position)); // z.
    const vec3 r = vec3_normalized(vec3_cross(f, up));          // x.
    const vec3 u = vec3_cross(r, f);                            // y.

    mat4 result = mat4_identity();

    result.data[0]  = r.x;
    result.data[4]  = r.y;
    result.data[8]  = r.z;

    result.data[1]  = u.x;
    result.data[5]  = u.y;
    result.data[9]  = u.z;

    result.data[2]  = -f.x;
    result.data[6]  = -f.y;
    result.data[10] = -f.z;

    result.data[12] = -vec3_dot(r, position);
    result.data[13] = -vec3_dot(u, position);
    result.data[14] =  vec3_dot(f, position);

    return result;
}

/*
    @brief Возвращает транспонированную матрицу (строки -> столбцы).
    @param matrix Исходная матрица.
    @return Транспонированная матрица.
*/
INLINE mat4 mat4_transposed(const mat4 matrix)
{
    mat4 result;

    for(u32 i = 0; i < 4; ++i)
    {
        for(u32 j = 0; j < 4; ++j)
        {
            result.data[i * 4 + j] = matrix.data[j * 4 + i];
        }
    }

    return result;
}

/*
    @brief Вычисляет определитель матрицы.
    @note Определитель равен 0, если матрица вырождена.
    @param matrix Матрица.
    @return Определитель матрицы.
*/
INLINE f32 mat4_determinant(const mat4 matrix)
{
    const f32* m = matrix.data;

    const f32 sub00 = m[10] * m[15] - m[11] * m[14];
    const f32 sub01 = m[9]  * m[15] - m[11] * m[13];
    const f32 sub02 = m[9]  * m[14] - m[10] * m[13];
    const f32 sub03 = m[8]  * m[15] - m[11] * m[12];
    const f32 sub04 = m[8]  * m[14] - m[10] * m[12];
    const f32 sub05 = m[8]  * m[13] - m[9]  * m[12];

    return m[0] * (m[5] * sub00 - m[6] * sub01 + m[7] * sub02) -
           m[1] * (m[4] * sub00 - m[6] * sub03 + m[7] * sub04) +
           m[2] * (m[4] * sub01 - m[5] * sub03 + m[7] * sub05) -
           m[3] * (m[4] * sub02 - m[5] * sub04 + m[6] * sub05);
}

/*
    @brief Вычисляет обратную матрицу.
    @warning Поведение не определено для вырожденных матриц (определитель = 0).
    @param matrix Исходная матрица.
    @return Обратная матрица.
*/
INLINE mat4 mat4_inverse(const mat4 matrix)
{
    const f32* m = matrix.data;

    // Вычисляем все необходимые 2x2 произведения.
    const f32 t[24] = {
        m[10] * m[15], m[14] * m[11], m[ 6] * m[15], m[14] * m[ 7], m[ 6] * m[11], //  0 -  4
        m[10] * m[ 7], m[ 2] * m[15], m[14] * m[ 3], m[ 2] * m[11], m[10] * m[ 3], //  5 -  9
        m[ 2] * m[ 7], m[ 6] * m[ 3], m[ 8] * m[13], m[12] * m[ 9], m[ 4] * m[13], // 10 - 14
        m[12] * m[ 5], m[ 4] * m[ 9], m[ 8] * m[ 5], m[ 0] * m[13], m[12] * m[ 1], // 15 - 19
        m[ 0] * m[ 9], m[ 8] * m[ 1], m[ 0] * m[ 5], m[ 4] * m[ 1]                 // 20 - 23
    };

    mat4 result;
    f32* inv = result.data;

    // Вычисляем первые 4 элемента (первый столбец).
    inv[0] = (t[0] * m[5] + t[3] * m[9] + t[ 4] * m[13])
           - (t[1] * m[5] + t[2] * m[9] + t[ 5] * m[13]);
    inv[1] = (t[1] * m[1] + t[6] * m[9] + t[ 9] * m[13])
           - (t[0] * m[1] + t[7] * m[9] + t[ 8] * m[13]);
    inv[2] = (t[2] * m[1] + t[7] * m[5] + t[10] * m[13])
           - (t[3] * m[1] + t[6] * m[5] + t[11] * m[13]);
    inv[3] = (t[5] * m[1] + t[8] * m[5] + t[11] * m[ 9])
           - (t[4] * m[1] + t[9] * m[5] + t[10] * m[ 9]);

    // Вычисляем определитель через уже найденные элементы.
    const f32 det = m[0] * inv[0] + m[4] * inv[1] + m[8] * inv[2] + m[12] * inv[3];
    const f32 inv_det = 1.0f / det;

    // Проверка на вырожденность матрицы.
    if(math_abs(det) < F32_EPSILON_CMP)
    {
        return mat4_identity();
    }

    // Делим первые 4 элемента на определитель.
    inv[0] *= inv_det;
    inv[1] *= inv_det;
    inv[2] *= inv_det;
    inv[3] *= inv_det;

    // Вычисляем остальные элементы.
    inv[4]  = inv_det * ((t[ 1] * m[ 4] + t[ 2] * m[ 8] + t[ 5] * m[12])
                      - ( t[ 0] * m[ 4] + t[ 3] * m[ 8] + t[ 4] * m[12]));
    inv[5]  = inv_det * ((t[ 0] * m[ 0] + t[ 7] * m[ 8] + t[ 8] * m[12])
                      - ( t[ 1] * m[ 0] + t[ 6] * m[ 8] + t[ 9] * m[12]));
    inv[6]  = inv_det * ((t[ 3] * m[ 0] + t[ 6] * m[ 4] + t[11] * m[12])
                      - ( t[ 2] * m[ 0] + t[ 7] * m[ 4] + t[10] * m[12]));
    inv[7]  = inv_det * ((t[ 4] * m[ 0] + t[ 9] * m[ 4] + t[10] * m[ 8])
                      - ( t[ 5] * m[ 0] + t[ 8] * m[ 4] + t[11] * m[ 8]));
    inv[8]  = inv_det * ((t[12] * m[ 7] + t[15] * m[11] + t[16] * m[15])
                      - ( t[13] * m[ 7] + t[14] * m[11] + t[17] * m[15]));
    inv[9]  = inv_det * ((t[13] * m[ 3] + t[18] * m[11] + t[21] * m[15])
                      - ( t[12] * m[ 3] + t[19] * m[11] + t[20] * m[15]));
    inv[10] = inv_det * ((t[14] * m[ 3] + t[19] * m[ 7] + t[22] * m[15])
                      - ( t[15] * m[ 3] + t[18] * m[ 7] + t[23] * m[15]));
    inv[11] = inv_det * ((t[17] * m[ 3] + t[20] * m[ 7] + t[23] * m[11])
                      - ( t[16] * m[ 3] + t[21] * m[ 7] + t[22] * m[11]));
    inv[12] = inv_det * ((t[14] * m[10] + t[17] * m[14] + t[13] * m[ 6])
                      - ( t[16] * m[14] + t[12] * m[ 6] + t[15] * m[10]));
    inv[13] = inv_det * ((t[20] * m[14] + t[12] * m[ 2] + t[19] * m[10])
                      - ( t[18] * m[10] + t[21] * m[14] + t[13] * m[ 2]));
    inv[14] = inv_det * ((t[18] * m[ 6] + t[23] * m[14] + t[15] * m[ 2])
                      - ( t[22] * m[14] + t[14] * m[ 2] + t[19] * m[ 6]));
    inv[15] = inv_det * ((t[22] * m[10] + t[16] * m[ 2] + t[21] * m[ 6])
                      - ( t[20] * m[ 6] + t[23] * m[10] + t[17] * m[ 2]));

    return result;
}

/*
    @brief Создает матрицу перемещения.
    @param translation Вектор перемещения.
    @return Матрица перемещения.
*/
INLINE mat4 mat4_translation(const vec3 translation)
{
    mat4 result = mat4_identity();
    result.data[12] = translation.x;
    result.data[13] = translation.y;
    result.data[14] = translation.z;

    return result;
}

/*
    @brief Создает матрицу масштабирования.
    @warning При нулевом масштабе матрица становится вырожденной.
    @param scale Вектор масштабирования.
    @return Матрица масштабирования.
*/
INLINE mat4 mat4_scale(const vec3 scale)
{
    mat4 result = mat4_identity();
    result.data[ 0] = scale.x;
    result.data[ 5] = scale.y;
    result.data[10] = scale.z;

    return result;
}

/*
    @brief Создает матрицу вращения вокруг оси X.
    @param angle_radians Угол поворота в радианах.
    @return Матрица вращения вокруг оси X.
*/
INLINE mat4 mat4_rotation_x(const f32 angle_radians)
{
    const f32 cos_a = math_cos(angle_radians);
    const f32 sin_a = math_sin(angle_radians);

    mat4 result = mat4_identity();
    result.data[ 5] =  cos_a;
    result.data[ 6] =  sin_a;
    result.data[ 9] = -sin_a;
    result.data[10] =  cos_a;

    return result;
}

/*
    @brief Создает матрицу вращения вокруг оси Y.
    @param angle_radians Угол поворота в радианах.
    @return Матрица вращения вокруг оси Y.
*/
INLINE mat4 mat4_rotation_y(const f32 angle_radians)
{
    const f32 cos_a = math_cos(angle_radians);
    const f32 sin_a = math_sin(angle_radians);

    mat4 result = mat4_identity();
    result.data[ 0] =  cos_a;
    result.data[ 2] = -sin_a;
    result.data[ 8] =  sin_a;
    result.data[10] =  cos_a;

    return result;
}

/*
    @brief Создает матрицу вращения вокруг оси Z.
    @param angle_radians Угол поворота в радианах.
    @return Матрица вращения вокруг оси Z.
*/
INLINE mat4 mat4_rotation_z(const f32 angle_radians)
{
    const f32 cos_a = math_cos(angle_radians);
    const f32 sin_a = math_sin(angle_radians);

    mat4 result = mat4_identity();
    result.data[0] =  cos_a;
    result.data[1] =  sin_a;
    result.data[4] = -sin_a;
    result.data[5] =  cos_a;

    return result;
}

/*
    @brief Создает матрицу вращения вокруг произвольной оси.
    @warning Ось должна быть единичной для корректного результата.
    @param axis Ось вращения (должна быть единичной).
    @param angle_radians Угол поворота в радианах.
    @return Матрица вращения.
*/
INLINE mat4 mat4_rotation_axis(const vec3 axis, const f32 angle_radians)
{
    const f32 cos_a = math_cos(angle_radians);
    const f32 sin_a = math_sin(angle_radians);
    const f32 one_minus_cos = 1.0f - cos_a;
    const f32 x = axis.x;
    const f32 y = axis.y;
    const f32 z = axis.z;

    mat4 result;
    result.data[ 0] = cos_a + x * x * one_minus_cos;
    result.data[ 1] = y * x * one_minus_cos + z * sin_a;
    result.data[ 2] = z * x * one_minus_cos - y * sin_a;
    result.data[ 3] = 0.0f;

    result.data[ 4] = x * y * one_minus_cos - z * sin_a;
    result.data[ 5] = cos_a + y * y * one_minus_cos;
    result.data[ 6] = z * y * one_minus_cos + x * sin_a;
    result.data[ 7] = 0.0f;

    result.data[ 8] = x * z * one_minus_cos + y * sin_a;
    result.data[ 9] = y * z * one_minus_cos - x * sin_a;
    result.data[10] = cos_a + z * z * one_minus_cos;
    result.data[11] = 0.0f;

    result.data[12] = 0.0f;
    result.data[13] = 0.0f;
    result.data[14] = 0.0f;
    result.data[15] = 1.0f;

    return result;
}

/*
    @brief Создает матрицу преобразования из позиции, вращения и масштаба (TRS).
    @note Порядок преобразований: масштаб → вращение → перемещение.
    @param translation Вектор перемещения.
    @param rotation Кватернион вращения.
    @param scale Вектор масштабирования.
    @return Матрица преобразования.
*/
INLINE mat4 mat4_from_trs(const vec3 translation, const quat rotation, const vec3 scale)
{
    const f32 x = rotation.x;
    const f32 y = rotation.y;
    const f32 z = rotation.z;
    const f32 w = rotation.w;

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

    mat4 result;
    result.data[ 0] = (1.0f - (yy + zz)) * scale.x;
    result.data[ 1] = (xy + wz) * scale.x;
    result.data[ 2] = (xz - wy) * scale.x;
    result.data[ 3] = 0.0f;

    result.data[ 4] = (xy - wz) * scale.y;
    result.data[ 5] = (1.0f - (xx + zz)) * scale.y;
    result.data[ 6] = (yz + wx) * scale.y;
    result.data[ 7] = 0.0f;

    result.data[ 8] = (xz + wy) * scale.z;
    result.data[ 9] = (yz - wx) * scale.z;
    result.data[10] = (1.0f - (xx + yy)) * scale.z;
    result.data[11] = 0.0f;

    result.data[12] = translation.x;
    result.data[13] = translation.y;
    result.data[14] = translation.z;
    result.data[15] = 1.0f;

    return result;
}

/*
    @brief Извлекает вектор перемещения из матрицы преобразования.
    @param matrix Матрица преобразования.
    @return Вектор перемещения.
*/
INLINE vec3 mat4_extract_translation(const mat4 matrix)
{
    return (vec3){{
        matrix.data[12],
        matrix.data[13],
        matrix.data[14]
    }};
}

/*
    @brief Извлекает вектор масштабирования из матрицы преобразования.
    @param matrix Матрица преобразования.
    @return Вектор масштабирования.
*/
INLINE vec3 mat4_extract_scale(const mat4 matrix)
{
    const vec3 x_axis = {{matrix.data[0], matrix.data[1], matrix.data[ 2]}};
    const vec3 y_axis = {{matrix.data[4], matrix.data[5], matrix.data[ 6]}};
    const vec3 z_axis = {{matrix.data[8], matrix.data[9], matrix.data[10]}};

    return (vec3){{
        vec3_length(x_axis),
        vec3_length(y_axis),
        vec3_length(z_axis)
    }};
}

/*
    @brief Извлекает кватернион вращения из матрицы преобразования.
    @param matrix Матрица преобразования.
    @return Кватернион вращения.
*/
INLINE quat mat4_extract_rotation(const mat4 matrix)
{
    const vec3 scale = mat4_extract_scale(matrix);

    if(scale.x < F32_EPSILON_CMP || scale.y < F32_EPSILON_CMP || scale.z < F32_EPSILON_CMP)
    {
        return quat_identity();
    }

    const f32 inv_scale_x = 1.0f / scale.x;
    const f32 inv_scale_y = 1.0f / scale.y;
    const f32 inv_scale_z = 1.0f / scale.z;

    mat4 rotation_mat = matrix;
    rotation_mat.data[ 0] *= inv_scale_x;
    rotation_mat.data[ 1] *= inv_scale_x;
    rotation_mat.data[ 2] *= inv_scale_x;

    rotation_mat.data[ 4] *= inv_scale_y;
    rotation_mat.data[ 5] *= inv_scale_y;
    rotation_mat.data[ 6] *= inv_scale_y;

    rotation_mat.data[ 8] *= inv_scale_z;
    rotation_mat.data[ 9] *= inv_scale_z;
    rotation_mat.data[10] *= inv_scale_z;

    return quat_from_mat4(rotation_mat);
}

/*
    @brief Возвращает вектор направления "вперед" из матрицы.
    @param matrix Матрица.
    @return Вектор направления вперед.
*/
INLINE vec3 mat4_forward(const mat4 matrix)
{
    vec3 forward = {{-matrix.data[2], -matrix.data[6], -matrix.data[10]}};
    vec3_normalize(&forward);
    return forward;
}

/*
    @brief Возвращает вектор направленный "назад" из матрицы.
    @param matrix Матрица.
    @return Вектор направления назад.
*/
INLINE vec3 mat4_backward(const mat4 matrix)
{
    vec3 backward = {{matrix.data[2], matrix.data[6], matrix.data[10]}};
    vec3_normalize(&backward);
    return backward;
}

/*
    @brief Возвращает вектор направления "вверх" из матрицы.
    @param matrix Матрица.
    @return Вектор направления вверх.
*/
INLINE vec3 mat4_up(const mat4 matrix)
{
    vec3 up = {{matrix.data[1], matrix.data[5], matrix.data[9]}};
    vec3_normalize(&up);
    return up;
}

/*
    @brief Возвращает вектор направления "вниз" из матрицы.
    @param matrix Матрица.
    @return Вектор направления вниз.
*/
INLINE vec3 mat4_down(const mat4 matrix)
{
    vec3 down = {{-matrix.data[1], -matrix.data[5], -matrix.data[9]}};
    vec3_normalize(&down);
    return down;
}

/*
    @brief Возвращает вектор направления "вправо" из матрицы.
    @param matrix Матрица.
    @return Вектор направления вправо.
*/
INLINE vec3 mat4_right(const mat4 matrix)
{
    vec3 right = {{matrix.data[0], matrix.data[4], matrix.data[8]}};
    vec3_normalize(&right);
    return right;
}

/*
    @brief Возвращает вектор направления "влево" из матрицы.
    @param matrix Матрица.
    @return Вектор направления влево.
*/
INLINE vec3 mat4_left(const mat4 matrix)
{
    vec3 left = {{-matrix.data[0], -matrix.data[4], -matrix.data[8]}};
    vec3_normalize(&left);
    return left;
}

/*
    @brief Умножает матрицу на 3D вектор (матрица слева).
    @param m Матрица.
    @param v Вектор.
    @return Преобразованный вектор.
*/
INLINE vec3 mat4_mul_vec3(const mat4 m, const vec3 v)
{
    return (vec3){{
        m.data[0] * v.x + m.data[4] * v.y + m.data[ 8] * v.z + m.data[12],
        m.data[1] * v.x + m.data[5] * v.y + m.data[ 9] * v.z + m.data[13],
        m.data[2] * v.x + m.data[6] * v.y + m.data[10] * v.z + m.data[14]
    }};
}

/*
    @brief Умножает матрицу на 4D вектор (матрица слева).
    @param m Матрица.
    @param v Вектор.
    @return Преобразованный вектор.
*/
INLINE vec4 mat4_mul_vec4(const mat4 m, const vec4 v)
{
    return (vec4){{
        m.data[0] * v.x + m.data[4] * v.y + m.data[8] * v.z + m.data[12] * v.w,
        m.data[1] * v.x + m.data[5] * v.y + m.data[9] * v.z + m.data[13] * v.w,
        m.data[2] * v.x + m.data[6] * v.y + m.data[10] * v.z + m.data[14] * v.w,
        m.data[3] * v.x + m.data[7] * v.y + m.data[11] * v.z + m.data[15] * v.w
    }};
}
