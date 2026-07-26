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

layout(location = 3) in vec4 instWorld0;
layout(location = 4) in vec4 instWorld1;
layout(location = 5) in vec4 instWorld2;
layout(location = 6) in vec4 instWorld3;
layout(location = 7) in vec4 instDiffuseColor;
layout(location = 8) in vec4 instEmissiveColorIntensity;

out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragWorldPos;
out vec4 fragDiffuseColor;
out vec3 fragEmissiveColor;
out float fragEmissiveIntensity;

void main()
{
    mat4 instanceWorld = mat4(instWorld0, instWorld1, instWorld2, instWorld3);

    vec4 worldPos = instanceWorld * vec4(position, 1.0);
    gl_Position = projection * view * worldPos;

    mat3 normalMatrix = transpose(inverse(mat3(instanceWorld)));

    fragTexCoord = texcoord;
    fragNormal = normalMatrix * normal;
    fragWorldPos = worldPos.xyz;
    fragDiffuseColor = instDiffuseColor;
    fragEmissiveColor = instEmissiveColorIntensity.xyz;
    fragEmissiveIntensity = instEmissiveColorIntensity.w;
}
