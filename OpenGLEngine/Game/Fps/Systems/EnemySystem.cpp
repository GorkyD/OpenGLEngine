#include "Systems/EnemySystem.h"
#include <algorithm>
#include <cmath>
#include "Ecs/Components/AnimatorComponent.h"
#include "Ecs/Components/ColliderComponent.h"
#include "Ecs/Components/HitFlashComponent.h"
#include "Ecs/Components/NameComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Engine/Engine.h"
#include "Math/Matrix4.h"
#include "Physics/Physics.h"
#include "Physics/PhysicsLayers.h"
#include "Resource/EntityFactory.h"
#include "Systems/ForestBuilder.h"

namespace
{
constexpr const char* ZombieModel = "Assets/Models/Enemies/ZombieWoman/glb/zombie_woman_ual.glb";
constexpr float AttackHitTime = 0.65f;
constexpr float StaggerTime = 0.35f;
constexpr float ModelYawOffset = 3.14159265f;

float HorizontalDistance(const Vector3& a, const Vector3& b)
{
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}
}

Entity EnemySystem::Spawn(Engine& engine, const Vector3& position, float yawDegrees)
{
    auto& world = engine.GetWorld();

    const Entity first = EntityFactory::CreateSkinnedModelEntity(world, engine.GetRenderEngine(), ZombieModel, engine.GetShaders().skinned);
    if (first == 0 || !world.HasComponent<AnimatorComponent>(first))
        return 0;

    const auto state = world.GetComponent<AnimatorComponent>(first).state;

    const Entity root = world.CreateEntity();
    world.AddComponent<NameComponent>(root).name = "Zombie";
    auto& transform = world.AddComponent<TransformComponent>(root);
    transform.position = {position.x, ForestBuilder::GroundHeight(world, position.x, position.z), position.z};

    auto& collider = world.AddComponent<ColliderComponent>(root);
    collider.center = {0.0f, 0.85f, 0.0f};
    collider.size = {0.55f, 1.7f, 0.55f};
    collider.layer = PhysicsLayers::Enemy;
    collider.restitution = 0.0f;

    world.AddComponent<HealthComponent>(root);

    auto& enemy = world.AddComponent<EnemyComponent>(root);
    enemy.animator = state;
    enemy.home = transform.position;
    enemy.wanderTarget = transform.position;
    enemy.yaw = yawDegrees * 3.14159265f / 180.0f;
    enemy.stateTimer = 1.0f + static_cast<float>(enemies.size());

    for (auto& [entity, animator] : world.GetPool<AnimatorComponent>())
    {
        if (animator.state == state)
        {
            enemy.parts.push_back(entity);
            auto& flash = world.AddComponent<HitFlashComponent>(entity);
            flash.flashColor = {0.6f, 0.04f, 0.02f};
            flash.flashIntensity = 0.9f;
        }
    }

    SetClip(enemy, "idle", true);
    SyncParts(world, root, enemy);
    enemies.push_back(root);
    return root;
}

void EnemySystem::Clear()
{
    enemies.clear();
}

int EnemySystem::AliveCount(EcsWorld& world) const
{
    int alive = 0;
    for (const auto entity : enemies)
    {
        if (world.HasComponent<EnemyComponent>(entity) && world.GetComponent<EnemyComponent>(entity).state != EnemyState::Dead)
            alive++;
    }
    return alive;
}

void EnemySystem::SetClip(EnemyComponent& enemy, const std::string& clip, bool loop)
{
    if (!enemy.animator || enemy.currentClip == clip)
        return;
    enemy.currentClip = clip;
    Animator::Play(*enemy.animator, clip, loop);
}

float EnemySystem::ClipLength(const EnemyComponent& enemy, const std::string& clip)
{
    if (!enemy.animator || !enemy.animator->clips)
        return 1.0f;
    for (const auto& candidate : *enemy.animator->clips)
    {
        if (candidate.name == clip)
            return candidate.ticksPerSecond > 0.0f ? candidate.durationTicks / candidate.ticksPerSecond : candidate.durationTicks / 25.0f;
    }
    return 1.0f;
}

bool EnemySystem::CanSee(EcsWorld& world, Entity self, const Vector3& eye, const Vector3& target, float range) const
{
    const Vector3 toTarget = target - eye;
    const float distance = toTarget.Length();
    if (distance > range)
        return false;
    if (distance < 0.001f)
        return true;

    RaycastQuery query;
    query.origin = eye;
    query.direction = toTarget * (1.0f / distance);
    query.maxDistance = distance;
    query.layerMask = PhysicsLayers::Default;
    query.ignoreEntity = self;

    RaycastHit hit;
    return !Physics::Raycast(world, query, hit);
}

