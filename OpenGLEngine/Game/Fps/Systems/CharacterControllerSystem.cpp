#include "Systems/CharacterControllerSystem.h"
#include <algorithm>
#include "Ecs/Components/ColliderComponent.h"
#include "Ecs/Components/TerrainComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Physics/Physics.h"

void CharacterControllerSystem::Configure(EcsWorld& world, Entity controlledEntity)
{
    entity = controlledEntity;

    if (!world.HasComponent<ColliderComponent>(entity))
        return;

    const auto& collider = world.GetComponent<ColliderComponent>(entity);
    halfExtents = collider.size * 0.5f;
    colliderOffset = collider.center;
}

Vector3 CharacterControllerSystem::BoxCenter(EcsWorld& world) const
{
    return world.GetComponent<TransformComponent>(entity).position + colliderOffset;
}

void CharacterControllerSystem::SetBoxCenter(EcsWorld& world, const Vector3& center) const
{
    world.GetComponent<TransformComponent>(entity).position = center - colliderOffset;
}

void CharacterControllerSystem::Update(EcsWorld& world, const Vector3& wishDirection, bool wantsJump, bool sprinting, float deltaTime)
{
    if (entity == INVALID_ENTITY || !world.HasComponent<TransformComponent>(entity))
        return;

    const float speed = walkSpeed * (sprinting ? sprintMultiplier : 1.0f);
    velocity.x = wishDirection.x * speed;
    velocity.z = wishDirection.z * speed;

    if (grounded && wantsJump)
    {
        velocity.y = jumpSpeed;
        grounded = false;
    }
    else if (grounded)
    {
        velocity.y = 0.0f;
    }
    else
    {
        velocity.y = (std::max)(terminalVelocity, velocity.y + gravity * deltaTime);
    }

    ResolveOverlaps(world);

    Vector3 center = BoxCenter(world);

    const Vector3 horizontalMove = {velocity.x * deltaTime, 0.0f, velocity.z * deltaTime};
    const Vector3 beforeHorizontal = center;
    center = SweepAndSlide(world, center, horizontalMove, 4);

    const bool blocked = (center - beforeHorizontal).LengthSquared() < horizontalMove.LengthSquared() * 0.25f;
    if (grounded && blocked && horizontalMove.LengthSquared() > 0.000001f)
    {
        Vector3 stepped = beforeHorizontal;
        if (TryStepUp(world, stepped, horizontalMove))
            center = stepped;
    }

    const Vector3 verticalMove = {0.0f, velocity.y * deltaTime, 0.0f};
    const Vector3 beforeVertical = center;
    center = SweepAndSlide(world, center, verticalMove, 2);

    const float verticalTravel = center.y - beforeVertical.y;
    if (velocity.y < 0.0f && verticalTravel > verticalMove.y + 0.0001f)
    {
        grounded = true;
        velocity.y = 0.0f;
    }
    else if (velocity.y > 0.0f && verticalTravel < verticalMove.y - 0.0001f)
    {
        velocity.y = 0.0f;
    }
    else if (velocity.y < 0.0f)
    {
        grounded = false;
    }

    if (!grounded && velocity.y <= 0.0f)
        SnapToGround(world, center);

    for (auto& pair : world.GetPool<TerrainComponent>())
    {
        const float terrainY = pair.second.GetHeightAt(center.x, center.z);
        const float feetY = center.y - halfExtents.y;
        if (feetY < terrainY)
        {
            center.y = terrainY + halfExtents.y;
            grounded = true;
            if (velocity.y < 0.0f)
                velocity.y = 0.0f;
        }
        break;
    }

    SetBoxCenter(world, center);
}

void CharacterControllerSystem::ResolveOverlaps(EcsWorld& world)
{
    Vector3 center = BoxCenter(world);

    for (int iteration = 0; iteration < 4; iteration++)
    {
        OverlapResult overlap;
        if (!Physics::OverlapBox(world, center, halfExtents, PhysicsLayers::All, entity, overlap))
            break;

        center += overlap.penetrationNormal * (overlap.penetrationDepth + skinWidth);

        if (overlap.penetrationNormal.y > maxSlopeNormalY)
        {
            grounded = true;
            if (velocity.y < 0.0f)
                velocity.y = 0.0f;
        }
    }

    SetBoxCenter(world, center);
}

Vector3 CharacterControllerSystem::SweepAndSlide(EcsWorld& world, Vector3 center, Vector3 displacement, int maxIterations)
{
    for (int iteration = 0; iteration < maxIterations; iteration++)
    {
        const float distance = std::sqrt(displacement.LengthSquared());
        if (distance < 0.00001f)
            break;

        BoxCastQuery query;
        query.origin = center;
        query.halfExtents = halfExtents;
        query.direction = displacement / distance;
        query.maxDistance = distance;
        query.ignoreEntity = entity;

        RaycastHit hit;
        if (!Physics::BoxCast(world, query, hit))
        {
            center += displacement;
            break;
        }

        const float travel = (std::max)(0.0f, hit.distance - skinWidth);
        center += query.direction * travel;

        const Vector3 remaining = displacement - query.direction * travel;
        const float into = Vector3::Dot(remaining, hit.normal);
        displacement = remaining - hit.normal * into;
    }

    return center;
}

bool CharacterControllerSystem::TryStepUp(EcsWorld& world, Vector3& center, const Vector3& horizontalMove)
{
    const Vector3 raised = SweepAndSlide(world, center, {0.0f, stepHeight, 0.0f}, 1);
    if (raised.y - center.y < 0.01f)
        return false;

    const Vector3 advanced = SweepAndSlide(world, raised, horizontalMove, 2);
    if ((advanced - raised).LengthSquared() < horizontalMove.LengthSquared() * 0.25f)
        return false;

    const Vector3 dropped = SweepAndSlide(world, advanced, {0.0f, -stepHeight, 0.0f}, 1);
    center = dropped;
    return true;
}

void CharacterControllerSystem::SnapToGround(EcsWorld& world, Vector3& center)
{
    BoxCastQuery query;
    query.origin = center;
    query.halfExtents = halfExtents;
    query.direction = {0.0f, -1.0f, 0.0f};
    query.maxDistance = groundSnapDistance;
    query.ignoreEntity = entity;

    RaycastHit hit;
    if (!Physics::BoxCast(world, query, hit))
        return;

    if (hit.normal.y < maxSlopeNormalY)
        return;

    center.y -= (std::max)(0.0f, hit.distance - skinWidth);
    grounded = true;
    velocity.y = 0.0f;
}
