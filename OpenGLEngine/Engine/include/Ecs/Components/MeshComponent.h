#pragma once

#include "Extension/Extension.h"

enum class RenderQueue
{
    Default,
    Overlay
};

struct MeshComponent
{
    VertexArrayObjectPtr vao;
    unsigned int indexCount = 0;
    bool visible = true;
    bool castsShadow = true;
    RenderQueue renderQueue = RenderQueue::Default;
};
