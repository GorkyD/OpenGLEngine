#pragma once
class OVector3
{
public:
	OVector3() {}

	OVector3(float x, float y, float z) : x(x), y(y), z(z) {}

	~OVector3() {}

	float x = 0, y = 0, z = 0;
};
