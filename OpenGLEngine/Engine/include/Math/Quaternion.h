#pragma once

#include <cmath>
#include "Vector3.h"

class Matrix4;

class Quaternion
{
public:
    Quaternion() {}

    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    static Quaternion Identity()
    {
        return Quaternion(0, 0, 0, 1);
    }

    static Quaternion FromAxisAngle(const Vector3& axis, float angle)
    {
        const float halfAngle = angle * 0.5f;
        const float s = std::sin(halfAngle);
        return Quaternion(axis.x * s, axis.y * s, axis.z * s, std::cos(halfAngle));
    }

    static float Dot(const Quaternion& a, const Quaternion& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    static Quaternion Normalize(const Quaternion& q)
    {
        const float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        if (len < 1e-8f)
            return Quaternion::Identity();
        const float inv = 1.0f / len;
        return Quaternion(q.x * inv, q.y * inv, q.z * inv, q.w * inv);
    }

    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t)
    {
        Quaternion end = b;
        float cosOmega = Dot(a, b);
        if (cosOmega < 0.0f)
        {
            cosOmega = -cosOmega;
            end = Quaternion(-b.x, -b.y, -b.z, -b.w);
        }

        float aScale, bScale;
        if (cosOmega > 0.9995f)
        {
            aScale = 1.0f - t;
            bScale = t;
        }
        else
        {
            const float omega = std::acos(cosOmega);
            const float invSinOmega = 1.0f / std::sin(omega);
            aScale = std::sin((1.0f - t) * omega) * invSinOmega;
            bScale = std::sin(t * omega) * invSinOmega;
        }

        return Normalize(Quaternion(a.x * aScale + end.x * bScale, a.y * aScale + end.y * bScale, a.z * aScale + end.z * bScale, a.w * aScale + end.w * bScale));
    }

    Matrix4 ToMatrix4() const;

    float x = 0, y = 0, z = 0, w = 1;
};
