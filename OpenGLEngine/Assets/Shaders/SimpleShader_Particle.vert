#version 410 core

uniform UniformData
{
    mat4 world;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in float alpha;

out vec2 fragUv;
out float fragAlpha;

void main()
{
    gl_Position = projection * view * vec4(position, 1.0);
    fragUv = uv;
    fragAlpha = alpha;
}
