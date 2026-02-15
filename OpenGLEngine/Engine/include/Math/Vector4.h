#pragma once
class Vector4 
{
public: 
	Vector4(){}

	Vector4(float x, float y, float z, float w): x(x),y(y),z(z),w(w){}

	~Vector4() {}

	float x = 0, y = 0, z = 0, w = 0;
};
