#pragma once
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/AmbientLightComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Render/UniformBuffer.h"
#include "Render/Texture.h"
#include "Math/Matrix4.h"
#include "Math/Vector4.h"
#include <glad/glad.h>

class RenderSystem : public IEcsSystem
{
public:
    static constexpr int MaxLights = 8;

    RenderSystem(RenderEngine* re, UniformBufferPtr ub) : renderEngine(re), uniformBuffer(ub) {}

    void Run(EcsWorld& world, float deltaTime) override
    {
        elapsedTime += deltaTime;

        auto& meshes = world.GetPool<MeshComponent>();
        auto& shaders = world.GetPool<ShaderComponent>();
        auto& materials = world.GetPool<MaterialComponent>();
        auto& transforms = world.GetPool<TransformComponent>();

        AmbientLightComponent ambient;
        for (auto& pair : world.GetPool<AmbientLightComponent>())
        {
            ambient = pair.second;
            break;
        }

        int numLights = 0;
        int lightType[MaxLights] = {};
        float lightColor[MaxLights * 3] = {};
        float lightIntensity[MaxLights] = {};
        float lightDirection[MaxLights * 3] = {};
        float lightPosition[MaxLights * 3] = {};
        float lightRange[MaxLights] = {};

        for (auto& pair : world.GetPool<LightComponent>())
        {
            if (numLights >= MaxLights)
                break;

            const auto& light = pair.second;
            lightType[numLights] = static_cast<int>(light.type);
            lightColor[numLights * 3 + 0] = light.color.x;
            lightColor[numLights * 3 + 1] = light.color.y;
            lightColor[numLights * 3 + 2] = light.color.z;
            lightIntensity[numLights] = light.intensity;
            lightDirection[numLights * 3 + 0] = light.direction.x;
            lightDirection[numLights * 3 + 1] = light.direction.y;
            lightDirection[numLights * 3 + 2] = light.direction.z;
            lightPosition[numLights * 3 + 0] = light.position.x;
            lightPosition[numLights * 3 + 1] = light.position.y;
            lightPosition[numLights * 3 + 2] = light.position.z;
            lightRange[numLights] = light.range;
            numLights++;
        }

        for (auto& pair : meshes)
        {
            Entity entity = pair.first;
            auto& mesh = pair.second;

            if (!shaders.Has(entity))
                continue;

            Matrix4 worldMatrix;
            if (transforms.Has(entity))
                worldMatrix = transforms.Get(entity).GetModelMatrix();

            uniformBuffer->SetSubData(&worldMatrix, 0, sizeof(Matrix4));

            auto& shaderComp = shaders.Get(entity);
            renderEngine->SetShaderProgram(shaderComp.shader);

            Vector4 color = {1, 1, 1, 1};
            int hasTexture = 0;
            int hasNormalMap = 0;
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);

            if (materials.Has(entity))
            {
                auto& mat = materials.Get(entity);
                color = mat.diffuseColor;
                if (mat.diffuseTexture)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mat.diffuseTexture->GetId());
                    hasTexture = 1;
                }
                if (mat.normalTexture)
                {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, mat.normalTexture->GetId());
                    hasNormalMap = 1;
                }
            }

            auto* shader = shaderComp.shader.get();

            glUniform4f(shader->GetUniformLocation("diffuseColor"), color.x, color.y, color.z, color.w);
            glUniform1i(shader->GetUniformLocation("hasTexture"), hasTexture);

            if (shaderComp.shaderType == ShaderRenderType::Lit)
            {
                glUniform1i(shader->GetUniformLocation("normalTexture"), 1);
                glUniform1i(shader->GetUniformLocation("hasNormalMap"), hasNormalMap);
                glUniform3f(shader->GetUniformLocation("ambientColor"), ambient.color.x, ambient.color.y, ambient.color.z);
                glUniform1f(shader->GetUniformLocation("ambientIntensity"), ambient.intensity);

                glUniform1i(shader->GetUniformLocation("numLights"), numLights);
                if (numLights > 0)
                {
                    glUniform1iv(shader->GetUniformLocation("lightType"), numLights, lightType);
                    glUniform3fv(shader->GetUniformLocation("lightColor"), numLights, lightColor);
                    glUniform1fv(shader->GetUniformLocation("lightIntensity"), numLights, lightIntensity);
                    glUniform3fv(shader->GetUniformLocation("lightDirection"), numLights, lightDirection);
                    glUniform3fv(shader->GetUniformLocation("lightPosition"), numLights, lightPosition);
                    glUniform1fv(shader->GetUniformLocation("lightRange"), numLights, lightRange);
                }

                float normalMatrix[9];
                worldMatrix.GetNormalMatrix(normalMatrix);
                glUniformMatrix3fv(shader->GetUniformLocation("normalMatrix"), 1, GL_TRUE, normalMatrix);
            }

            const bool isFire = shaderComp.shaderType == ShaderRenderType::Fire;
            if (isFire)
            {
                glUniform1f(shader->GetUniformLocation("time"), elapsedTime);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);
                glDisable(GL_CULL_FACE);
            }

            renderEngine->SetVertexArrayObject(mesh.vao);
            renderEngine->DrawIndexedTriangles(List, mesh.indexCount);

            if (isFire)
            {
                glEnable(GL_CULL_FACE);
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
            }
        }
    }

private:
    RenderEngine* renderEngine;
    UniformBufferPtr uniformBuffer;
    float elapsedTime = 0.0f;
};
