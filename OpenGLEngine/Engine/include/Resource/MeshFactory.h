#pragma once

#include "Extension/Extension.h"

class RenderEngine;

class MeshFactory
{
public:
    static VertexArrayObjectPtr CreateCube(RenderEngine* renderEngine, unsigned int& outIndexCount, float size = 1.0f);
    static VertexArrayObjectPtr CreateSphere(RenderEngine* renderEngine, unsigned int& outIndexCount, int rings = 16, int sectors = 24, float radius = 1.0f);
    static VertexArrayObjectPtr CreateDisc(RenderEngine* renderEngine, unsigned int& outIndexCount, int segments = 20, float radius = 1.0f);
    static VertexArrayObjectPtr CreateGem(RenderEngine* renderEngine, unsigned int& outIndexCount, int sides = 6, float radius = 1.0f, float topHeight = 1.2f, float bottomHeight = 0.6f);
};
