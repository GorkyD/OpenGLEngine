#pragma once

#include "Extension/Extension.h"
#include "Math/Vector3.h"

struct ProceduralSkyComponent
{
    ShaderProgramPtr shader;
    VertexArrayObjectPtr vao;
    TexturePtr moonTexture;

    Vector3 zenithColor = {0.02f, 0.03f, 0.08f};
    Vector3 horizonColor = {0.05f, 0.07f, 0.14f};
    Vector3 groundColor = {0.01f, 0.01f, 0.02f};

    float celestialSize = 0.05f;
    float celestialGlow = 32.0f;
    float celestialIntensity = 1.2f;

    bool starsEnabled = true;
    float starDensity = 0.997f;
    float starIntensity = 0.6f;
    float starRotationSpeed = 0.0015f;
};
