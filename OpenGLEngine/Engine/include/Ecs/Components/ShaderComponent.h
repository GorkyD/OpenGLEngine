#pragma once

#include "Extension/Extension.h"

enum class ShaderRenderType
{
    Unlit,
    Lit,
    Fire
};

struct ShaderComponent
{
    ShaderProgramPtr shader;
    ShaderRenderType shaderType = ShaderRenderType::Unlit;
};
