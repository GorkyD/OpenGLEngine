#pragma once

#include <algorithm>
#include "Ecs/Components/AABB.h"
#include "Ecs/Components/LocalBoundsComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/WorldTransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Math/Matrix4.h"

class BoundsSystem : public IEcsSystem
{
public:
    void Run(EcsWorld& world, float deltaTime) override
    {
        auto& transforms = world.GetPool<TransformComponent>();
        auto& bounds = world.GetPool<AABB>();

        for (auto& [entity, local] : world.GetPool<LocalBoundsComponent>())
        {
            if (!transforms.Has(entity) || !bounds.Has(entity))
                continue;

            const Matrix4 model =
                world.HasComponent<WorldTransformComponent>(entity) ? world.GetComponent<WorldTransformComponent>(entity).matrix : transforms.Get(entity).GetModelMatrix();

            const Vector3 corners[8] = {{local.min.x, local.min.y, local.min.z}, {local.max.x, local.min.y, local.min.z}, {local.min.x, local.max.y, local.min.z},
                                        {local.max.x, local.max.y, local.min.z}, {local.min.x, local.min.y, local.max.z}, {local.max.x, local.min.y, local.max.z},
                                        {local.min.x, local.max.y, local.max.z}, {local.max.x, local.max.y, local.max.z}};

            Vector3 worldMin = model.TransformPoint(corners[0]);
            Vector3 worldMax = worldMin;

            for (int i = 1; i < 8; i++)
            {
                const Vector3 point = model.TransformPoint(corners[i]);
                worldMin.x = (std::min)(worldMin.x, point.x);
                worldMin.y = (std::min)(worldMin.y, point.y);
                worldMin.z = (std::min)(worldMin.z, point.z);
                worldMax.x = (std::max)(worldMax.x, point.x);
                worldMax.y = (std::max)(worldMax.y, point.y);
                worldMax.z = (std::max)(worldMax.z, point.z);
            }

            auto& aabb = bounds.Get(entity);
            aabb.min = worldMin;
            aabb.max = worldMax;
        }
    }
};
