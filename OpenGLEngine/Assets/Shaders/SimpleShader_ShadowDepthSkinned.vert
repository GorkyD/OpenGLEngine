#version 410 core

#define MAX_BONES 100

layout(location = 0) in vec3 position;
layout(location = 3) in vec4 boneIds;
layout(location = 4) in vec4 boneWeights;

uniform mat4 world;
uniform mat4 lightSpaceMatrix;
uniform mat4 boneMatrices[MAX_BONES];

void main()
{
    float weightSum = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;

    vec4 skinnedPosition = vec4(position, 1.0);

    if (weightSum > 0.0001)
    {
        mat4 skinMatrix = boneWeights.x * boneMatrices[int(boneIds.x)] + boneWeights.y * boneMatrices[int(boneIds.y)] + boneWeights.z * boneMatrices[int(boneIds.z)] +
                           boneWeights.w * boneMatrices[int(boneIds.w)];
        skinnedPosition = skinMatrix * vec4(position, 1.0);
    }

    gl_Position = lightSpaceMatrix * world * skinnedPosition;
}
