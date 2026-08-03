#pragma once

#include <cstring>
#include "Quaternion.h"
#include "Vector3.h"

class Matrix4
{
public:
    Matrix4()
    {
        SetIdentity();
    }

    void SetIdentity()
    {
        ::memset(matrix, 0, sizeof(matrix));
        matrix[0][0] = 1;
        matrix[1][1] = 1;
        matrix[2][2] = 1;
        matrix[3][3] = 1;
    }

    void SetScale(const Vector3& scale)
    {
        matrix[0][0] = scale.x;
        matrix[1][1] = scale.y;
        matrix[2][2] = scale.z;
    }

    void SetTranslation(const Vector3& translation)
    {
        matrix[3][0] = translation.x;
        matrix[3][1] = translation.y;
        matrix[3][2] = translation.z;
    }

    void SetRotationX(float x)
    {
        matrix[1][1] = cos(x);
        matrix[1][2] = sin(x);
        matrix[2][1] = -sin(x);
        matrix[2][2] = cos(x);
    }

    void SetRotationY(float y)
    {
        matrix[0][0] = cos(y);
        matrix[0][2] = -sin(y);
        matrix[2][0] = sin(y);
        matrix[2][2] = cos(y);
    }

    void SetRotationZ(float z)
    {
        matrix[0][0] = cos(z);
        matrix[0][1] = sin(z);
        matrix[1][0] = -sin(z);
        matrix[1][1] = cos(z);
    }

    static Matrix4 FromAxisAngle(const Vector3& axis, float angle)
    {
        float c = std::cos(angle);
        float s = std::sin(angle);
        float t = 1.0f - c;

        Matrix4 m;
        m.matrix[0][0] = t * axis.x * axis.x + c;
        m.matrix[0][1] = t * axis.x * axis.y + s * axis.z;
        m.matrix[0][2] = t * axis.x * axis.z - s * axis.y;

        m.matrix[1][0] = t * axis.x * axis.y - s * axis.z;
        m.matrix[1][1] = t * axis.y * axis.y + c;
        m.matrix[1][2] = t * axis.y * axis.z + s * axis.x;

        m.matrix[2][0] = t * axis.x * axis.z + s * axis.y;
        m.matrix[2][1] = t * axis.y * axis.z - s * axis.x;
        m.matrix[2][2] = t * axis.z * axis.z + c;

        return m;
    }

    static Matrix4 Compose(const Vector3& translation, const Quaternion& rotation, const Vector3& scale)
    {
        Matrix4 s;
        s.SetScale(scale);

        Matrix4 t;
        t.SetTranslation(translation);

        return s * rotation.ToMatrix4() * t;
    }

    Vector3 TransformPoint(const Vector3& v) const
    {
        return {v.x * matrix[0][0] + v.y * matrix[1][0] + v.z * matrix[2][0] + matrix[3][0], v.x * matrix[0][1] + v.y * matrix[1][1] + v.z * matrix[2][1] + matrix[3][1],
                v.x * matrix[0][2] + v.y * matrix[1][2] + v.z * matrix[2][2] + matrix[3][2]};
    }

    Vector3 TransformDirection(const Vector3& v) const
    {
        return {v.x * matrix[0][0] + v.y * matrix[1][0] + v.z * matrix[2][0], v.x * matrix[0][1] + v.y * matrix[1][1] + v.z * matrix[2][1],
                v.x * matrix[0][2] + v.y * matrix[1][2] + v.z * matrix[2][2]};
    }

    Matrix4 operator*(const Matrix4& otherMatrix) const
    {
        Matrix4 out;
        for (auto i = 0; i < 4; i++)
        {
            for (auto j = 0; j < 4; j++)
            {
                out.matrix[i][j] = matrix[i][0] * otherMatrix.matrix[0][j] + matrix[i][1] * otherMatrix.matrix[1][j] + matrix[i][2] * otherMatrix.matrix[2][j] + matrix[i][3] * otherMatrix.matrix[3][j];
            }
        }
        return out;
    }

    void operator*=(const Matrix4& otherMatrix)
    {
        *this = *this * otherMatrix;
    }

    void SetOrthogonalLeftHanded(float width, float height, float nearPlane, float farPlane)
    {
        matrix[0][0] = 2.0f / width;
        matrix[1][1] = 2.0f / height;
        matrix[2][2] = 1.0f / (farPlane - nearPlane);
        matrix[3][2] = -(nearPlane / (farPlane - nearPlane));
    }