void EnemySystem::FaceTowards(EnemyComponent& enemy, const Vector3& from, const Vector3& target, float deltaTime)
{
    const float dx = target.x - from.x;
    const float dz = target.z - from.z;
    if (dx * dx + dz * dz < 0.0001f)
        return;

    const float desired = std::atan2(dx, dz);
    float delta = desired - enemy.yaw;
    while (delta > 3.14159265f)
        delta -= 6.2831853f;
    while (delta < -3.14159265f)
        delta += 6.2831853f;

    const float maxTurn = 4.0f * deltaTime;
    enemy.yaw += (std::max)(-maxTurn, (std::min)(maxTurn, delta));
}

void EnemySystem::MoveTowards(EcsWorld& world, Entity self, EnemyComponent& enemy, const Vector3& target, float speed, float deltaTime)
{
    auto& transform = world.GetComponent<TransformComponent>(self);
    FaceTowards(enemy, transform.position, target, deltaTime);

    const float distance = HorizontalDistance(transform.position, target);
    if (distance < 0.05f)
        return;

    const float step = (std::min)(distance, speed * deltaTime);
    const Vector3 direction = {(target.x - transform.position.x) / distance, 0.0f, (target.z - transform.position.z) / distance};

    Vector3 next = transform.position + direction * step;

    for (const auto other : enemies)
    {
        if (other == self || !world.HasComponent<TransformComponent>(other))
            continue;
        if (world.GetComponent<EnemyComponent>(other).state == EnemyState::Dead)
            continue;
        const Vector3 otherPosition = world.GetComponent<TransformComponent>(other).position;
        const float separation = HorizontalDistance(next, otherPosition);
        if (separation < 0.8f && separation > 0.001f)
        {
            const Vector3 push = {(next.x - otherPosition.x) / separation, 0.0f, (next.z - otherPosition.z) / separation};
            next = next + push * ((0.8f - separation) * 0.5f);
        }
    }

    next.y = ForestBuilder::GroundHeight(world, next.x, next.z);
    transform.position = next;
}

void EnemySystem::SyncParts(EcsWorld& world, Entity self, const EnemyComponent& enemy)
{
    const auto& transform = world.GetComponent<TransformComponent>(self);
    const Matrix4 rotation = Matrix4::FromAxisAngle({0.0f, 1.0f, 0.0f}, enemy.yaw + ModelYawOffset);

    for (const auto part : enemy.parts)
    {
        if (!world.HasComponent<TransformComponent>(part))
            continue;
        auto& partTransform = world.GetComponent<TransformComponent>(part);
        partTransform.position = transform.position;
        partTransform.rotation = rotation;
    }
}

void EnemySystem::HearNoise(EcsWorld& world, const Vector3& position, float radius)
{
    for (const auto entity : enemies)
    {
        if (!world.HasComponent<EnemyComponent>(entity))
            continue;
        auto& enemy = world.GetComponent<EnemyComponent>(entity);
        if (enemy.state == EnemyState::Dead)
            continue;
        if (HorizontalDistance(world.GetComponent<TransformComponent>(entity).position, position) > radius)
            continue;
        enemy.lastKnownTarget = position;
        enemy.alertTimer = 8.0f;
    }
}

bool EnemySystem::Damage(Engine& engine, Entity entity, float amount)
{
    auto& world = engine.GetWorld();
    if (!world.HasComponent<EnemyComponent>(entity) || !world.HasComponent<HealthComponent>(entity))
        return false;

    auto& enemy = world.GetComponent<EnemyComponent>(entity);
    auto& health = world.GetComponent<HealthComponent>(entity);
    if (health.dead)
        return false;

    health.hp -= amount;

    for (const auto part : enemy.parts)
    {
        if (world.HasComponent<HitFlashComponent>(part))
            world.GetComponent<HitFlashComponent>(part).Trigger();
    }

    if (health.hp <= 0.0f)
    {
        health.hp = 0.0f;
        health.dead = true;
        enemy.state = EnemyState::Dead;
        enemy.stateTimer = 0.0f;
        SetClip(enemy, "death_fall", false);
        world.GetPool<ColliderComponent>().Remove(entity);
        return true;
    }

    enemy.alertTimer = 10.0f;
    if (enemy.state != EnemyState::Attack)
    {
        enemy.state = EnemyState::Stagger;
        enemy.stateTimer = 0.0f;
        enemy.currentClip.clear();
        SetClip(enemy, "hit_chest", false);
    }
    return false;
}

