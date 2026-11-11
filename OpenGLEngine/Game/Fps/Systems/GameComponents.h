#pragma once

#include <memory>
#include <string>
#include <vector>
#include "Ecs/Core/Entity.h"
#include "Math/Vector3.h"

struct AnimatorState;

enum class ItemId
{
    None,
    PistolAmmo,
    Bandage,
    Wood,
    Battery
};

inline const char* ItemName(ItemId item)
{
    switch (item)
    {
    case ItemId::PistolAmmo:
        return "Pistol ammo";
    case ItemId::Bandage:
        return "Bandage";
    case ItemId::Wood:
        return "Firewood";
    case ItemId::Battery:
        return "Battery";
    default:
        return "-";
    }
}

inline int ItemMaxStack(ItemId item)
{
    switch (item)
    {
    case ItemId::PistolAmmo:
        return 24;
    case ItemId::Bandage:
        return 3;
    case ItemId::Wood:
        return 3;
    case ItemId::Battery:
        return 2;
    default:
        return 1;
    }
}

struct HealthComponent
{
    float hp = 100.0f;
    float maxHp = 100.0f;
    bool dead = false;
};

enum class EnemyState
{
    Idle,
    Wander,
    Chase,
    Lurk,
    Attack,
    Stagger,
    Dead
};

struct EnemyComponent
{
    EnemyState state = EnemyState::Idle;
    std::vector<Entity> parts;
    std::shared_ptr<AnimatorState> animator;
    std::string currentClip;

    Vector3 home = {0, 0, 0};
    Vector3 wanderTarget = {0, 0, 0};
    Vector3 lastKnownTarget = {0, 0, 0};

    float yaw = 0.0f;
    float stateTimer = 0.0f;
    float attackCooldown = 0.0f;
    float alertTimer = 0.0f;
    bool attackLanded = false;

    float walkSpeed = 1.15f;
    float sightRange = 15.0f;
    float attackRange = 1.5f;
    float attackDamage = 18.0f;
};

struct PickupComponent
{
    ItemId item = ItemId::None;
    int count = 1;
    Vector3 center = {0, 0, 0};
    std::vector<Entity> parts;
};

struct CampfireComponent
{
    float fuel = 60.0f;
    float maxFuel = 100.0f;
    float burnRate = 0.5f;
    float woodFuel = 25.0f;

    Entity light = 0;
    Entity flame = 0;
    Entity sparks = 0;

    float baseIntensity = 9.0f;
    float baseRange = 16.0f;
    float minSafeRadius = 3.0f;
    float maxSafeRadius = 9.0f;
    float outTimer = 0.0f;
    float time = 0.0f;

    float FuelRatio() const
    {
        return maxFuel > 0.0f ? fuel / maxFuel : 0.0f;
    }

    float SafeRadius() const
    {
        return fuel <= 0.0f ? 0.0f : minSafeRadius + (maxSafeRadius - minSafeRadius) * FuelRatio();
    }
};
