#pragma once

#include "Math/Vector3.h"
#include "Physics/PhysicsLayers.h"

struct ColliderComponent
{
    Vector3 center = {0, 0, 0};
    Vector3 size = {1, 1, 1};

    bool isStatic = false;
    bool isTrigger = false;
    float restitution = 0.3f;

    unsigned int layer = PhysicsLayers::Default;
    unsigned int collidesWith = PhysicsLayers::All;

    Vector3 worldMin = {0, 0, 0};
    Vector3 worldMax = {0, 0, 0};
};
