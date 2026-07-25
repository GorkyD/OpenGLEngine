#pragma once
#include "Math/Vector3.h"

struct FogComponent
{
    Vector3 color = {0.5f, 0.55f, 0.65f};
    float start = 15.0f;
    float end = 60.0f;

    float horizonSpread = 0.12f;
    float horizonIntensity = 0.8f;
};
