#pragma once

#include <string>
#include <vector>
#include "Ecs/Core/Entity.h"
#include "Math/Vector3.h"
#include "Systems/GameComponents.h"

class Engine;
class EcsWorld;

struct EnemyFrameInput
{
    Vector3 playerPosition = {0, 0, 0};
    Vector3 playerEyePosition = {0, 0, 0};
    bool playerAlive = true;
    bool flashlightOn = false;
    Vector3 firePosition = {0, 0, 0};
    float fireSafeRadius = 0.0f;
};

class EnemySystem
{
public:
    Entity Spawn(Engine& engine, const Vector3& position, float yawDegrees);
    float Update(Engine& engine, const EnemyFrameInput& input, float deltaTime);
    bool Damage(Engine& engine, Entity enemy, float amount);
    void HearNoise(EcsWorld& world, const Vector3& position, float radius);

    const std::vector<Entity>& Enemies() const
    {
        return enemies;
    }

    int AliveCount(EcsWorld& world) const;
    void Clear();

    static constexpr float HeadHeight = 1.32f;

private:
    void SetClip(EnemyComponent& enemy, const std::string& clip, bool loop);
    static float ClipLength(const EnemyComponent& enemy, const std::string& clip);
    bool CanSee(EcsWorld& world, Entity self, const Vector3& eye, const Vector3& target, float range) const;
    void MoveTowards(EcsWorld& world, Entity self, EnemyComponent& enemy, const Vector3& target, float speed, float deltaTime);
    void FaceTowards(EnemyComponent& enemy, const Vector3& from, const Vector3& target, float deltaTime);
    void SyncParts(EcsWorld& world, Entity self, const EnemyComponent& enemy);

    std::vector<Entity> enemies;
    unsigned int randomState = 12345u;
};
