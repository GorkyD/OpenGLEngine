#version 410 core

uniform UniformData
{
    mat4 world;
    mat4 view;
    mat4 projection;
};

uniform mat3 normalMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec3 normal;

out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragWorldPos;
out vec4 fragVertexColor;

uniform vec2 psxSnapGrid;

void main()
{
    vec4 worldPos = world * vec4(position, 1.0);
    gl_Position = projection * view * worldPos;

    if (psxSnapGrid.y > 0.0 && gl_Position.w > 0.0)
    {
        vec2 grid = psxSnapGrid * 0.5;
        gl_Position.xy = round(gl_Position.xy / gl_Position.w * grid) / grid * gl_Position.w;
    }

    fragTexCoord = texcoord;
    fragNormal = normalMatrix * normal;
    fragWorldPos = worldPos.xyz;
    fragVertexColor = vec4(1.0);
}
