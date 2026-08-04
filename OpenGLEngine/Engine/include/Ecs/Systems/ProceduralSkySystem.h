#pragma once

#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/ProceduralSkyComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Math/Vector3.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Render/Texture.h"
#include "Render/VertexArrayObject.h"
#include <glad/glad.h>

class ProceduralSkySystem : public IEcsSystem
{
public:
    ProceduralSkySystem(RenderEngine* re) : renderEngine(re) {}

    void Run(EcsWorld& world, float deltaTime) override
    {
        elapsedTime += deltaTime;

        for (auto& pair : world.GetPool<ProceduralSkyComponent>())
        {
            auto& sky = pair.second;
            if (!sky.shader || !sky.vao)
                continue;

            Vector3 lightDir = {0.0f, 1.0f, 0.0f};
            Vector3 lightColor = {1.0f, 1.0f, 1.0f};
            for (auto& lightPair : world.GetPool<LightComponent>())
            {
                if (lightPair.second.type != LightType::Directional)
                    continue;
                lightDir = Vector3::Normalize(lightPair.second.direction);
                lightColor = lightPair.second.color;
                break;
            }

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);

            renderEngine->SetShaderProgram(sky.shader);

            glUniform3f(sky.shader->GetUniformLocation("zenithColor"), sky.zenithColor.x, sky.zenithColor.y, sky.zenithColor.z);
            glUniform3f(sky.shader->GetUniformLocation("horizonColor"), sky.horizonColor.x, sky.horizonColor.y, sky.horizonColor.z);
            glUniform3f(sky.shader->GetUniformLocation("groundColor"), sky.groundColor.x, sky.groundColor.y, sky.groundColor.z);
            glUniform3f(sky.shader->GetUniformLocation("lightDir"), lightDir.x, lightDir.y, lightDir.z);
            glUniform3f(sky.shader->GetUniformLocation("lightColor"), lightColor.x, lightColor.y, lightColor.z);
            glUniform1f(sky.shader->GetUniformLocation("celestialSize"), sky.celestialSize);
            glUniform1f(sky.shader->GetUniformLocation("celestialGlow"), sky.celestialGlow);
            glUniform1f(sky.shader->GetUniformLocation("celestialIntensity"), sky.celestialIntensity);
            glUniform1i(sky.shader->GetUniformLocation("starsEnabled"), sky.starsEnabled ? 1 : 0);
            glUniform1f(sky.shader->GetUniformLocation("starDensity"), sky.starDensity);
            glUniform1f(sky.shader->GetUniformLocation("starIntensity"), sky.starIntensity);
            glUniform1f(sky.shader->GetUniformLocation("starRotation"), elapsedTime * sky.starRotationSpeed);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sky.moonTexture ? sky.moonTexture->GetId() : 0);
            glUniform1i(sky.shader->GetUniformLocation("moonTexture"), 0);
            glUniform1i(sky.shader->GetUniformLocation("hasMoonTexture"), sky.moonTexture ? 1 : 0);

            renderEngine->SetVertexArrayObject(sky.vao);
            renderEngine->DrawTriangles(List, 36, 0);

            glEnable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST);

            break;
        }
    }

private:
    RenderEngine* renderEngine;
    float elapsedTime = 0.0f;
};
