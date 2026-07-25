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

        for (auto& pair : pendulums)
        {
            Entity entity = pair.first;
            auto& pendulum = pair.second;

            if (!transforms.Has(entity))
                continue;

            const float swingAngle = pendulum.amplitudeRadians * std::sin(elapsedTime * pendulum.speed + pendulum.phase);

            auto& transform = transforms.Get(entity);
            transform.position = pendulum.pivot + Vector3(std::sin(swingAngle) * pendulum.armLength,
                                                            -std::cos(swingAngle) * pendulum.armLength, 0.0f);
            transform.rotation.SetRotationZ(swingAngle);

            const float flicker = 1.0f + pendulum.flickerAmount * std::sin(elapsedTime * pendulum.flickerSpeed);

            if (lights.Has(entity) && pendulum.lightBaseIntensity > 0.0f)
            {
                auto& light = lights.Get(entity);
                light.position = transform.position;
                light.intensity = pendulum.lightBaseIntensity * flicker;
            }

            if (materials.Has(entity) && pendulum.emissiveBaseIntensity > 0.0f)
            {
                auto& material = materials.Get(entity);
                material.emissiveIntensity = pendulum.emissiveBaseIntensity * flicker;
            }
        }
    }

private:
    float elapsedTime = 0.0f;
};
