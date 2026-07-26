#pragma once

#include "Extension/Extension.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include <string>

struct TextComponent
{
    std::string text;

    FontPtr font;

    Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector2 position = {0.0f, 0.0f};

    float scale = 1.0f;

    bool visible = true;
};
