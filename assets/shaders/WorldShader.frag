#version 450

// Должно соответствовать color_blend_state.pAttachments = ..., 0 индексу массива.
layout(location = 0) out vec4 out_color;

// Полученные данные из вершинного шейдера.
layout(location = 0) in vec4 in_color;

void main()
{
    out_color = in_color;
}
