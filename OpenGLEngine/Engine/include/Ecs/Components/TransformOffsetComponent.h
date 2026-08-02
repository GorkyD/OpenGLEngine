#pragma once

#include "Math/Matrix4.h"
#include "Math/Vector3.h"

struct TransformOffsetComponent
{
    Vector3 position = {0, 0, 0};
    Vector3 rotationDegrees = {0, 0, 0};
    Vector3 scale = {1, 1, 1};

    bool procedural = false;

    Matrix4 GetRotationMatrix() const
    {
        constexpr float deg2rad = 3.14159265f / 180.0f;
        return Matrix4::FromAxisAngle({1.0f, 0.0f, 0.0f}, rotationDegrees.x * deg2rad) * Matrix4::FromAxisAngle({0.0f, 1.0f, 0.0f}, rotationDegrees.y * deg2rad) *
               Matrix4::FromAxisAngle({0.0f, 0.0f, 1.0f}, rotationDegrees.z * deg2rad);
    }
};