float EnemySystem::Update(Engine& engine, const EnemyFrameInput& input, float deltaTime)
{
    auto& world = engine.GetWorld();
    float damageToPlayer = 0.0f;

    for (const auto self : enemies)
    {
        if (!world.HasComponent<EnemyComponent>(self))
            continue;

        auto& enemy = world.GetComponent<EnemyComponent>(self);
        auto& transform = world.GetComponent<TransformComponent>(self);
        enemy.stateTimer += deltaTime;
        enemy.attackCooldown = (std::max)(0.0f, enemy.attackCooldown - deltaTime);
        enemy.alertTimer = (std::max)(0.0f, enemy.alertTimer - deltaTime);

        if (enemy.state == EnemyState::Dead)
        {
            SyncParts(world, self, enemy);
            continue;
        }

        const Vector3 eye = transform.position + Vector3(0.0f, 1.5f, 0.0f);
        const float sightRange = enemy.sightRange + (input.flashlightOn ? 10.0f : 0.0f);
        const float playerDistance = HorizontalDistance(transform.position, input.playerPosition);
        const bool seesPlayer = input.playerAlive && CanSee(world, self, eye, input.playerEyePosition, sightRange);

        if (seesPlayer)
        {
            enemy.lastKnownTarget = input.playerPosition;
            enemy.alertTimer = 6.0f;
        }

        const bool playerInFireLight = input.fireSafeRadius > 0.0f && HorizontalDistance(input.playerPosition, input.firePosition) < input.fireSafeRadius + 0.5f;
        const bool hunting = input.playerAlive && (seesPlayer || enemy.alertTimer > 0.0f);

        switch (enemy.state)
        {
        case EnemyState::Stagger:
            if (enemy.stateTimer >= StaggerTime)
            {
                enemy.state = EnemyState::Chase;
                enemy.stateTimer = 0.0f;
            }
            break;

        case EnemyState::Attack:
        {
            FaceTowards(enemy, transform.position, input.playerPosition, deltaTime);
            if (!enemy.attackLanded && enemy.stateTimer >= AttackHitTime)
            {
                enemy.attackLanded = true;
                if (input.playerAlive && playerDistance <= enemy.attackRange + 0.5f)
                    damageToPlayer += enemy.attackDamage;
            }
            if (enemy.stateTimer >= ClipLength(enemy, "attack_swing"))
            {
                enemy.state = EnemyState::Chase;
                enemy.stateTimer = 0.0f;
                enemy.attackCooldown = 0.5f;
            }
            break;
        }

        default:
        {
            if (hunting)
            {
                Vector3 target = seesPlayer ? input.playerPosition : enemy.lastKnownTarget;

                if (playerInFireLight)
                {
                    Vector3 away = transform.position - input.firePosition;
                    away.y = 0.0f;
                    const float length = away.Length();
                    away = length > 0.001f ? away * (1.0f / length) : Vector3(1.0f, 0.0f, 0.0f);
                    target = input.firePosition + away * (input.fireSafeRadius + 1.2f);
                }

                if (!playerInFireLight && seesPlayer && playerDistance <= enemy.attackRange && enemy.attackCooldown <= 0.0f)
                {
                    enemy.state = EnemyState::Attack;
                    enemy.stateTimer = 0.0f;
                    enemy.attackLanded = false;
                    enemy.currentClip.clear();
                    SetClip(enemy, "attack_swing", false);
                    break;
                }

                if (HorizontalDistance(transform.position, target) > 0.4f)
                {
                    enemy.state = EnemyState::Chase;
                    SetClip(enemy, "walk", true);
                    MoveTowards(world, self, enemy, target, enemy.walkSpeed, deltaTime);
                }
                else
                {
                    enemy.state = playerInFireLight ? EnemyState::Lurk : EnemyState::Idle;
                    SetClip(enemy, "idle", true);
                    FaceTowards(enemy, transform.position, input.playerPosition, deltaTime);
                }
                break;
            }

            if (enemy.state == EnemyState::Wander)
            {
                SetClip(enemy, "walk", true);
                MoveTowards(world, self, enemy, enemy.wanderTarget, enemy.walkSpeed * 0.6f, deltaTime);
                if (HorizontalDistance(transform.position, enemy.wanderTarget) < 0.4f || enemy.stateTimer > 12.0f)
                {
                    enemy.state = EnemyState::Idle;
                    enemy.stateTimer = 0.0f;
                }
            }
            else
            {
                enemy.state = EnemyState::Idle;
                SetClip(enemy, "idle", true);
                if (enemy.stateTimer > 5.0f)
                {
                    randomState = randomState * 1664525u + 1013904223u;
                    const float angle = static_cast<float>((randomState >> 8) & 0xFFFF) / 65535.0f * 6.2831853f;
                    enemy.wanderTarget = enemy.home + Vector3(std::cos(angle), 0.0f, std::sin(angle)) * 5.0f;
                    enemy.state = EnemyState::Wander;
                    enemy.stateTimer = 0.0f;
                }
            }
            break;
        }
        }

        if (input.fireSafeRadius > 0.0f)
        {
            Vector3 fromFire = transform.position - input.firePosition;
            fromFire.y = 0.0f;
            const float distance = fromFire.Length();
            if (distance < input.fireSafeRadius && distance > 0.001f)
            {
                const Vector3 pushed = input.firePosition + fromFire * (input.fireSafeRadius / distance);
                transform.position = {pushed.x, ForestBuilder::GroundHeight(world, pushed.x, pushed.z), pushed.z};
            }
        }

        SyncParts(world, self, enemy);
    }

    return damageToPlayer;
}
