#version 410 core

uniform UniformData
{
    mat4 world;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

out vec3 fragColor;

void main()
{
    gl_Position = projection * view * vec4(position, 1.0);
    fragColor = color;
}
