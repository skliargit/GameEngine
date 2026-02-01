#version 450

// Вершинные атрибуты (обновляется на каждую вершину).
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;

// Передача данных в следующий шейдер (фрагментный).
layout(location = 0) out vec4 out_color;

// Набор дескрипторов (обновляется на каждый кадр).
layout(set = 0, binding = 0) uniform camera_ubo {
    mat4 proj;       // Матрица проекции камеры.
    mat4 view;       // Матрица вида камеры.
} camera;

// Push константы (обновляется на каждый объект/команду рисования).
// layout(push_constant) uniform model_ubo {
//     mat4 transform;  // Матрица трансформации объекта.
// } model;

void main()
{
    out_color = in_color;

    // Четвертый параметр указывает: 1.0 - точка в пространстве, 0.0 - вектор направления.
    // gl_Position = camera.proj * camera.view * model.transform * vec4(in_position, 1.0);
    gl_Position = camera.proj * camera.view * vec4(in_position, 1.0);
}
