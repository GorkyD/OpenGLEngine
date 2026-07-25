#version 410 core

uniform UniformData
{
    mat4 world;
    mat4 view;
    mat4 projection;
};

uniform float time;

layout(location = 0) in vec3 position;

out float heightFactor;

void main()
{
    float wobble = position.y * position.y;
    vec3 animatedPos = position;
    animatedPos.x += sin(time * 9.0 + position.y * 5.0) * 0.06 * wobble;
    animatedPos.z += cos(time * 7.0 + position.y * 4.0) * 0.06 * wobble;

    vec4 worldPos = world * vec4(animatedPos, 1.0);
    gl_Position = projection * view * worldPos;

    heightFactor = position.y;
}
