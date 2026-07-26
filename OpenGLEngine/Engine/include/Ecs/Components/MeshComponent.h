#pragma once

#include "Extension/Extension.h"

struct MeshComponent
{
    VertexArrayObjectPtr vao;
    unsigned int indexCount = 0;
    bool visible = true;
};
