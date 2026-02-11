#pragma once
class OVector4 
{
public: 
	OVector4(){}

	OVector4(float x, float y, float z, float w): x(x),y(y),z(z),w(w){}

	~OVector4() {}

	float x = 0, y = 0, z = 0, w = 0;
};