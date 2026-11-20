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

out vec3 hullNormal;
out vec3 hullPosition;
out vec3 cameraPosition;

void main()
{
    vec3 inflated = position + normalize(normal) * outlineWidth;
    vec4 worldPosition = world * vec4(inflated, 1.0);
    hullPosition = worldPosition.xyz;
    hullNormal = mat3(world) * normal;
    cameraPosition = inverse(view)[3].xyz;
    gl_Position = projection * view * worldPosition;
}
