#pragma once

#include "Ecs/Components/AABB.h"
#include "Ecs/Components/ColliderComponent.h"
#include "Ecs/Components/RigidbodyComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Ecs/Core/IEcsSystem.h"
#include <vector>

class SimplePhysicSystem : public IEcsSystem
{
public:
    SimplePhysicSystem() {}

    void Run(EcsWorld& world, float deltaTime) override
    {
        auto& rigidbodyPool = world.GetPool<RigidbodyComponent>();
        auto& transformPool = world.GetPool<TransformComponent>();
        auto& colliderPool = world.GetPool<ColliderComponent>();

        for (auto& [entity, rigidbody] : rigidbodyPool)
        {
            rigidbody.grounded = false;

            if (rigidbody.useGravity)
                rigidbody.AddForce({0, gravity * rigidbody.mass, 0});

            if (transformPool.Has(entity))
                rigidbody.Integrate(transformPool.Get(entity).position, deltaTime);
        }

        collidables.clear();
        for (auto& [entity, collider] : colliderPool)
        {
            if (!transformPool.Has(entity))
                continue;

            UpdateWorldBounds(transformPool.Get(entity), collider);
            collidables.push_back(entity);
        }

        for (size_t i = 0; i < collidables.size(); ++i)
        {
            for (size_t j = i + 1; j < collidables.size(); ++j)
                ResolvePair(rigidbodyPool, transformPool, colliderPool, collidables[i], collidables[j]);
        }
    }

private:
    void UpdateWorldBounds(const TransformComponent& transform, ColliderComponent& collider) const
    {
        const Vector3 scaledCenter = {collider.center.x * transform.scale.x, collider.center.y * transform.scale.y, collider.center.z * transform.scale.z};
        const Vector3 half = {collider.size.x * transform.scale.x * 0.5f, collider.size.y * transform.scale.y * 0.5f, collider.size.z * transform.scale.z * 0.5f};

        const Vector3 origin = transform.position + RotatePoint(scaledCenter, transform.rotation);
        const Vector3 rotatedHalf = RotateHalfExtents(half, transform.rotation);

        collider.worldMin = origin - rotatedHalf;
        collider.worldMax = origin + rotatedHalf;
    }

    void ResolvePair(ComponentPool<RigidbodyComponent>& rigidbodyPool, ComponentPool<TransformComponent>& transformPool, ComponentPool<ColliderComponent>& colliderPool,
                     Entity a, Entity b) const
    {
        auto& colliderA = colliderPool.Get(a);
        auto& colliderB = colliderPool.Get(b);

        if (colliderA.isStatic && colliderB.isStatic)
            return;
        if (colliderA.isTrigger || colliderB.isTrigger)
            return;
        if ((colliderA.layer & colliderB.collidesWith) == 0 || (colliderB.layer & colliderA.collidesWith) == 0)
            return;

        AABB boxA;
        boxA.min = colliderA.worldMin;
        boxA.max = colliderA.worldMax;

        AABB boxB;
        boxB.min = colliderB.worldMin;
        boxB.max = colliderB.worldMax;

        const CollisionInfo info = CheckCollision(boxA, boxB);
        if (!info.collided)
            return;

        const bool hasRbA = rigidbodyPool.Has(a);
        const bool hasRbB = rigidbodyPool.Has(b);

        const float invMassA = (hasRbA && !colliderA.isStatic) ? 1.0f / rigidbodyPool.Get(a).mass : 0.0f;
        const float invMassB = (hasRbB && !colliderB.isStatic) ? 1.0f / rigidbodyPool.Get(b).mass : 0.0f;
        const float invMassSum = invMassA + invMassB;

        if (invMassSum < 1e-8f)
            return;

        transformPool.Get(a).position -= info.normal * (info.depth * invMassA / invMassSum);
        transformPool.Get(b).position += info.normal * (info.depth * invMassB / invMassSum);

        if (info.normal.y < -groundNormalThreshold && hasRbA)
            MarkGrounded(rigidbodyPool.Get(a));
        if (info.normal.y > groundNormalThreshold && hasRbB)
            MarkGrounded(rigidbodyPool.Get(b));

        Vector3 relVel = {0, 0, 0};
        if (hasRbA)
            relVel = relVel + rigidbodyPool.Get(a).velocity;
        if (hasRbB)
            relVel = relVel - rigidbodyPool.Get(b).velocity;

        const float velAlongNormal = Vector3::Dot(relVel, info.normal);
        if (velAlongNormal < 0)
            return;

        const float bounce = (std::min)(colliderA.restitution, colliderB.restitution);
        const float impulseScalar = -(1.0f + bounce) * velAlongNormal / invMassSum;
        const Vector3 impulse = info.normal * impulseScalar;

        if (hasRbA)
            rigidbodyPool.Get(a).velocity += impulse * invMassA;
        if (hasRbB)
            rigidbodyPool.Get(b).velocity -= impulse * invMassB;
    }

    static void MarkGrounded(RigidbodyComponent& rigidbody)
    {
        rigidbody.grounded = true;
        if (rigidbody.velocity.y < 0.0f)
            rigidbody.velocity.y = 0.0f;
    }

    static Vector3 RotatePoint(const Vector3& point, const Matrix4& rot)
    {
        return {point.x * rot.matrix[0][0] + point.y * rot.matrix[1][0] + point.z * rot.matrix[2][0],
                point.x * rot.matrix[0][1] + point.y * rot.matrix[1][1] + point.z * rot.matrix[2][1],
                point.x * rot.matrix[0][2] + point.y * rot.matrix[1][2] + point.z * rot.matrix[2][2]};
    }

    static Vector3 RotateHalfExtents(const Vector3& half, const Matrix4& rot)
    {
        return {std::abs(rot.matrix[0][0]) * half.x + std::abs(rot.matrix[1][0]) * half.y + std::abs(rot.matrix[2][0]) * half.z,
                std::abs(rot.matrix[0][1]) * half.x + std::abs(rot.matrix[1][1]) * half.y + std::abs(rot.matrix[2][1]) * half.z,
                std::abs(rot.matrix[0][2]) * half.x + std::abs(rot.matrix[1][2]) * half.y + std::abs(rot.matrix[2][2]) * half.z};
    }

    std::vector<Entity> collidables;

    float gravity = -9.8f;
    static constexpr float groundNormalThreshold = 0.7f;
};
