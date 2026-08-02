#pragma once

#include <cmath>
#include "Math/Matrix4.h"
#include "Math/Vector3.h"

struct Plane
{
    Vector3 normal = {0, 1, 0};
    float distance = 0.0f;

    float SignedDistance(const Vector3& point) const
    {
        return normal.x * point.x + normal.y * point.y + normal.z * point.z + distance;
    }
};

class Frustum
{
public:
    void ExtractFromViewProjection(const Matrix4& viewProjection)
    {
        const auto& m = viewProjection.matrix;

        SetPlane(Left, m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]);
        SetPlane(Right, m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]);
        SetPlane(Bottom, m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]);
        SetPlane(Top, m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]);
        SetPlane(Near, m[0][2], m[1][2], m[2][2], m[3][2]);
        SetPlane(Far, m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]);
    }

    bool IntersectsBox(const Vector3& min, const Vector3& max) const
    {
        for (const auto& plane : planes)
        {
            const Vector3 positive = {plane.normal.x >= 0.0f ? max.x : min.x, plane.normal.y >= 0.0f ? max.y : min.y, plane.normal.z >= 0.0f ? max.z : min.z};

            if (plane.SignedDistance(positive) < 0.0f)
                return false;
        }

        return true;
    }

private:
    enum PlaneIndex
    {
        Left = 0,
        Right,
        Bottom,
        Top,
        Near,
        Far,
        PlaneCount
    };

    void SetPlane(int index, float x, float y, float z, float w)
    {
        const float length = std::sqrt(x * x + y * y + z * z);
        if (length < 0.000001f)
        {
            planes[index] = Plane();
            return;
        }

        planes[index].normal = {x / length, y / length, z / length};
        planes[index].distance = w / length;
    }

    Plane planes[PlaneCount];
};
