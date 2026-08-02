#include "Physics/Physics.h"
#include <algorithm>
#include <cmath>
#include "Ecs/Components/ColliderComponent.h"
#include "Ecs/Core/EcsWorld.h"

bool Physics::RayIntersectsBox(const Vector3& origin, const Vector3& direction, const Vector3& boxMin, const Vector3& boxMax, float maxDistance, float& outDistance,
                                Vector3& outNormal)
{
    const float originArray[3] = {origin.x, origin.y, origin.z};
    const float directionArray[3] = {direction.x, direction.y, direction.z};
    const float minArray[3] = {boxMin.x, boxMin.y, boxMin.z};
    const float maxArray[3] = {boxMax.x, boxMax.y, boxMax.z};

    float tMin = 0.0f;
    float tMax = maxDistance;

    int entryAxis = 0;
    float entrySign = -1.0f;

    for (int axis = 0; axis < 3; axis++)
    {
        if (std::fabs(directionArray[axis]) < 0.0000001f)
        {
            if (originArray[axis] < minArray[axis] || originArray[axis] > maxArray[axis])
                return false;
            continue;
        }

        const float inverse = 1.0f / directionArray[axis];
        float tNear = (minArray[axis] - originArray[axis]) * inverse;
        float tFar = (maxArray[axis] - originArray[axis]) * inverse;

        float sign = -1.0f;
        if (tNear > tFar)
        {
            std::swap(tNear, tFar);
            sign = 1.0f;
        }

        if (tNear > tMin)
        {
            tMin = tNear;
            entryAxis = axis;
            entrySign = sign;
        }

        tMax = (std::min)(tMax, tFar);

        if (tMin > tMax)
            return false;
    }

    outDistance = tMin;

    outNormal = {0, 0, 0};
    if (entryAxis == 0)
        outNormal.x = entrySign;
    else if (entryAxis == 1)
        outNormal.y = entrySign;
    else
        outNormal.z = entrySign;

    return true;
}

bool Physics::BoxCast(EcsWorld& world, const BoxCastQuery& query, RaycastHit& outHit)
{
    const Vector3 direction = Vector3::Normalize(query.direction);

    bool found = false;
    float closest = query.maxDistance;

    for (auto& [entity, collider] : world.GetPool<ColliderComponent>())
    {
        if (entity == query.ignoreEntity || collider.isTrigger)
            continue;
        if ((collider.layer & query.layerMask) == 0)
            continue;

        const Vector3 expandedMin = collider.worldMin - query.halfExtents;
        const Vector3 expandedMax = collider.worldMax + query.halfExtents;

        float distance = 0.0f;
        Vector3 normal = {0, 0, 0};

        if (!RayIntersectsBox(query.origin, direction, expandedMin, expandedMax, closest, distance, normal))
            continue;

        if (distance > closest)
            continue;

        closest = distance;
        found = true;

        outHit.entity = entity;
        outHit.distance = distance;
        outHit.normal = normal;
        outHit.point = query.origin + direction * distance;
    }

    return found;
}

bool Physics::OverlapBox(EcsWorld& world, const Vector3& center, const Vector3& halfExtents, unsigned int layerMask, Entity ignoreEntity, OverlapResult& outResult)
{
    const Vector3 boxMin = center - halfExtents;
    const Vector3 boxMax = center + halfExtents;

    bool found = false;
    float deepest = 0.0f;

    for (auto& [entity, collider] : world.GetPool<ColliderComponent>())
    {
        if (entity == ignoreEntity || collider.isTrigger)
            continue;
        if ((collider.layer & layerMask) == 0)
            continue;

        const float overlapX = (std::min)(boxMax.x, collider.worldMax.x) - (std::max)(boxMin.x, collider.worldMin.x);
        const float overlapY = (std::min)(boxMax.y, collider.worldMax.y) - (std::max)(boxMin.y, collider.worldMin.y);
        const float overlapZ = (std::min)(boxMax.z, collider.worldMax.z) - (std::max)(boxMin.z, collider.worldMin.z);

        if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f)
            continue;

        const Vector3 colliderCenter = (collider.worldMin + collider.worldMax) * 0.5f;

        Vector3 normal = {0, 0, 0};
        float depth = overlapX;

        if (overlapX <= overlapY && overlapX <= overlapZ)
            normal.x = center.x < colliderCenter.x ? -1.0f : 1.0f;
        else if (overlapY <= overlapX && overlapY <= overlapZ)
        {
            depth = overlapY;
            normal.y = center.y < colliderCenter.y ? -1.0f : 1.0f;
        }
        else
        {
            depth = overlapZ;
            normal.z = center.z < colliderCenter.z ? -1.0f : 1.0f;
        }

        if (depth <= deepest)
            continue;

        deepest = depth;
        found = true;

        outResult.entity = entity;
        outResult.penetrationNormal = normal;
        outResult.penetrationDepth = depth;
    }

    return found;
}

bool Physics::Raycast(EcsWorld& world, const RaycastQuery& query, RaycastHit& outHit)
{
    const Vector3 direction = Vector3::Normalize(query.direction);

    bool found = false;
    float closest = query.maxDistance;

    for (auto& [entity, collider] : world.GetPool<ColliderComponent>())
    {
        if (entity == query.ignoreEntity)
            continue;
        if ((collider.layer & query.layerMask) == 0)
            continue;

        float distance = 0.0f;
        Vector3 normal = {0, 0, 0};

        if (!RayIntersectsBox(query.origin, direction, collider.worldMin, collider.worldMax, closest, distance, normal))
            continue;

        if (distance > closest)
            continue;

        closest = distance;
        found = true;

        outHit.entity = entity;
        outHit.distance = distance;
        outHit.normal = normal;
        outHit.point = query.origin + direction * distance;
    }

    return found;
}
