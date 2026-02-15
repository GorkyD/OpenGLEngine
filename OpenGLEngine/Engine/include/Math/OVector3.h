#pragma once
#include <cmath>

class OVector3
{
public:
	OVector3() {}

	OVector3(float x, float y, float z) : x(x), y(y), z(z) {}

	~OVector3() {}

	OVector3 operator+(const OVector3& v) const { return OVector3(x + v.x, y + v.y, z + v.z); }
	OVector3 operator-(const OVector3& v) const { return OVector3(x - v.x, y - v.y, z - v.z); }
	OVector3 operator*(float s) const { return OVector3(x * s, y * s, z * s); }
	OVector3 operator-() const { return OVector3(-x, -y, -z); }

	OVector3& operator+=(const OVector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
	OVector3& operator-=(const OVector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }

	static float Dot(const OVector3& a, const OVector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

	static OVector3 Cross(const OVector3& a, const OVector3& b)
	{
		return OVector3(
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		);
	}

	static OVector3 Normalize(const OVector3& v)
	{
		float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
		if (len < 1e-8f) return OVector3(0, 0, 0);
		float inv = 1.0f / len;
		return OVector3(v.x * inv, v.y * inv, v.z * inv);
	}

	float x = 0, y = 0, z = 0;
};
