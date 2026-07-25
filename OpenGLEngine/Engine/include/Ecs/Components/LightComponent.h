#pragma once
#include "Math/Vector3.h"

enum class LightType
{
    Directional,
    Point
};

struct LightComponent
{
    LightType type = LightType::Directional;
    Vector3 color = {1.0f, 1.0f, 1.0f};
    float intensity = 0.8f;

    // Directional light only
    Vector3 direction = {0.5f, 1.0f, -0.3f};

    // Point light only
    Vector3 position = {0.0f, 0.0f, 0.0f};
    float range = 10.0f;
};
