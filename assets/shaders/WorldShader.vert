#version 450

// Получение данных.
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;

// Передача данных в следующий шейдер.
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = in_color;

    // Четвертый параметр указывает: 1.0 - точка в пространстве, 0.0 - вектор направления.
    gl_Position = vec4(in_position, 1.0);
}
