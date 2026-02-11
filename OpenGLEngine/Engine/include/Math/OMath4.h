#pragma once
#include <cstring>
#include "OVector3.h"

class OMath4
{
	public:
		OMath4()
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

		void SetScale(const OVector3& scale)
		{
			matrix[0][0] = scale.x;
			matrix[1][1] = scale.y;
			matrix[2][2] = scale.z;
		}

		void SetTranslation(const OVector3& translation)
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

		void operator *=(const OMath4& otherMatrix)
		{
			OMath4 out;
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
			*this = out;
		}

		void SetOrthogonalLeftHanded(float width, float height, float nearPlane, float farPlane)
		{
			matrix[0][0] = 2.0f / width;
			matrix[1][1] = 2.0f / height;
			matrix[2][2] = 1.0f / (farPlane - nearPlane);
			matrix[3][2] = -(nearPlane / (farPlane - nearPlane));
		}

		float matrix[4][4] = {};
};
