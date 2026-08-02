#version 410 core

uniform mat4 projection;

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

out vec4 fragColor;

void main()
{
    gl_Position = projection * vec4(position, 0.0, 1.0);
    fragColor = color;
}
