#version 410 core

uniform UniformData
{
    mat4 world;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec3 normal;

uniform float outlineWidth;

void main()
{
    vec3 inflated = position + normalize(normal) * outlineWidth;
    gl_Position = projection * view * world * vec4(inflated, 1.0);
}
