#pragma once

#include "Ecs/Components/RotatorComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"

class RotatorSystem : public IEcsSystem
{
public:
    void Run(EcsWorld& world, float deltaTime) override
    {
        auto& rotators = world.GetPool<RotatorComponent>();
        auto& transforms = world.GetPool<TransformComponent>();

        for (auto& [entityId, rotatorComponent] : rotators)
        {
            if (!transforms.Has(entityId))
                continue;

            rotatorComponent.currentAngle += rotatorComponent.degreesPerSecond * deltaTime;
            if (rotatorComponent.currentAngle > 360.0f)
                rotatorComponent.currentAngle -= 360.0f;

            constexpr float degToRad = 3.14159265f / 180.0f;

            auto& transform = transforms.Get(entityId);
            transform.rotation.SetRotationY(rotatorComponent.currentAngle * degToRad);
        }
    }
};
