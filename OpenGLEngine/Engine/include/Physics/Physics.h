#pragma once

#include "Ecs/Core/Entity.h"
#include "Math/Vector3.h"
#include "Physics/PhysicsLayers.h"

class EcsWorld;

struct RaycastHit
{
    Entity entity = INVALID_ENTITY;
    Vector3 point = {0, 0, 0};
    Vector3 normal = {0, 0, 0};
    float distance = 0.0f;
};

struct RaycastQuery
{
    Vector3 origin = {0, 0, 0};
    Vector3 direction = {0, 0, 1};
    float maxDistance = 1000.0f;

    unsigned int layerMask = PhysicsLayers::All;
    Entity ignoreEntity = INVALID_ENTITY;
};

struct BoxCastQuery
{
    Vector3 origin = {0, 0, 0};
    Vector3 halfExtents = {0.5f, 0.5f, 0.5f};
    Vector3 direction = {0, -1, 0};
    float maxDistance = 1.0f;

    unsigned int layerMask = PhysicsLayers::All;
    Entity ignoreEntity = INVALID_ENTITY;
};

struct OverlapResult
{
    Entity entity = INVALID_ENTITY;
    Vector3 penetrationNormal = {0, 0, 0};
    float penetrationDepth = 0.0f;
};

class Physics
{
public:
    static bool Raycast(EcsWorld& world, const RaycastQuery& query, RaycastHit& outHit);
    static bool BoxCast(EcsWorld& world, const BoxCastQuery& query, RaycastHit& outHit);
    static bool OverlapBox(EcsWorld& world, const Vector3& center, const Vector3& halfExtents, unsigned int layerMask, Entity ignoreEntity, OverlapResult& outResult);

    static bool RayIntersectsBox(const Vector3& origin, const Vector3& direction, const Vector3& boxMin, const Vector3& boxMax, float maxDistance, float& outDistance,
                                  Vector3& outNormal);
};
