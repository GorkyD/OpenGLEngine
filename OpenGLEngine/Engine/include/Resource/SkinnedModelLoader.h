#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "Math/Matrix4.h"
#include "Math/Quaternion.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Resource/ModelLoader.h"

struct SkinnedVertex
{
    Vector3 position;
    Vector2 uv;
    Vector3 normal;
    float boneIds[4] = {0, 0, 0, 0};
    float boneWeights[4] = {0, 0, 0, 0};
};

struct SkinnedMeshData
{
    std::vector<SkinnedVertex> vertices;
    std::vector<unsigned int> indices;
    int materialIndex = -1;
};

struct BoneInfo
{
    std::string name;
    Matrix4 offsetMatrix;
    int nodeIndex = -1;
};

struct SkeletonNode
{
    std::string name;
    Matrix4 localBindTransform;
    Vector3 bindPosition;
    Quaternion bindRotation;
    Vector3 bindScale = {1, 1, 1};
    int parentIndex = -1;
};

struct Skeleton
{
    std::vector<SkeletonNode> nodes;
    std::vector<BoneInfo> bones;
    std::unordered_map<std::string, int> boneNameToIndex;
    std::unordered_map<std::string, int> nodeNameToIndex;
    Matrix4 globalInverseTransform;
};

struct VectorKey
{
    float time = 0;
    Vector3 value;
};

struct QuaternionKey
{
    float time = 0;
    Quaternion value;
};

struct AnimationChannel
{
    std::string nodeName;
    std::vector<VectorKey> positionKeys;
    std::vector<QuaternionKey> rotationKeys;
    std::vector<VectorKey> scaleKeys;
};

struct AnimationClip
{
    std::string name;
    float durationTicks = 0;
    float ticksPerSecond = 25.0f;
    std::vector<AnimationChannel> channels;
};

struct SkinnedModelData
{
    std::vector<SkinnedMeshData> meshes;
    std::vector<MaterialData> materials;
    Skeleton skeleton;
    std::vector<AnimationClip> animations;
};

class SkinnedModelLoader
{
public:
    static SkinnedModelData Load(const std::string& filePath);
};
