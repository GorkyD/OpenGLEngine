#include "Systems/CampfireSystem.h"
#include <algorithm>
#include <cmath>
#include "Ecs/Components/FogComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ParticleEmitterComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"

void CampfireSystem::Configure(EcsWorld& world, const CampfireEntities& entities)
{
    entity = entities.flame;
    position = entities.position;
    lightBase = entities.position + Vector3(0.0f, 0.8f, 0.0f);

    auto& campfire = world.AddComponent<CampfireComponent>(entity);
    campfire.light = entities.light;
    campfire.flame = entities.flame;
    campfire.sparks = entities.sparks;

    fogCaptured = false;
    for (auto& [fogEntity, fog] : world.GetPool<FogComponent>())
    {
        baseFogStart = fog.start;
        baseFogEnd = fog.end;
        fogCaptured = true;
        break;
    }
}

const CampfireComponent* CampfireSystem::Get(EcsWorld& world) const
{
    return world.HasComponent<CampfireComponent>(entity) ? &world.GetComponent<CampfireComponent>(entity) : nullptr;
}

bool CampfireSystem::Feed(EcsWorld& world)
{
    if (!world.HasComponent<CampfireComponent>(entity))
        return false;

    auto& campfire = world.GetComponent<CampfireComponent>(entity);
    if (campfire.fuel >= campfire.maxFuel - 1.0f)
        return false;

    campfire.fuel = (std::min)(campfire.maxFuel, campfire.fuel + campfire.woodFuel);
    campfire.outTimer = 0.0f;
    return true;
}

void CampfireSystem::Update(EcsWorld& world, float deltaTime)
{
    if (!world.HasComponent<CampfireComponent>(entity))
        return;

    auto& campfire = world.GetComponent<CampfireComponent>(entity);
    campfire.time += deltaTime;
    campfire.fuel = (std::max)(0.0f, campfire.fuel - campfire.burnRate * deltaTime);

    const bool burning = campfire.fuel > 0.0f;
    campfire.outTimer = burning ? 0.0f : campfire.outTimer + deltaTime;

    const float ratio = campfire.FuelRatio();
    const float t = campfire.time;
    const float flicker = 0.78f + 0.12f * std::sin(t * 7.3f) + 0.07f * std::sin(t * 13.1f + 1.7f) + 0.05f * std::sin(t * 23.7f + 4.1f);

    if (world.HasComponent<LightComponent>(campfire.light))
    {
        auto& light = world.GetComponent<LightComponent>(campfire.light);
        light.intensity = burning ? campfire.baseIntensity * (0.3f + 0.7f * ratio) * flicker : 0.0f;
        light.range = campfire.baseRange * (0.45f + 0.55f * ratio);
        light.position = lightBase + Vector3(0.05f * std::sin(t * 5.1f), 0.04f * std::sin(t * 9.3f), 0.05f * std::cos(t * 6.7f));
    }

    if (world.HasComponent<MeshComponent>(campfire.flame))
        world.GetComponent<MeshComponent>(campfire.flame).visible = burning;

    if (world.HasComponent<TransformComponent>(campfire.flame))
    {
        auto& transform = world.GetComponent<TransformComponent>(campfire.flame);
        const float height = 0.95f * (0.3f + 0.7f * ratio) * (0.92f + 0.08f * std::sin(t * 11.0f));
        transform.scale = {0.45f * (0.6f + 0.4f * ratio), height, 0.45f * (0.6f + 0.4f * ratio)};
    }

    if (world.HasComponent<ParticleEmitterComponent>(campfire.sparks))
        world.GetComponent<ParticleEmitterComponent>(campfire.sparks).spawnRate = burning ? 4.0f + 14.0f * ratio : 0.0f;

    if (world.HasComponent<ParticleEmitterComponent>(campfire.flame))
        world.GetComponent<ParticleEmitterComponent>(campfire.flame).spawnRate = burning ? 12.0f : 4.0f;

    if (fogCaptured)
    {
        for (auto& [fogEntity, fog] : world.GetPool<FogComponent>())
        {
            fog.start = baseFogStart * (0.35f + 0.65f * ratio);
            fog.end = baseFogEnd * (0.45f + 0.55f * ratio);
            break;
        }
    }
}
