#pragma once

#include "Math/Vector3.h"

enum class LightType
{
    Directional,
    Point,
    Spot
};

struct LightComponent
{
    LightType type = LightType::Directional;

    Vector3 color = {1.0f, 1.0f, 1.0f};
    Vector3 direction = {0.5f, 1.0f, -0.3f};
    Vector3 position = {0.0f, 0.0f, 0.0f};

    float intensity = 0.8f;
    float range = 10.0f;

    float innerConeAngleDeg = 20.0f;
    float outerConeAngleDeg = 30.0f;

    bool castShadows = false;
    float shadowOrthoSize = 25.0f;
    float shadowDistance = 40.0f;
    float shadowBias = 0.0025f;
    float shadowAmbientOcclusion = 0.4f;
    float shadowFocusDistance = 0.0f;
    float shadowNormalBias = 0.05f;
};
