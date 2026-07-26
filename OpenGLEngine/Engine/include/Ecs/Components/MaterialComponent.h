#pragma once

#include "Extension/Extension.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

struct MaterialComponent
{
    TexturePtr diffuseTexture;
    TexturePtr normalTexture;
    TexturePtr roughnessTexture;
    TexturePtr metallicTexture;
    TexturePtr aoTexture;

    Vector4 diffuseColor = {1, 1, 1, 1};
    Vector3 emissiveColor = {0, 0, 0};

    float emissiveIntensity = 0.0f;
};
