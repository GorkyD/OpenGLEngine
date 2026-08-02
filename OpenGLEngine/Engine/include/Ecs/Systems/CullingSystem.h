#pragma once

#include "Ecs/Components/AABB.h"
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/VisibilityComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Math/Frustum.h"

class CullingSystem : public IEcsSystem
{
public:
    bool enabled = true;

    int GetVisibleCount() const
    {
        return visibleCount;
    }
    int GetCulledCount() const
    {
        return culledCount;
    }
    const Frustum& GetFrustum() const
    {
        return frustum;
    }

    void Run(EcsWorld& world, float deltaTime) override
    {
        visibleCount = 0;
        culledCount = 0;

        if (!UpdateFrustum(world))
            return;

        auto& bounds = world.GetPool<AABB>();

        for (auto& [entity, aabb] : bounds)
        {
            auto& visibility = world.HasComponent<VisibilityComponent>(entity) ? world.GetComponent<VisibilityComponent>(entity)
                                                                               : world.AddComponent<VisibilityComponent>(entity);

            if (!enabled || !visibility.cullingEnabled || IsDegenerate(aabb))
            {
                visibility.visibleInFrustum = true;
                visibleCount++;
                continue;
            }

            visibility.visibleInFrustum = frustum.IntersectsBox(aabb.min, aabb.max);

            if (visibility.visibleInFrustum)
                visibleCount++;
            else
                culledCount++;
        }
    }

private:
    static bool IsDegenerate(const AABB& aabb)
    {
        return aabb.min.x == aabb.max.x && aabb.min.y == aabb.max.y && aabb.min.z == aabb.max.z;
    }

    bool UpdateFrustum(EcsWorld& world)
    {
        for (auto& [entity, camera] : world.GetPool<CameraComponent>())
        {
            if (!camera.isActive || !world.HasComponent<TransformComponent>(entity))
                continue;

            frustum.ExtractFromViewProjection(camera.view * camera.projection);
            return true;
        }

        return false;
    }

    Frustum frustum;
    int visibleCount = 0;
    int culledCount = 0;
};
