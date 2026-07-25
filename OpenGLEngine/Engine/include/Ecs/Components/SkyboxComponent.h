#pragma once
#include "Extension/Extension.h"

struct SkyboxComponent
{
    ShaderProgramPtr shader;
    TexturePtr cubemap;
    VertexArrayObjectPtr vao;
};
