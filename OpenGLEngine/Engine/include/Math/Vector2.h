#pragma once

class Vector2
{
public:
	Vector2() {}

	Vector2(float x, float y) : x(x), y(y){}

	~Vector2() {}

	Vector2 operator+(const Vector2& v) const { return Vector2(x + v.x, y + v.y); }
	Vector2 operator-(const Vector2& v) const { return Vector2(x - v.x, y - v.y); }
	Vector2 operator*(float s) const { return Vector2(x * s, y * s); }
	Vector2 operator-() const { return Vector2(-x, -y); }

	Vector2& operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }
	Vector2& operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }


	float x = 0, y = 0;
};
