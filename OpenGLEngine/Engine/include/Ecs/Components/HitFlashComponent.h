#pragma once

#include "Math/Vector3.h"

struct HitFlashComponent
{
    Vector3 flashColor = {1, 1, 1};
    float flashIntensity = 4.0f;
    float duration = 0.15f;

    float timer = -1.0f;
    bool active = false;

    Vector3 originalEmissiveColor = {0, 0, 0};
    float originalEmissiveIntensity = 0.0f;

    void Trigger()
    {
        timer = duration;
    }

    bool IsFlashing() const
    {
        return timer > 0.0f;
    }
};
