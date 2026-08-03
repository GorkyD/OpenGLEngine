#pragma once

#include "Ecs/Components/TextComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Render/Font.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Render/Texture.h"
#include "Window/Window.h"
#include "Math/Matrix4.h"
#include <glad/glad.h>
#include <unordered_map>

class UITextSystem : public IEcsSystem
{
public:
    UITextSystem(RenderEngine* re, const ShaderProgramPtr& shader, Window* window) : renderEngine(re), textShader(shader), window(window) {}

    void Init(EcsWorld& world) override
    {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, bufferCapacity * sizeof(FontGlyphVertex), nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(FontGlyphVertex), static_cast<void*>(nullptr));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(FontGlyphVertex), reinterpret_cast<void*>(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(FontGlyphVertex), reinterpret_cast<void*>(4 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    ~UITextSystem() override
    {
        if (vbo)
            glDeleteBuffers(1, &vbo);
        if (vao)
            glDeleteVertexArrays(1, &vao);
    }

    void Run(EcsWorld& world, float deltaTime) override
    {
        if (!textShader)
            return;

        auto& texts = world.GetPool<TextComponent>();

        std::unordered_map<Texture*, std::vector<FontGlyphVertex>> batches;

        for (auto& pair : texts)
        {
            auto& text = pair.second;
            if (!text.visible || !text.font || text.text.empty())
                continue;

            Texture* atlasPtr = text.font->GetAtlasTexture().get();
            if (!atlasPtr)
                continue;

            const auto vertices = text.font->BuildGeometry(text.text, text.position.x, text.position.y, text.scale, text.color);
            if (vertices.empty())
                continue;

            auto& batch = batches[atlasPtr];
            batch.insert(batch.end(), vertices.begin(), vertices.end());
        }

        if (batches.empty())
            return;

        const Rect screen = window->GetInnerSize();

        Matrix4 projection;
        projection.SetOrthographicOffCenter(0.0f, static_cast<float>(screen.width), static_cast<float>(screen.height), 0.0f, -1.0f, 1.0f);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        renderEngine->SetShaderProgram(textShader);
        glUniformMatrix4fv(textShader->GetUniformLocation("projection"), 1, GL_FALSE, &projection.matrix[0][0]);
        glUniform1i(textShader->GetUniformLocation("atlas"), 0);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glActiveTexture(GL_TEXTURE0);

        for (auto& batchPair : batches)
        {
            Texture* atlasPtr = batchPair.first;
            auto& vertices = batchPair.second;
            if (vertices.empty())
                continue;

            EnsureCapacity(vertices.size());

            glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(FontGlyphVertex), vertices.data());

            glBindTexture(GL_TEXTURE_2D, atlasPtr->GetId());

            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
        }

        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
    }

private:
    void EnsureCapacity(size_t vertexCount)
    {
        if (vertexCount <= bufferCapacity)
            return;

        bufferCapacity = vertexCount * 2;
        glBufferData(GL_ARRAY_BUFFER, bufferCapacity * sizeof(FontGlyphVertex), nullptr, GL_DYNAMIC_DRAW);
    }

    RenderEngine* renderEngine;
    ShaderProgramPtr textShader;
    Window* window;

    unsigned int vao = 0;
    unsigned int vbo = 0;

    size_t bufferCapacity = 1024;
};
