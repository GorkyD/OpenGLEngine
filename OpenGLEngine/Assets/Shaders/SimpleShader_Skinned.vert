#version 410 core

#define MAX_BONES 100

uniform UniformData
{
    mat4 world;
    mat4 view;
    mat4 projection;
};

uniform mat3 normalMatrix;
uniform mat4 boneMatrices[MAX_BONES];

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 boneIds;
layout(location = 4) in vec4 boneWeights;
layout(location = 5) in vec4 vertexColor;

out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragWorldPos;
out vec4 fragVertexColor;

uniform vec2 psxSnapGrid;

void main()
{
    float weightSum = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;

    vec4 skinnedPosition = vec4(position, 1.0);
    vec3 skinnedNormal = normal;

    if (weightSum > 0.0001)
    {
        mat4 skinMatrix = boneWeights.x * boneMatrices[int(boneIds.x)] + boneWeights.y * boneMatrices[int(boneIds.y)] + boneWeights.z * boneMatrices[int(boneIds.z)] +
                           boneWeights.w * boneMatrices[int(boneIds.w)];

        skinnedPosition = skinMatrix * vec4(position, 1.0);
        skinnedNormal = mat3(skinMatrix) * normal;
    }

    vec4 worldPos = world * skinnedPosition;
    gl_Position = projection * view * worldPos;

    if (psxSnapGrid.y > 0.0 && gl_Position.w > 0.0)
    {
        vec2 grid = psxSnapGrid * 0.5;
        gl_Position.xy = round(gl_Position.xy / gl_Position.w * grid) / grid * gl_Position.w;
    }

    fragTexCoord = texcoord;
    fragNormal = normalMatrix * skinnedNormal;
    fragWorldPos = worldPos.xyz;
    fragVertexColor = vertexColor;
}
