#pragma once
#include "Math/Vector3.h"

struct AmbientLightComponent
{
    Vector3 color = {1.0f, 1.0f, 1.0f};
    float intensity = 0.2f;
};
