#pragma once

#include <string>
#include <vector>
#include "Math/Vector3.h"
#include "Math/Vector4.h"

class EcsWorld;
class RenderEngine;
class Engine;

struct PlacedObject
{
    std::string assetPath;
    std::string name;
    std::string parentName;
    std::string attachBone;
    std::string visibility;

    Vector3 position = {0, 0, 0};
    Vector3 rotationDegrees = {0, 0, 0};
    Vector3 scale = {1, 1, 1};

    bool hasCollider = false;
    Vector3 colliderCenter = {0, 0, 0};
    Vector3 colliderSize = {1, 1, 1};
    bool colliderStatic = false;
    bool colliderTrigger = false;
    unsigned int colliderLayer = 1u;
    unsigned int colliderMask = ~0u;
    float colliderRestitution = 0.3f;

    bool hasMaterial = false;
    Vector4 diffuseColor = {1, 1, 1, 1};
    Vector3 emissiveColor = {0, 0, 0};
    float emissiveIntensity = 0.0f;
    float roughness = 1.0f;
    float metallic = 0.0f;
};

struct PlacedLight
{
    int type = 0;
    Vector3 color = {1, 1, 1};
    Vector3 direction = {0, 1, 0};
    Vector3 position = {0, 0, 0};
    float intensity = 1.0f;
    float range = 10.0f;

    bool castShadows = false;
    float shadowOrthoSize = 25.0f;
    float shadowDistance = 40.0f;
    float shadowBias = 0.0025f;
    float shadowAmbientOcclusion = 0.4f;
    float shadowFocusDistance = 0.0f;
    float shadowNormalBias = 0.05f;

    float innerConeAngleDeg = 20.0f;
    float outerConeAngleDeg = 30.0f;
};

struct PlacedAmbient
{
    Vector3 color = {1, 1, 1};
    float intensity = 0.2f;
};

struct SceneData
{
    std::vector<PlacedObject> objects;
    std::vector<PlacedLight> lights;
    std::vector<PlacedAmbient> ambients;

    bool Empty() const
    {
        return objects.empty() && lights.empty() && ambients.empty();
    }
};

class SceneSerializer
{
public:
    static std::string GetScenePath(const std::string& sceneName, const std::string& ownerName);
    static std::string GetSourceScenePath(const std::string& sceneName, const std::string& ownerName, const std::string& sourceAssetsPath);

    static bool Save(Engine& engine, const std::string& sceneName);
    static bool Load(Engine& engine, const std::string& sceneName);

    static SceneData ReadFile(const std::string& sceneName, const std::string& ownerName, const std::string& sourceAssetsPath);

    static std::string MakeUniqueGroupName(EcsWorld& world, const std::string& assetPath);
};
