#include "Systems/HudSystem.h"

#include <glad/glad.h>
#include "Math/Matrix4.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Window/Window.h"

HudSystem::~HudSystem()
{
    if (vbo)
        glDeleteBuffers(1, &vbo);
    if (vao)
        glDeleteVertexArrays(1, &vao);
}

void HudSystem::Init(RenderEngine* re, const ShaderProgramPtr& shader, Window* win)
{
    renderEngine = re;
    hudShader = shader;
    window = win;

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

void HudSystem::Draw()
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
    glDisable(GL_CULL_FACE);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(vertices.size() * sizeof(HudVertex)), vertices.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void HudSystem::BuildCrosshair(float centerX, float centerY)
{
    const float half = crosshairThickness * 0.5f;

    AddQuad(centerX - crosshairGap - crosshairSize, centerY - half, crosshairSize, crosshairThickness);
    AddQuad(centerX + crosshairGap, centerY - half, crosshairSize, crosshairThickness);
    AddQuad(centerX - half, centerY - crosshairGap - crosshairSize, crosshairThickness, crosshairSize);
    AddQuad(centerX - half, centerY + crosshairGap, crosshairThickness, crosshairSize);
}

void HudSystem::AddQuad(float x, float y, float width, float height)
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
