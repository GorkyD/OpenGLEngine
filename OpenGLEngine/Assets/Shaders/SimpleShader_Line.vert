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
    vec4 worldPos = world * vec4(position, 1.0);
    gl_Position = projection * view * worldPos;
    fragColor = color;
}
