#pragma once
#include "Extension/Extension.h"

class RenderEngine;

class MeshFactory
{
public:
    static VertexArrayObjectPtr CreateSphere(RenderEngine* renderEngine, unsigned int& outIndexCount, int rings = 16,
                                             int sectors = 24, float radius = 1.0f);
};
