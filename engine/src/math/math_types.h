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
} vec2f;

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
} vec3f;

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
} vec4f;

// @brief Представляет вершину в двухмерном пространстве.
typedef struct {
    // @brief Пространственные координаты.
    vec2f position;
    // @brief Текстурные координаты.
    vec2f texcoord;
    // @brief Цвет RGBA.
    vec4f color;
} vertex2d;

// @brief Представляет вершину в трехмерном пространстве.
typedef struct {
    // @brief Пространственные координаты.
    vec3f position;
    // @brief Текстурные координаты.
    // vec2f texcoord;
    // @brief Цвет RGBA.
    vec4f color;
} vertex3d;
