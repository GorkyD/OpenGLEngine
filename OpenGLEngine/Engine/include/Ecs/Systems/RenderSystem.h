#pragma once

#include "Ecs/Components/AnimatorComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/AmbientLightComponent.h"
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/FogComponent.h"
#include "Ecs/Components/OutlineComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/VisibilityComponent.h"
#include "Ecs/Components/WorldTransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Render/UniformBuffer.h"
#include "Render/Texture.h"
#include "Render/InstanceData.h"
#include "Render/VertexArrayObject.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include <glad/glad.h>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class RenderSystem : public IEcsSystem
{
    struct BatchKey
    {
        VertexArrayObject* vao = nullptr;
        unsigned int diffuseTextureId = 0;

        bool operator==(const BatchKey& other) const
        {
            return vao == other.vao && diffuseTextureId == other.diffuseTextureId;
        }
    };

    struct BatchKeyHash
    {
        size_t operator()(const BatchKey& key) const
        {
            return std::hash<const void*>()(key.vao) ^ (std::hash<unsigned int>()(key.diffuseTextureId) << 1);
        }
    };

public:
    static constexpr int MaxLights = 32;

    RenderSystem(RenderEngine* re, const UniformBufferPtr& ub, const ShaderProgramPtr& outlineShader, const ShaderProgramPtr& litInstancedShader) :
        renderEngine(re), uniformBuffer(ub), outlineShader(outlineShader), litInstancedShader(litInstancedShader)
    {
    }

    void Run(EcsWorld& world, float deltaTime) override
    {
        elapsedTime += deltaTime;

        lastUploadedPose = nullptr;
        lastPoseShader = nullptr;

        auto& meshes = world.GetPool<MeshComponent>();
        auto& shaders = world.GetPool<ShaderComponent>();
        auto& materials = world.GetPool<MaterialComponent>();
        auto& transforms = world.GetPool<TransformComponent>();
        auto& worldTransforms = world.GetPool<WorldTransformComponent>();

        auto& visibilities = world.GetPool<VisibilityComponent>();

        auto isVisible = [&](Entity entity) { return !visibilities.Has(entity) || visibilities.Get(entity).visibleInFrustum; };

        auto modelMatrixOf = [&](Entity entity) -> Matrix4
        {
            if (worldTransforms.Has(entity))
                return worldTransforms.Get(entity).matrix;
            if (transforms.Has(entity))
                return transforms.Get(entity).GetModelMatrix();
            return Matrix4();
        };

        AmbientLightComponent ambient;
        for (auto& [entityId, ambientLightComponent] : world.GetPool<AmbientLightComponent>())
        {
            ambient = ambientLightComponent;
            break;
        }

        FogComponent fog;
        for (auto& [entityId, fogComponent] : world.GetPool<FogComponent>())
        {
            fog = fogComponent;
            break;
        }

        Vector3 viewPos;
        for (auto& [entityId, cameraComponent] : world.GetPool<CameraComponent>())
        {
            if (!cameraComponent.isActive || !transforms.Has(entityId))
                continue;
            viewPos = transforms.Get(entityId).position;
            break;
        }

        int numLights = 0;
        int lightType[MaxLights] = {};
        float lightColor[MaxLights * 3] = {};
        float lightIntensity[MaxLights] = {};
        float lightDirection[MaxLights * 3] = {};
        float lightPosition[MaxLights * 3] = {};
        float lightRange[MaxLights] = {};

        for (auto& [entityId, lightComponent] : world.GetPool<LightComponent>())
        {
            if (numLights >= MaxLights)
                break;

            const auto& light = lightComponent;
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

        auto drawEntity = [&](Entity entity, MeshComponent& mesh)
        {
            if (!shaders.Has(entity))
                return;

            if (!mesh.visible || !isVisible(entity))
                return;

            Matrix4 worldMatrix = modelMatrixOf(entity);

            uniformBuffer->SetSubData(&worldMatrix, 0, sizeof(Matrix4));

            auto& shaderComp = shaders.Get(entity);
            renderEngine->SetShaderProgram(shaderComp.shader);

            Vector4 color = {1, 1, 1, 1};
            Vector3 emissiveColor = {0, 0, 0};
            float emissiveIntensity = 0.0f;
            float roughnessScalar = 1.0f;
            float metallicScalar = 0.0f;
            int hasTexture = 0;
            int hasNormalMap = 0;
            int hasRoughnessMap = 0;
            int hasMetallicMap = 0;
            int hasAoMap = 0;
            for (int unit = 0; unit < 5; unit++)
            {
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            if (materials.Has(entity))
            {
                auto& mat = materials.Get(entity);
                color = mat.diffuseColor;
                emissiveColor = mat.emissiveColor;
                emissiveIntensity = mat.emissiveIntensity;
                roughnessScalar = mat.roughness;
                metallicScalar = mat.metallic;
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
                if (mat.roughnessTexture)
                {
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, mat.roughnessTexture->GetId());
                    hasRoughnessMap = 1;
                }
                if (mat.metallicTexture)
                {
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, mat.metallicTexture->GetId());
                    hasMetallicMap = 1;
                }
                if (mat.aoTexture)
                {
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_2D, mat.aoTexture->GetId());
                    hasAoMap = 1;
                }
            }

            auto* shader = shaderComp.shader.get();

            glUniform4f(shader->GetUniformLocation("diffuseColor"), color.x, color.y, color.z, color.w);
            glUniform1i(shader->GetUniformLocation("hasTexture"), hasTexture);

            if (shaderComp.shaderType == ShaderRenderType::Skinned && world.HasComponent<AnimatorComponent>(entity))
            {
                auto& animState = world.GetComponent<AnimatorComponent>(entity).state;
                if (animState && !animState->boneMatrices.empty() && (animState.get() != lastUploadedPose || shader != lastPoseShader))
                {
                    glUniformMatrix4fv(shader->GetUniformLocation("boneMatrices[0]"), static_cast<int>(animState->boneMatrices.size()), GL_FALSE,
                                        &animState->boneMatrices[0].matrix[0][0]);

                    lastUploadedPose = animState.get();
                    lastPoseShader = shader;
                }
            }

            if (shaderComp.shaderType == ShaderRenderType::Lit || shaderComp.shaderType == ShaderRenderType::Skinned)
            {
                glUniform1i(shader->GetUniformLocation("normalTexture"), 1);
                glUniform1i(shader->GetUniformLocation("hasNormalMap"), hasNormalMap);
                glUniform1i(shader->GetUniformLocation("roughnessTexture"), 2);
                glUniform1i(shader->GetUniformLocation("hasRoughnessMap"), hasRoughnessMap);
                glUniform1i(shader->GetUniformLocation("metallicTexture"), 3);
                glUniform1i(shader->GetUniformLocation("hasMetallicMap"), hasMetallicMap);
                glUniform1i(shader->GetUniformLocation("aoTexture"), 4);
                glUniform1i(shader->GetUniformLocation("hasAoMap"), hasAoMap);
                glUniform1f(shader->GetUniformLocation("roughnessScalar"), roughnessScalar);
                glUniform1f(shader->GetUniformLocation("metallicScalar"), metallicScalar);
                glUniform3f(shader->GetUniformLocation("viewPos"), viewPos.x, viewPos.y, viewPos.z);
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
                glUniformMatrix3fv(shader->GetUniformLocation("normalMatrix"), 1, GL_FALSE, normalMatrix);

                glUniform3f(shader->GetUniformLocation("emissiveColor"), emissiveColor.x, emissiveColor.y, emissiveColor.z);
                glUniform1f(shader->GetUniformLocation("emissiveIntensity"), emissiveIntensity);

                glUniform3f(shader->GetUniformLocation("fogColor"), fog.color.x, fog.color.y, fog.color.z);
                glUniform1f(shader->GetUniformLocation("fogStart"), fog.start);
                glUniform1f(shader->GetUniformLocation("fogEnd"), fog.end);
            }

            const bool isFire = shaderComp.shaderType == ShaderRenderType::Fire;
            const bool isShadow = shaderComp.shaderType == ShaderRenderType::Shadow;
            if (isFire)
                glUniform1f(shader->GetUniformLocation("time"), elapsedTime);

            if (isFire || isShadow)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, isFire ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);
                glDisable(GL_CULL_FACE);
            }

            renderEngine->SetVertexArrayObject(mesh.vao);
            renderEngine->DrawIndexedTriangles(List, mesh.indexCount);

            if (isFire || isShadow)
            {
                glEnable(GL_CULL_FACE);
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
            }
        };

        auto typeOf = [&](Entity entity) -> ShaderRenderType { return shaders.Has(entity) ? shaders.Get(entity).shaderType : ShaderRenderType::Unlit; };

        // Clear previous frame batches completely to avoid unbounded growth of the map
        batchGroups.clear();

        for (auto& [entityId, meshComponent] : meshes)
        {
            if (!meshComponent.visible || !isVisible(entityId))
                continue;
            if (typeOf(entityId) != ShaderRenderType::Lit)
                continue;
            if (world.HasComponent<OutlineComponent>(entityId))
                continue;

            unsigned int diffuseTextureId = 0;
            if (materials.Has(entityId))
            {
                auto& mat = materials.Get(entityId);
                if (mat.normalTexture || mat.roughnessTexture || mat.metallicTexture || mat.aoTexture)
                    continue;
                if (mat.diffuseTexture)
                    diffuseTextureId = mat.diffuseTexture->GetId();
            }

            batchGroups[{meshComponent.vao.get(), diffuseTextureId}].push_back(entityId);
        }

        batchedEntities.clear();

        for (auto& [key, entityList] : batchGroups)
        {
            if (entityList.size() < 2)
                continue;

            VertexArrayObject* vaoPtr = key.vao;

            instances.clear();
            instances.reserve(entityList.size());
            for (auto entity : entityList)
            {
                InstanceData instance = {};

                const Matrix4 worldMatrix = modelMatrixOf(entity);
                std::memcpy(instance.world, worldMatrix.matrix, sizeof(float) * 16);

                if (materials.Has(entity))
                {
                    auto& mat = materials.Get(entity);
                    instance.diffuseColor[0] = mat.diffuseColor.x;
                    instance.diffuseColor[1] = mat.diffuseColor.y;
                    instance.diffuseColor[2] = mat.diffuseColor.z;
                    instance.diffuseColor[3] = mat.diffuseColor.w;
                    instance.emissiveColorIntensity[0] = mat.emissiveColor.x;
                    instance.emissiveColorIntensity[1] = mat.emissiveColor.y;
                    instance.emissiveColorIntensity[2] = mat.emissiveColor.z;
                    instance.emissiveColorIntensity[3] = mat.emissiveIntensity;
                    instance.materialParams[0] = mat.roughness;
                    instance.materialParams[1] = mat.metallic;
                }
                else
                {
                    instance.diffuseColor[0] = 1;
                    instance.diffuseColor[1] = 1;
                    instance.diffuseColor[2] = 1;
                    instance.diffuseColor[3] = 1;
                    instance.emissiveColorIntensity[0] = 0;
                    instance.emissiveColorIntensity[1] = 0;
                    instance.emissiveColorIntensity[2] = 0;
                    instance.emissiveColorIntensity[3] = 0;
                    instance.materialParams[0] = 1.0f;
                    instance.materialParams[1] = 0.0f;
                }

                instances.push_back(instance);
            }

            renderEngine->SetShaderProgram(litInstancedShader);

            glUniform3f(litInstancedShader->GetUniformLocation("viewPos"), viewPos.x, viewPos.y, viewPos.z);
            glUniform3f(litInstancedShader->GetUniformLocation("ambientColor"), ambient.color.x, ambient.color.y, ambient.color.z);
            glUniform1f(litInstancedShader->GetUniformLocation("ambientIntensity"), ambient.intensity);

            glUniform1i(litInstancedShader->GetUniformLocation("numLights"), numLights);
            if (numLights > 0)
            {
                glUniform1iv(litInstancedShader->GetUniformLocation("lightType"), numLights, lightType);
                glUniform3fv(litInstancedShader->GetUniformLocation("lightColor"), numLights, lightColor);
                glUniform1fv(litInstancedShader->GetUniformLocation("lightIntensity"), numLights, lightIntensity);
                glUniform3fv(litInstancedShader->GetUniformLocation("lightDirection"), numLights, lightDirection);
                glUniform3fv(litInstancedShader->GetUniformLocation("lightPosition"), numLights, lightPosition);
                glUniform1fv(litInstancedShader->GetUniformLocation("lightRange"), numLights, lightRange);
            }

            glUniform3f(litInstancedShader->GetUniformLocation("fogColor"), fog.color.x, fog.color.y, fog.color.z);
            glUniform1f(litInstancedShader->GetUniformLocation("fogStart"), fog.start);
            glUniform1f(litInstancedShader->GetUniformLocation("fogEnd"), fog.end);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, key.diffuseTextureId);
            glUniform1i(litInstancedShader->GetUniformLocation("diffuseTexture"), 0);
            glUniform1i(litInstancedShader->GetUniformLocation("hasTexture"), key.diffuseTextureId != 0 ? 1 : 0);

            vaoPtr->UploadInstanceData(instances.data(), static_cast<unsigned int>(instances.size()));
            renderEngine->SetVertexArrayObject(meshes.Get(entityList[0]).vao);
            renderEngine->DrawIndexedTrianglesInstanced(List, meshes.Get(entityList[0]).indexCount, static_cast<unsigned int>(instances.size()));

            for (auto entity : entityList)
                batchedEntities.insert(entity);
        }

        for (auto& [entityId, meshComponent] : meshes)
        {
            const auto type = typeOf(entityId);
            if (type == ShaderRenderType::Fire || type == ShaderRenderType::Shadow)
                continue;
            if (batchedEntities.count(entityId))
                continue;
            drawEntity(entityId, meshComponent);
        }

        for (auto& [entityId, meshComponent] : meshes)
        {
            if (typeOf(entityId) == ShaderRenderType::Fire)
                drawEntity(entityId, meshComponent);
        }

        for (auto& [entityId, meshComponent] : meshes)
        {
            if (typeOf(entityId) == ShaderRenderType::Shadow)
                drawEntity(entityId, meshComponent);
        }

        if (outlineShader)
        {
            for (auto& [entityId, outlineComponent] : world.GetPool<OutlineComponent>())
            {
                const Entity entity = entityId;
                if (!meshes.Has(entity) || !transforms.Has(entity) || !meshes.Get(entity).visible)
                    continue;

                Matrix4 worldMatrix = modelMatrixOf(entity);
                uniformBuffer->SetSubData(&worldMatrix, 0, sizeof(Matrix4));

                renderEngine->SetShaderProgram(outlineShader);
                glUniform1f(outlineShader->GetUniformLocation("outlineWidth"), outlineComponent.width);
                glUniform3f(outlineShader->GetUniformLocation("outlineColor"), outlineComponent.color[0], outlineComponent.color[1], outlineComponent.color[2]);

                glCullFace(GL_FRONT);
                renderEngine->SetVertexArrayObject(meshes.Get(entity).vao);
                renderEngine->DrawIndexedTriangles(List, meshes.Get(entity).indexCount);
                glCullFace(GL_BACK);
            }
        }
    }

private:
    RenderEngine* renderEngine;
    UniformBufferPtr uniformBuffer;
    ShaderProgramPtr outlineShader;
    ShaderProgramPtr litInstancedShader;

    const AnimatorState* lastUploadedPose = nullptr;
    const ShaderProgram* lastPoseShader = nullptr;

    std::unordered_map<BatchKey, std::vector<Entity>, BatchKeyHash> batchGroups;
    std::unordered_set<Entity> batchedEntities;
    std::vector<InstanceData> instances;

    float elapsedTime = 0.0f;
};
