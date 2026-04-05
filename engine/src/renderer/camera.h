#pragma once

#include <math/types.h>

// @brief Ограничения камеры по вертикали.
#define CAMERA_MAX_PITCH_DEG  89.0f
#define CAMERA_MIN_PITCH_DEG -89.0f

/**
    @brief 
*/
typedef struct camera {
    // ------------------ Ядро камеры ------------------
    vec3 position;            /**< @brief Позиция камеры в мировых координатах (x, y, z).   */
    f32 yaw_rad;              /**< @brief Текущее значение горизонтального угла в радианах. */
    f32 pitch_rad;            /**< @brief Текущее значение вертикального угла в радианах.   */
    bool wait_update_view;    /**< @brief Флаг ожидания обновления вида.                    */

    // ------------------ Кешированные -----------------
    quat orientation;         /**< @brief Кватернион вращения.                              */
    vec3 forward;             /**< @brief Вектор направления вперед (нормализован).         */
    vec3 right;               /**< @brief Вектор направления вправо (нормализован).         */
    vec3 up;                  /**< @brief Вектор направления вверх (нормализован).          */
    mat4 view;                /**< @brief Матрица вида камеры.                              */
} camera;

/**
    @brief Инициализация камеры.
*/
CORE_API camera camera_init(vec3 position);

/**
    @brief Движение камеры вперед.
*/
CORE_API void camera_move_forward(camera* cam, f32 speed);

/**
    @brief Движение камеры назад.
*/
CORE_API void camera_move_backward(camera* cam, f32 speed);

/**
    @brief Движение камеры вправо.
*/
CORE_API void camera_move_right(camera* cam, f32 speed);

/**
    @brief Движение камеры влево.
*/
CORE_API void camera_move_left(camera* cam, f32 speed);

/**
    @brief Движение камеры вверх.
*/
CORE_API void camera_move_up(camera* cam, f32 speed);

/**
    @brief Движение камеры назад.
*/
CORE_API void camera_move_down(camera* cam, f32 speed);

/**
    @brief Поворачивает камеру на дельты углов (в градусах).
    @param cam Указатель на камеру, которую необходимо повернуть.
    @param yaw_deg Поворот камеры вокруг Y на заданный угол (направление от X->Z).
    @param pitch_deg Поворот камеры вокруг X на заданный угол (направление от Y->Z).
*/
CORE_API void camera_rotate(camera* cam, f32 yaw_deg, f32 pitch_deg);

/**
    @brief Направляет камеру на целевую точку.
*/
CORE_API void camera_look_at(camera* cam, vec3 target);

/**
    @brief Получает указатель на матрицу вида.
*/
CORE_API mat4 camera_get_view(camera* cam);
