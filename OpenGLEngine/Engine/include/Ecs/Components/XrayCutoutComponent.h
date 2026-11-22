#pragma once

#include <array>
#include "Math/Vector3.h"

struct XrayCutoutComponent
{
    std::array<Vector3, 4> centers = {};
    int count = 0;
    float radius = 0.012f;
    float depthMargin = 0.0025f;
};
