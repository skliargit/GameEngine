#include "renderer/camera.h"

#include "math/vector.h"
#include "math/matrix.h"
#include "math/quaternion.h"

static void update_vectors(camera* cam)
{
    // Вращение векторов.
    cam->forward = quat_rotate_vec3(vec3_forward(), cam->orientation);
    cam->right   = quat_rotate_vec3(vec3_right(), cam->orientation);
    cam->up      = quat_rotate_vec3(vec3_up(), cam->orientation);

    // Нормализация.
    vec3_normalize(&cam->forward);
    vec3_normalize(&cam->right);
    vec3_normalize(&cam->up);
}

camera camera_init(vec3 position)
{
    camera cam = {
        .position         = position,
        .orientation      = quat_identity(),
        .wait_update_view = true,
    };

    update_vectors(&cam);
    return cam;
}

void camera_move_forward(camera* cam, f32 speed)
{
    cam->position = vec3_add(cam->position, vec3_mul_scalar(cam->forward, speed));
    cam->wait_update_view = true;
}

void camera_move_backward(camera* cam, f32 speed)
{
    camera_move_forward(cam, -speed);    
}

void camera_move_right(camera* cam, f32 speed)
{
    cam->position = vec3_add(cam->position, vec3_mul_scalar(cam->right, speed));
    cam->wait_update_view = true;
}

void camera_move_left(camera* cam, f32 speed)
{
    camera_move_right(cam, -speed);
}

void camera_move_up(camera* cam, f32 speed)
{
    // cam->position = vec3_add(cam->position, vec3_mul_scalar(cam->up, speed));
    cam->position.y += speed;
    cam->wait_update_view = true;
}

void camera_move_down(camera* cam, f32 speed)
{
    camera_move_up(cam, -speed);
}

void camera_rotate(camera* cam, f32 yaw_deg, f32 pitch_deg)
{
    // Преобразование в радианы.
    cam->yaw_rad += math_deg_to_rad(yaw_deg);
    cam->pitch_rad += math_deg_to_rad(pitch_deg);

    // Нормализация горизонтального вращения.
    cam->yaw_rad = math_angle_normalize(cam->yaw_rad, ANGLE_RANGE_ZERO_TO_2PI);

    // Ограничение вертикального вращения.
    cam->pitch_rad = CLAMP(cam->pitch_rad, CAMERA_MIN_PITCH_DEG * MATH_DEG_TO_RAD, CAMERA_MAX_PITCH_DEG * MATH_DEG_TO_RAD);

    // Создание кватернионов вращения.
    quat yaw_rotation = quat_from_axis_angle(vec3_up(), cam->yaw_rad);
    quat pitch_rotation = quat_from_axis_angle(vec3_right(), cam->pitch_rad);

    // Применение поворотов к текущей ориентации.
    cam->orientation = quat_mul(yaw_rotation, pitch_rotation);

    // Нормализация для предотвращения дрейфа.
    quat_normalize(&cam->orientation);

    // Обновление векторов.
    update_vectors(cam);

    cam->wait_update_view = true;
}

void camera_look_at(camera* cam, vec3 target)
{
    // Полученеи матрицы вида и ее ориентации.
    cam->view = mat4_look_at(cam->position, target, vec3_up());
    cam->orientation = quat_from_mat4(cam->view);

    // Обновление векторов.
    update_vectors(cam);

    cam->wait_update_view = false;
}

mat4 camera_get_view(camera* cam)
{
    if(cam->wait_update_view)
    {
        vec3 target = vec3_add(cam->position, cam->forward);
        cam->view = mat4_look_at(cam->position, target, vec3_up());
        cam->wait_update_view = false;
    }

    return cam->view;
}
