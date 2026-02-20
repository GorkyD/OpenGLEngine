#pragma once
#include <cmath>

class Vector3
{
	public:
		Vector3() {}

		Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

		~Vector3() {}

		Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
		Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
		Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
		Vector3 operator/(float s) const { return Vector3(x / s, y / s, z / s); }
		Vector3 operator-() const { return Vector3(-x, -y, -z); }

		Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
		Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }

		static float Dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

		static Vector3 Cross(const Vector3& a, const Vector3& b)
		{
			return Vector3(
				a.y * b.z - a.z * b.y,
				a.z * b.x - a.x * b.z,
				a.x * b.y - a.y * b.x
			);
		}

		static Vector3 Normalize(const Vector3& v)
		{
			float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
			if (len < 1e-8f) return Vector3(0, 0, 0);
			float inv = 1.0f / len;
			return Vector3(v.x * inv, v.y * inv, v.z * inv);
		}

		float x = 0, y = 0, z = 0;
};
