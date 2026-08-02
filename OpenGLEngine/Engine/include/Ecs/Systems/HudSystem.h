#pragma once

#include <vector>
#include <glad/glad.h>
#include "Ecs/Core/IEcsSystem.h"
#include "Math/Matrix4.h"
#include "Math/Vector4.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Window/Window.h"

struct HudVertex
{
    float x = 0.0f;
    float y = 0.0f;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

class HudSystem : public IEcsSystem
{
public:
    HudSystem(RenderEngine* renderEngine, const ShaderProgramPtr& shader, Window* window) : renderEngine(renderEngine), hudShader(shader), window(window) {}

    bool showCrosshair = true;
    float crosshairSize = 10.0f;
    float crosshairGap = 4.0f;
    float crosshairThickness = 2.0f;
    Vector4 crosshairColor = {1.0f, 1.0f, 1.0f, 0.85f};

    void Init(EcsWorld& world) override
    {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, bufferCapacity * sizeof(HudVertex), nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(HudVertex), static_cast<void*>(nullptr));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(HudVertex), reinterpret_cast<void*>(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    ~HudSystem() override
    {
        if (vbo)
            glDeleteBuffers(1, &vbo);
        if (vao)
            glDeleteVertexArrays(1, &vao);
    }

    void Run(EcsWorld& world, float deltaTime) override
    {
        if (!hudShader || !showCrosshair)
            return;

        const Rect screen = window->GetInnerSize();
        if (screen.width <= 0 || screen.height <= 0)
            return;

        vertices.clear();
        BuildCrosshair(static_cast<float>(screen.width) * 0.5f, static_cast<float>(screen.height) * 0.5f);

        if (vertices.empty())
            return;

        Matrix4 projection;
        projection.SetOrthographicOffCenter(0.0f, static_cast<float>(screen.width), static_cast<float>(screen.height), 0.0f, -1.0f, 1.0f);

        renderEngine->SetShaderProgram(hudShader);
        glUniformMatrix4fv(hudShader->GetUniformLocation("projection"), 1, GL_FALSE, &projection.matrix[0][0]);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(vertices.size() * sizeof(HudVertex)), vertices.data());
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

private:
    void BuildCrosshair(float centerX, float centerY)
    {
        const float half = crosshairThickness * 0.5f;

        AddQuad(centerX - crosshairGap - crosshairSize, centerY - half, crosshairSize, crosshairThickness);
        AddQuad(centerX + crosshairGap, centerY - half, crosshairSize, crosshairThickness);
        AddQuad(centerX - half, centerY - crosshairGap - crosshairSize, crosshairThickness, crosshairSize);
        AddQuad(centerX - half, centerY + crosshairGap, crosshairThickness, crosshairSize);
    }

    void AddQuad(float x, float y, float width, float height)
    {
        if (vertices.size() + 6 > bufferCapacity)
            return;

        const HudVertex topLeft = {x, y, crosshairColor.x, crosshairColor.y, crosshairColor.z, crosshairColor.w};
        const HudVertex topRight = {x + width, y, crosshairColor.x, crosshairColor.y, crosshairColor.z, crosshairColor.w};
        const HudVertex bottomRight = {x + width, y + height, crosshairColor.x, crosshairColor.y, crosshairColor.z, crosshairColor.w};
        const HudVertex bottomLeft = {x, y + height, crosshairColor.x, crosshairColor.y, crosshairColor.z, crosshairColor.w};

        vertices.push_back(topLeft);
        vertices.push_back(bottomRight);
        vertices.push_back(topRight);

        vertices.push_back(topLeft);
        vertices.push_back(bottomLeft);
        vertices.push_back(bottomRight);
    }

    static constexpr size_t bufferCapacity = 256;

    RenderEngine* renderEngine;
    ShaderProgramPtr hudShader;
    Window* window;

    std::vector<HudVertex> vertices;

    unsigned int vao = 0;
    unsigned int vbo = 0;
};
