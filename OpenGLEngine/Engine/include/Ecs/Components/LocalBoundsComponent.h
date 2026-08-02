#pragma once

#include "Math/Vector3.h"

struct LocalBoundsComponent
{
    Vector3 min = {0, 0, 0};
    Vector3 max = {0, 0, 0};
};
