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

out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragWorldPos;

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

    fragTexCoord = texcoord;
    fragNormal = normalMatrix * skinnedNormal;
    fragWorldPos = worldPos.xyz;
}
