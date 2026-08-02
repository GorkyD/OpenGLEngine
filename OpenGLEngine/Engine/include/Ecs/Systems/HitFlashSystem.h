#pragma once

#include <algorithm>
#include "Ecs/Components/GroupComponent.h"
#include "Ecs/Components/HitFlashComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Core/IEcsSystem.h"

class HitFlashSystem : public IEcsSystem
{
public:
    static void TriggerGroup(EcsWorld& world, Entity entity)
    {
        if (!world.HasComponent<GroupComponent>(entity))
        {
            if (world.HasComponent<HitFlashComponent>(entity))
                world.GetComponent<HitFlashComponent>(entity).Trigger();
            return;
        }

        const std::string groupName = world.GetComponent<GroupComponent>(entity).name;
        for (auto& [candidate, group] : world.GetPool<GroupComponent>())
        {
            if (group.name == groupName && world.HasComponent<HitFlashComponent>(candidate))
                world.GetComponent<HitFlashComponent>(candidate).Trigger();
        }
    }

    void Run(EcsWorld& world, float deltaTime) override
    {
        auto& materials = world.GetPool<MaterialComponent>();

        for (auto& [entity, flash] : world.GetPool<HitFlashComponent>())
        {
            if (flash.timer < 0.0f || !materials.Has(entity))
                continue;

            auto& material = materials.Get(entity);

            if (!flash.active)
            {
                flash.active = true;
                flash.originalEmissiveColor = material.emissiveColor;
                flash.originalEmissiveIntensity = material.emissiveIntensity;
            }

            flash.timer -= deltaTime;

            if (flash.timer <= 0.0f)
            {
                flash.timer = -1.0f;
                flash.active = false;
                material.emissiveColor = flash.originalEmissiveColor;
                material.emissiveIntensity = flash.originalEmissiveIntensity;
                continue;
            }

            const float amount = flash.duration > 0.0001f ? flash.timer / flash.duration : 0.0f;
            material.emissiveColor = flash.flashColor;
            material.emissiveIntensity = flash.flashIntensity * amount;
        }
    }
};
