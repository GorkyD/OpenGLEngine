#pragma once
#include "Math/Vector3.h"

struct TransformComponent
{
   Vector3 position = { 0, 0, 0 };
   Vector3 scale = { 1, 1, 1 };
};
