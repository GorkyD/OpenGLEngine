#pragma once

#include "Math/Vector3.h"

struct PendulumComponent
{
    Vector3 pivot = {0, 0, 0};

    float armLength = 1.0f;
    float amplitudeRadians = 0.35f;
    float speed = 1.0f;
    float phase = 0.0f;
    float lightBaseIntensity = 0.0f;
    float flickerAmount = 0.0f;
    float flickerSpeed = 6.0f;
    float emissiveBaseIntensity = 0.0f;
};
