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

layout(location = 8) in vec4 instWorld0;
layout(location = 9) in vec4 instWorld1;
layout(location = 10) in vec4 instWorld2;
layout(location = 11) in vec4 instWorld3;
layout(location = 12) in vec4 instDiffuseColor;
layout(location = 13) in vec4 instEmissiveColorIntensity;
layout(location = 14) in vec4 instMaterialParams;

out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragWorldPos;
out vec4 fragDiffuseColor;
out vec3 fragEmissiveColor;
out float fragEmissiveIntensity;
out float fragRoughness;
out float fragMetallic;

uniform vec2 psxSnapGrid;

void main()
{
    mat4 instanceWorld = mat4(instWorld0, instWorld1, instWorld2, instWorld3);

    vec4 worldPos = instanceWorld * vec4(position, 1.0);
    gl_Position = projection * view * worldPos;

    if (psxSnapGrid.y > 0.0 && gl_Position.w > 0.0)
    {
        vec2 grid = psxSnapGrid * 0.5;
        gl_Position.xy = round(gl_Position.xy / gl_Position.w * grid) / grid * gl_Position.w;
    }

    mat3 normalMatrix = transpose(inverse(mat3(instanceWorld)));

    fragTexCoord = texcoord;
    fragNormal = normalMatrix * normal;
    fragWorldPos = worldPos.xyz;
    fragDiffuseColor = instDiffuseColor;
    fragEmissiveColor = instEmissiveColorIntensity.xyz;
    fragEmissiveIntensity = instEmissiveColorIntensity.w;
    fragRoughness = instMaterialParams.x;
    fragMetallic = instMaterialParams.y;
}
