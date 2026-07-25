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
out vec3 localPos;

void main()
{
    float wobble = position.y * position.y;
    float angle0 = atan(position.z, position.x);

    float sway = sin(time * 9.0 + position.y * 5.0 + angle0 * 2.0) * 0.06
               + sin(time * 17.0 + position.y * 11.0 + angle0 * 3.0) * 0.03;
    float sway2 = cos(time * 7.3 + position.y * 4.2 + angle0 * 2.0) * 0.06
                + cos(time * 15.0 + position.y * 9.0 + angle0 * 3.0) * 0.03;

    float twist = sin(time * 3.0 + position.y * 2.0) * 0.35 * position.y;
    float cosT = cos(twist);
    float sinT = sin(twist);
    vec3 twisted = vec3(position.x * cosT - position.z * sinT, position.y, position.x * sinT + position.z * cosT);

    vec3 animatedPos = twisted;
    animatedPos.x += sway * wobble;
    animatedPos.z += sway2 * wobble;
    animatedPos.y += sin(time * 12.0 + position.x * 6.0 + angle0) * 0.02 * wobble;

    vec4 worldPos = world * vec4(animatedPos, 1.0);
    gl_Position = projection * view * worldPos;

    heightFactor = position.y;
    localPos = position;
}
