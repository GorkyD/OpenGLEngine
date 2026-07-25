#pragma once
#include "Ecs/Components/SkyboxComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Render/Texture.h"
#include "Render/VertexArrayObject.h"
#include <glad/glad.h>

class SkyboxSystem : public IEcsSystem
{
public:
    SkyboxSystem(RenderEngine* re) : renderEngine(re) {}

    void Run(EcsWorld& world, float deltaTime) override
    {
        for (auto& pair : world.GetPool<SkyboxComponent>())
        {
            auto& skybox = pair.second;
            if (!skybox.shader || !skybox.cubemap || !skybox.vao)
                continue;

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);

            renderEngine->SetShaderProgram(skybox.shader);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(skybox.cubemap->GetTarget(), skybox.cubemap->GetId());
            glUniform1i(skybox.shader->GetUniformLocation("skyboxTexture"), 0);

            renderEngine->SetVertexArrayObject(skybox.vao);
            renderEngine->DrawTriangles(List, 36, 0);

            glEnable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST);

            break;
        }
    }

private:
    RenderEngine* renderEngine;
};
