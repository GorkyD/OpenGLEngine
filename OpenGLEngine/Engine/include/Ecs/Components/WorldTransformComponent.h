#pragma once

#include "Math/Matrix4.h"

struct WorldTransformComponent
{
    Matrix4 matrix;
};

struct LocalMatrixComponent
{
    Matrix4 matrix;
};
