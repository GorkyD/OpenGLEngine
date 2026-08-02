#pragma once

#include "Ecs/Core/Entity.h"
#include "Math/Vector3.h"
#include "Physics/PhysicsLayers.h"

struct ProjectileComponent
{
    Vector3 velocity = {0, 0, 0};
    float gravity = 0.0f;
    float radius = 0.0f;
    float lifetime = 5.0f;

    unsigned int layerMask = PhysicsLayers::All;
    Entity owner = INVALID_ENTITY;

    bool hasHit = false;
    bool expired = false;

    Entity hitEntity = INVALID_ENTITY;
    Vector3 hitPoint = {0, 0, 0};
    Vector3 hitNormal = {0, 0, 0};

    bool IsFinished() const
    {
        return hasHit || expired;
    }
};
