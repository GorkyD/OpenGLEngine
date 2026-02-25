#pragma once
#include <cstring>
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

	Matrix4 operator *(const Matrix4& otherMatrix) const
	{
		Matrix4 out;
		for (auto i = 0; i < 4; i++)
		{
			for (auto j = 0; j < 4; j++)
			{
				out.matrix[i][j] =
					matrix[i][0] * otherMatrix.matrix[0][j] +
					matrix[i][1] * otherMatrix.matrix[1][j] +
					matrix[i][2] * otherMatrix.matrix[2][j] +
					matrix[i][3] * otherMatrix.matrix[3][j];
			}
		}
		return out;
	}

	void operator *=(const Matrix4& otherMatrix)
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
			matrix[0][0] = right.x;    matrix[0][1] = newUp.x;    matrix[0][2] = forward.x;
			matrix[1][0] = right.y;    matrix[1][1] = newUp.y;    matrix[1][2] = forward.y;
			matrix[2][0] = right.z;    matrix[2][1] = newUp.z;    matrix[2][2] = forward.z;
			matrix[3][0] = -Vector3::Dot(right, eye);
			matrix[3][1] = -Vector3::Dot(newUp, eye);
			matrix[3][2] = -Vector3::Dot(forward, eye);
			matrix[3][3] = 1.0f;
		}

		float matrix[4][4] = {};
};
