#pragma once

#include <vector>
#include "Extension/Extension.h"
#include "Math/Vector4.h"

class RenderEngine;
class Window;

struct HudVertex
{
    float x = 0.0f;
    float y = 0.0f;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

class HudSystem
{
public:
    ~HudSystem();

    void Init(RenderEngine* renderEngine, const ShaderProgramPtr& shader, Window* window);
    void Draw();

    bool showCrosshair = true;
    float crosshairSize = 10.0f;
    float crosshairGap = 4.0f;
    float crosshairThickness = 2.0f;
    Vector4 crosshairColor = {1.0f, 1.0f, 1.0f, 0.85f};

private:
    void BuildCrosshair(float centerX, float centerY);
    void AddQuad(float x, float y, float width, float height);

    static constexpr size_t bufferCapacity = 256;

    RenderEngine* renderEngine = nullptr;
    ShaderProgramPtr hudShader;
    Window* window = nullptr;

    std::vector<HudVertex> vertices;

    unsigned int vao = 0;
    unsigned int vbo = 0;
};
