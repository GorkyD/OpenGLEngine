#pragma once

#include "Extension/Extension.h"

enum class ShaderRenderType
{
    Unlit,
    Lit,
    Skinned,
    Fire,
    Shadow
};

struct ShaderComponent
{
    ShaderProgramPtr shader;
    ShaderRenderType shaderType = ShaderRenderType::Unlit;
};
