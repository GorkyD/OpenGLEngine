#version 410 core

layout(location = 0) in vec3 position;

uniform mat4 world;
uniform mat4 lightSpaceMatrix;

void main()
{
    gl_Position = lightSpaceMatrix * world * vec4(position, 1.0);
}
