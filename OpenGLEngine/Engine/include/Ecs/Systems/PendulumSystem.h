#pragma once

#include <cmath>
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/PendulumComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"

class PendulumSystem : public IEcsSystem
{
public:
    void Run(EcsWorld& world, float deltaTime) override
    {
        elapsedTime += deltaTime;

        auto& pendulums = world.GetPool<PendulumComponent>();
        auto& transforms = world.GetPool<TransformComponent>();
        auto& lights = world.GetPool<LightComponent>();
        auto& materials = world.GetPool<MaterialComponent>();

        for (auto& [entityId, pendulumComponent] : pendulums)
        {
            if (!transforms.Has(entityId))
                continue;

            const float swingAngle = pendulumComponent.amplitudeRadians * std::sin(elapsedTime * pendulumComponent.speed + pendulumComponent.phase);

            auto& transform = transforms.Get(entityId);
            transform.position = pendulumComponent.pivot + Vector3(std::sin(swingAngle) * pendulumComponent.armLength, -std::cos(swingAngle) * pendulumComponent.armLength, 0.0f);
            transform.rotation.SetRotationZ(swingAngle);

            const float flicker = 1.0f + pendulumComponent.flickerAmount * std::sin(elapsedTime * pendulumComponent.flickerSpeed);

            if (lights.Has(entityId) && pendulumComponent.lightBaseIntensity > 0.0f)
            {
                auto& light = lights.Get(entityId);
                light.position = transform.position;
                light.intensity = pendulumComponent.lightBaseIntensity * flicker;
            }

            if (materials.Has(entityId) && pendulumComponent.emissiveBaseIntensity > 0.0f)
            {
                auto& material = materials.Get(entityId);
                material.emissiveIntensity = pendulumComponent.emissiveBaseIntensity * flicker;
            }
        }
    }

private:
    float elapsedTime = 0.0f;
};
