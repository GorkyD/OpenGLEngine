#pragma once

#include <cmath>
#include "Ecs/Components/ProjectileComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Physics/Physics.h"

class ProjectileSystem : public IEcsSystem
{
public:
    void Run(EcsWorld& world, float deltaTime) override
    {
        auto& transforms = world.GetPool<TransformComponent>();

        for (auto& [entity, projectile] : world.GetPool<ProjectileComponent>())
        {
            if (projectile.IsFinished() || !transforms.Has(entity))
                continue;

            projectile.lifetime -= deltaTime;
            if (projectile.lifetime <= 0.0f)
            {
                projectile.expired = true;
                continue;
            }

            if (projectile.gravity != 0.0f)
                projectile.velocity.y += projectile.gravity * deltaTime;

            const Vector3 step = projectile.velocity * deltaTime;
            const float distance = step.Length();
            if (distance < 0.000001f)
                continue;

            auto& transform = transforms.Get(entity);
            const Vector3 direction = step / distance;

            if (SweepStep(world, entity, projectile, transform.position, direction, distance))
                continue;

            transform.position += step;
        }
    }

private:
    static bool SweepStep(EcsWorld& world, Entity entity, ProjectileComponent& projectile, Vector3& position, const Vector3& direction, float distance)
    {
        RaycastHit hit;
        bool blocked = false;

        if (projectile.radius > 0.0f)
        {
            BoxCastQuery query;
            query.origin = position;
            query.halfExtents = {projectile.radius, projectile.radius, projectile.radius};
            query.direction = direction;
            query.maxDistance = distance;
            query.layerMask = projectile.layerMask;
            query.ignoreEntity = projectile.owner;

            blocked = Physics::BoxCast(world, query, hit);
        }
        else
        {
            RaycastQuery query;
            query.origin = position;
            query.direction = direction;
            query.maxDistance = distance;
            query.layerMask = projectile.layerMask;
            query.ignoreEntity = projectile.owner;

            blocked = Physics::Raycast(world, query, hit);
        }

        if (!blocked || hit.entity == entity)
            return false;

        position = position + direction * hit.distance;

        projectile.hasHit = true;
        projectile.hitEntity = hit.entity;
        projectile.hitPoint = hit.point;
        projectile.hitNormal = hit.normal;

        return true;
    }
};