    void SetOrthographicOffCenter(float left, float right, float bottom, float top, float nearPlane, float farPlane)
    {
        ::memset(matrix, 0, sizeof(matrix));
        matrix[0][0] = 2.0f / (right - left);
        matrix[1][1] = 2.0f / (top - bottom);
        matrix[2][2] = 1.0f / (farPlane - nearPlane);
        matrix[3][0] = -(right + left) / (right - left);
        matrix[3][1] = -(top + bottom) / (top - bottom);
        matrix[3][2] = -nearPlane / (farPlane - nearPlane);
        matrix[3][3] = 1.0f;
    }

    void SetPerspectiveLeftHanded(float fovY, float aspect, float nearPlane, float farPlane)
    {
        ::memset(matrix, 0, sizeof(matrix));
        float f = 1.0f / std::tan(fovY * 0.5f);
        matrix[0][0] = f / aspect;
        matrix[1][1] = f;
        matrix[2][2] = farPlane / (farPlane - nearPlane);
        matrix[2][3] = 1.0f;
        matrix[3][2] = -(nearPlane * farPlane) / (farPlane - nearPlane);
    }

    void SetLookAtLeftHanded(const Vector3& eye, const Vector3& target, const Vector3& up)
    {
        Vector3 forward = Vector3::Normalize(target - eye);
        Vector3 right = Vector3::Normalize(Vector3::Cross(up, forward));
        Vector3 newUp = Vector3::Cross(forward, right);

        ::memset(matrix, 0, sizeof(matrix));
        matrix[0][0] = right.x;
        matrix[0][1] = newUp.x;
        matrix[0][2] = forward.x;
        matrix[1][0] = right.y;
        matrix[1][1] = newUp.y;
        matrix[1][2] = forward.y;
        matrix[2][0] = right.z;
        matrix[2][1] = newUp.z;
        matrix[2][2] = forward.z;
        matrix[3][0] = -Vector3::Dot(right, eye);
        matrix[3][1] = -Vector3::Dot(newUp, eye);
        matrix[3][2] = -Vector3::Dot(forward, eye);
        matrix[3][3] = 1.0f;
    }

    void GetNormalMatrix(float outMatrix[9]) const
    {
        const float a00 = matrix[0][0], a01 = matrix[0][1], a02 = matrix[0][2];
        const float a10 = matrix[1][0], a11 = matrix[1][1], a12 = matrix[1][2];
        const float a20 = matrix[2][0], a21 = matrix[2][1], a22 = matrix[2][2];

        const float det = a00 * (a11 * a22 - a12 * a21) - a01 * (a10 * a22 - a12 * a20) + a02 * (a10 * a21 - a11 * a20);
        const float invDet = (det != 0.0f) ? 1.0f / det : 0.0f;

        outMatrix[0] = (a11 * a22 - a12 * a21) * invDet;
        outMatrix[1] = -(a01 * a22 - a02 * a21) * invDet;
        outMatrix[2] = (a01 * a12 - a02 * a11) * invDet;
        outMatrix[3] = -(a10 * a22 - a12 * a20) * invDet;
        outMatrix[4] = (a00 * a22 - a02 * a20) * invDet;
        outMatrix[5] = -(a00 * a12 - a02 * a10) * invDet;
        outMatrix[6] = (a10 * a21 - a11 * a20) * invDet;
        outMatrix[7] = -(a00 * a21 - a01 * a20) * invDet;
        outMatrix[8] = (a00 * a11 - a01 * a10) * invDet;
    }

    Matrix4 Inverse() const
    {
        const float* m = &matrix[0][0];
        float inv[16];

        inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
        inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
        inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
        inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
        inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
        inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
        inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
        inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
        inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
        inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
        inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
        inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
        inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
        inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
        inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
        inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

        const float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
        const float invDet = (det != 0.0f) ? 1.0f / det : 0.0f;

        Matrix4 out;
        float* outData = &out.matrix[0][0];
        for (int i = 0; i < 16; i++)
            outData[i] = inv[i] * invDet;
        return out;
    }

    float matrix[4][4] = {};
};

inline Matrix4 Quaternion::ToMatrix4() const
{
    Matrix4 m;
    m.matrix[0][0] = 1 - 2 * (y * y + z * z);
    m.matrix[0][1] = 2 * (x * y + w * z);
    m.matrix[0][2] = 2 * (x * z - w * y);

    m.matrix[1][0] = 2 * (x * y - w * z);
    m.matrix[1][1] = 1 - 2 * (x * x + z * z);
    m.matrix[1][2] = 2 * (y * z + w * x);

    m.matrix[2][0] = 2 * (x * z + w * y);
    m.matrix[2][1] = 2 * (y * z - w * x);
    m.matrix[2][2] = 1 - 2 * (x * x + y * y);

    return m;
}
