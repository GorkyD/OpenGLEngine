#pragma once
#include "Math/Matrix4.h"

struct UniformData
{
	Matrix4 world;
	Matrix4 view;
	Matrix4 projection;
};
