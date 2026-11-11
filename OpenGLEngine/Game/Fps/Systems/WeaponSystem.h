#pragma once

#include <vector>
#include "Ecs/Core/Entity.h"
#include "Math/Vector3.h"

class Engine;
class EcsWorld;
class Inventory;

struct ShotResult
{
    bool hit = false;
    Entity entity = INVALID_ENTITY;
    Vector3 point = {0, 0, 0};
    Vector3 origin = {0, 0, 0};
};

class WeaponSystem
{
public:
    void Configure(Entity cameraEntity, Entity muzzleEntity);
    ShotResult Fire(Engine& engine);
    ShotResult Trace(Engine& engine, float range) const;
    void SpawnImpact(Engine& engine, const Vector3& point, const Vector3& towardsShooter, bool flesh);
    void Update(EcsWorld& world, float deltaTime);

    bool CanFire() const
    {
        return magazine > 0;
    }
    bool CanReload(const Inventory& inventory) const;
    void Reload(Inventory& inventory);

    int magazine = 8;
    int magazineCapacity = 8;
    float range = 120.0f;
    float muzzleForwardOffset = 0.8f;

private:
    Vector3 ResolveMuzzlePosition(EcsWorld& world, const Vector3& eyePosition, const Vector3& forward) const;
    void SpawnMuzzleFlash(Engine& engine, const Vector3& position, const Vector3& direction);
    void SpawnBurst(Engine& engine, const Vector3& position, const Vector3& baseVelocity, float spread, int count, float size, float lifetime, bool additive, const float color[4], float gravity);

    Entity cameraEntity = 0;
    Entity muzzleEntity = 0;

    struct TimedEntity
    {
        Entity entity = 0;
        float timeRemaining = 0.0f;
    };
    std::vector<TimedEntity> transient;
};
