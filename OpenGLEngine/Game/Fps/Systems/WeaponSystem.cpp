#include "Systems/WeaponSystem.h"
#include <algorithm>
#include <cstdlib>
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/ParticleEmitterComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/WorldTransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Engine/Engine.h"
#include "Physics/Physics.h"
#include "Systems/Inventory.h"

namespace
{
float RandomRange(const float minValue, const float maxValue)
{
    const float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return minValue + t * (maxValue - minValue);
}
}

void WeaponSystem::Configure(Entity playerCameraEntity, Entity weaponMuzzleEntity)
{
    cameraEntity = playerCameraEntity;
    muzzleEntity = weaponMuzzleEntity;
    transient.clear();
}

Vector3 WeaponSystem::ResolveMuzzlePosition(EcsWorld& world, const Vector3& eyePosition, const Vector3& forward) const
{
    if (muzzleEntity == 0 || !world.HasComponent<WorldTransformComponent>(muzzleEntity))
        return eyePosition + forward * muzzleForwardOffset;

    const auto& matrix = world.GetComponent<WorldTransformComponent>(muzzleEntity).matrix;
    return {matrix.matrix[3][0], matrix.matrix[3][1], matrix.matrix[3][2]};
}

ShotResult WeaponSystem::Trace(Engine& engine, float traceRange) const
{
    auto& world = engine.GetWorld();
    const auto& cam = world.GetComponent<CameraComponent>(cameraEntity);
    const auto& camTransform = world.GetComponent<TransformComponent>(cameraEntity);

    RaycastQuery query;
    query.origin = camTransform.position;
    query.direction = cam.forward;
    query.maxDistance = traceRange;
    query.layerMask = PhysicsLayers::All & ~PhysicsLayers::Player;
    query.ignoreEntity = cameraEntity;

    ShotResult result;
    result.origin = camTransform.position;

    RaycastHit hit;
    if (Physics::Raycast(world, query, hit))
    {
        result.hit = true;
        result.entity = hit.entity;
        result.point = hit.point;
    }
    else
    {
        result.point = camTransform.position + cam.forward * traceRange;
    }

    return result;
}

ShotResult WeaponSystem::Fire(Engine& engine)
{
    if (magazine <= 0)
        return {};

    magazine--;

    auto& world = engine.GetWorld();
    const auto& cam = world.GetComponent<CameraComponent>(cameraEntity);
    const auto& camTransform = world.GetComponent<TransformComponent>(cameraEntity);

    const ShotResult shot = Trace(engine, range);
    const Vector3 muzzle = ResolveMuzzlePosition(world, camTransform.position, cam.forward);
    SpawnMuzzleFlash(engine, muzzle, cam.forward);
    return shot;
}

bool WeaponSystem::CanReload(const Inventory& inventory) const
{
    return magazine < magazineCapacity && inventory.Count(ItemId::PistolAmmo) > 0;
}

void WeaponSystem::Reload(Inventory& inventory)
{
    const int needed = magazineCapacity - magazine;
    magazine += inventory.Remove(ItemId::PistolAmmo, needed);
}

void WeaponSystem::SpawnBurst(Engine& engine, const Vector3& position, const Vector3& baseVelocity, float spread, int count, float size, float lifetime, bool additive,
                              const float color[4], float gravity)
{
    auto& world = engine.GetWorld();

    const Entity entity = world.CreateEntity();
    world.AddComponent<TransformComponent>(entity).position = position;

    auto& emitter = world.AddComponent<ParticleEmitterComponent>(entity);
    emitter.color = {color[0], color[1], color[2], color[3]};
    emitter.additive = additive;
    emitter.spawnRate = 0.0f;
    emitter.startSize = size;
    emitter.endSize = size * 0.3f;
    emitter.lifetimeMin = lifetime * 0.6f;
    emitter.lifetimeMax = lifetime;
    emitter.gravity = {0.0f, gravity, 0.0f};
    emitter.maxParticles = static_cast<unsigned int>(count);
    emitter.particles.reserve(count);

    for (int i = 0; i < count; i++)
    {
        Particle particle;
        particle.position = position;
        particle.velocity = baseVelocity + Vector3(RandomRange(-1.0f, 1.0f), RandomRange(-1.0f, 1.0f), RandomRange(-1.0f, 1.0f)) * spread;
        particle.lifetime = RandomRange(emitter.lifetimeMin, emitter.lifetimeMax);
        particle.age = 0.0f;
        emitter.particles.push_back(particle);
    }

    transient.push_back({entity, lifetime});
}

void WeaponSystem::SpawnMuzzleFlash(Engine& engine, const Vector3& position, const Vector3& direction)
{
    const float color[4] = {1.0f, 0.85f, 0.5f, 1.0f};
    SpawnBurst(engine, position, direction * 2.5f, 1.2f, 10, 0.3f, 0.06f, true, color, 0.0f);

    auto& world = engine.GetWorld();
    const Entity lightEntity = world.CreateEntity();
    auto& light = world.AddComponent<LightComponent>(lightEntity);
    light.type = LightType::Point;
    light.color = {1.0f, 0.75f, 0.4f};
    light.intensity = 6.0f;
    light.range = 9.0f;
    light.position = position + direction * 0.3f;
    light.serialized = false;
    transient.push_back({lightEntity, 0.05f});
}

void WeaponSystem::SpawnImpact(Engine& engine, const Vector3& point, const Vector3& towardsShooter, bool flesh)
{
    if (flesh)
    {
        const float blood[4] = {0.35f, 0.02f, 0.01f, 0.9f};
        SpawnBurst(engine, point, towardsShooter * 1.2f, 1.1f, 14, 0.09f, 0.5f, false, blood, -6.0f);
    }
    else
    {
        const float dust[4] = {0.45f, 0.42f, 0.38f, 0.5f};
        SpawnBurst(engine, point, towardsShooter * 0.8f, 0.6f, 8, 0.12f, 0.4f, false, dust, -1.0f);
    }
}

void WeaponSystem::Update(EcsWorld& world, float deltaTime)
{
    for (size_t i = 0; i < transient.size();)
    {
        transient[i].timeRemaining -= deltaTime;
        if (transient[i].timeRemaining <= 0.0f)
        {
            world.DestroyEntity(transient[i].entity);
            transient[i] = transient.back();
            transient.pop_back();
        }
        else
        {
            i++;
        }
    }
}
