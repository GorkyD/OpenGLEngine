#pragma once
#include "Extension/Extension.h"
#include "Math/Vector4.h"

struct MaterialComponent
{
	TexturePtr diffuseTexture;
	Vector4 diffuseColor = {1, 1, 1, 1};
};
