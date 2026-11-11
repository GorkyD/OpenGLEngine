#pragma once

#include <string>
#include <vector>
#include "Ecs/Core/Entity.h"
#include "Math/Vector3.h"

class Engine;
class EcsWorld;

struct CampfireEntities
{
    Entity flame = 0;
    Entity sparks = 0;
    Entity light = 0;
    Vector3 position = {0, 0, 0};
};

class ForestBuilder
{
public:
    static float GroundHeight(EcsWorld& world, float x, float z);

    static std::vector<Entity> SpawnProp(Engine& engine, const std::string& path, float x, float z, float targetSize, float yawDegrees, float sink = 0.0f, bool fitLargestSide = false);
    static Entity AddBoxCollider(Engine& engine, const std::vector<Entity>& parts, float shrink = 0.0f);
    static Vector3 BoundsCenter(EcsWorld& world, const std::vector<Entity>& parts);
    static void Bounds(EcsWorld& world, const std::vector<Entity>& parts, Vector3& outMin, Vector3& outMax);

    static CampfireEntities BuildCampfire(Engine& engine, const Vector3& position, float intensity);
    static void PlaceCampItems(Engine& engine);
    static void ScatterForest(Engine& engine, unsigned int seed, float clearingRadius);
    static void ApplyPointFiltering(Engine& engine);
};
