#pragma once

#include <core/defines.h>

// @brief Вектор из 2х элементов c плавающей точкой.
typedef union {
    // @brief Массив из 2х элементов.
    f32 elements[2];
    struct {
        union {
            // @brief Первый элемент.
            f32 x, r, u, re;
        };
        union {
            // @brief Второй элемент.
            f32 y, t, v, im;
        };
    };
} vec2;

// @brief Вектор из 3х элементов c плавающей точкой.
typedef union {
    // @brief Массив из 3х элементов.
    f32 elements[3];
    struct {
        union {
            // @brief Первый элемент.
            f32 x, r, u, n, i;
        };
        union {
            // @brief Второй элемент.
            f32 y, g, v, t, j;
        };
        union {
            // @brief Третий элемент.
            f32 z, b, w, l, k;
        };
    };
} vec3;

// @brief Вектор из 4х элементов c плавающей точкой.
typedef union {
    // @brief Массив из 4х элементов.
    f32 elements[4];
    struct {
        union {
            // @brief Первый элемент.
            f32 x, r, s;
        };
        union {
            // @brief Второй элемент.
            f32 y, g, t;
        };
        union {
            // @brief Третий элемент.
            f32 z, b, p, width;
        };
        union {
            // @brief Четвертый элемент.
            f32 w, a, q, height;
        };
    };
} vec4;

/*
    @brief Кватернион вращения (w, x, y, z), где w - скалярная часть, (x, y, z) - векторная часть.
    @note Используется для интерполяции вращений (slerp), избегания Gimbal Lock.
*/
typedef vec4 quat;

// @brief Матрица 4х4 из элементов с плавающей точкой.
typedef union {
    // @brief Массив из 16ти элементов с плавающей точкой.
    f32 data[16];
} mat4;

// @brief Представляет вершину в двухмерном пространстве.
typedef struct {
    // @brief Пространственные координаты.
    vec2 position;
    // @brief Текстурные координаты.
    vec2 texcoord;
    // @brief Цвет RGBA.
    vec4 color;
} vertex2d;

// @brief Представляет вершину в трехмерном пространстве.
typedef struct {
    // @brief Пространственные координаты.
    vec3 position;
    // @brief Текстурные координаты.
    // vec2f texcoord;
    // @brief Цвет RGBA.
    vec4 color;
} vertex3d;
