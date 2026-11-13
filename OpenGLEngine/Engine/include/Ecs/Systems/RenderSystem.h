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
#include "Ecs/Components/PostProcessComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/VisibilityComponent.h"
#include "Ecs/Components/WorldTransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Ecs/Systems/ShadowMapSystem.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Render/UniformBuffer.h"
#include "Render/UniformData.h"
#include "Render/Texture.h"
#include "Render/InstanceData.h"
#include "Render/VertexArrayObject.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include <glad/glad.h>
#include <algorithm>
#include <cmath>
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
        bool doubleSided = false;

        bool operator==(const BatchKey& other) const
        {
            return vao == other.vao && diffuseTextureId == other.diffuseTextureId && doubleSided == other.doubleSided;
        }
    };

    struct BatchKeyHash
    {
        size_t operator()(const BatchKey& key) const
        {
            return std::hash<const void*>()(key.vao) ^ (std::hash<unsigned int>()(key.diffuseTextureId) << 1) ^ (key.doubleSided ? 0x9e3779b9u : 0u);
        }
    };

    struct TransparentDraw
    {
        Entity entity;
        float distanceSq;
    };

    static constexpr int ShadowMapTextureUnit = 5;
    bool overlayPassOnly = false;
    bool overlayDeferred = false;

public:
    static constexpr int MaxLights = MaxFrameLights;

    RenderSystem(RenderEngine* re, const UniformBufferPtr& ub, const ShaderProgramPtr& outlineShader, const ShaderProgramPtr& litInstancedShader, ShadowMapSystem* shadowMapSystem) :
        renderEngine(re), uniformBuffer(ub), outlineShader(outlineShader), litInstancedShader(litInstancedShader), shadowMapSystem(shadowMapSystem)
    {
        frameUniformBuffer = renderEngine->CreateUniformBuffer({sizeof(FrameUniformData)});
    }

    bool HasDeferredOverlay() const
    {
        return overlayDeferred;
    }

    void RunOverlay(EcsWorld& world)
    {
        overlayPassOnly = true;
        Run(world, 0.0f);
        overlayPassOnly = false;
        overlayDeferred = false;
    }

    void Run(EcsWorld& world, float deltaTime) override
    {
        elapsedTime += deltaTime;

        bool deferOverlay = false;
        for (auto& pair : world.GetPool<PostProcessComponent>())
        {
            deferOverlay = pair.second.overlayAfterPostProcess;
            break;
        }
        if (!overlayPassOnly)
            overlayDeferred = deferOverlay;

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

        auto typeOf = [&](Entity entity) -> ShaderRenderType { return shaders.Has(entity) ? shaders.Get(entity).shaderType : ShaderRenderType::Unlit; };

        auto isTransparent = [](ShaderRenderType type) { return type == ShaderRenderType::Fire || type == ShaderRenderType::Shadow; };

        const Vector3 viewPos = UploadFrameData(world);

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
            bool doubleSided = false;

            auto bindTexture = [](int unit, const TexturePtr& texture, int& hasFlag)
            {
                if (!texture)
                    return;
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, texture->GetId());
                hasFlag = 1;
            };

            if (materials.Has(entity))
            {
                auto& mat = materials.Get(entity);
                color = mat.diffuseColor;
                emissiveColor = mat.emissiveColor;
                emissiveIntensity = mat.emissiveIntensity;
                roughnessScalar = mat.roughness;
                metallicScalar = mat.metallic;
                doubleSided = mat.doubleSided;
                bindTexture(0, mat.diffuseTexture, hasTexture);
                bindTexture(1, mat.normalTexture, hasNormalMap);
                bindTexture(2, mat.roughnessTexture, hasRoughnessMap);
                bindTexture(3, mat.metallicTexture, hasMetallicMap);
                bindTexture(4, mat.aoTexture, hasAoMap);
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
                glUniform1i(shader->GetUniformLocation("shadowMap"), ShadowMapTextureUnit);
                glUniform1f(shader->GetUniformLocation("roughnessScalar"), roughnessScalar);
                glUniform1f(shader->GetUniformLocation("metallicScalar"), metallicScalar);

                glUniform1i(shader->GetUniformLocation("bypassSpotCone"), mesh.renderQueue == RenderQueue::Overlay ? 1 : 0);
                glUniform2f(shader->GetUniformLocation("psxSnapGrid"), psxSnapGrid[0], psxSnapGrid[1]);

                float normalMatrix[9];
                worldMatrix.GetNormalMatrix(normalMatrix);
                glUniformMatrix3fv(shader->GetUniformLocation("normalMatrix"), 1, GL_FALSE, normalMatrix);

                glUniform3f(shader->GetUniformLocation("emissiveColor"), emissiveColor.x, emissiveColor.y, emissiveColor.z);
                glUniform1f(shader->GetUniformLocation("emissiveIntensity"), emissiveIntensity);
            }

            const bool isFire = shaderComp.shaderType == ShaderRenderType::Fire;
            const bool blended = isTransparent(shaderComp.shaderType);
            if (isFire)
                glUniform1f(shader->GetUniformLocation("time"), elapsedTime);

            const bool cullWasEnabled = glIsEnabled(GL_CULL_FACE) == GL_TRUE;
            const bool cullDisabled = (doubleSided || blended) && cullWasEnabled;
            if (cullDisabled)
                glDisable(GL_CULL_FACE);

            if (blended)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, isFire ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);
            }

            renderEngine->SetVertexArrayObject(mesh.vao);
            renderEngine->DrawIndexedTriangles(List, mesh.indexCount);

            if (cullDisabled)
                glEnable(GL_CULL_FACE);

            if (blended)
            {
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
            }
        };

        auto drawTransparent = [&](RenderQueue queue)
        {
            transparentDraws.clear();
            for (auto& [entityId, meshComponent] : meshes)
            {
                if (meshComponent.renderQueue != queue || !isTransparent(typeOf(entityId)))
                    continue;
                if (!meshComponent.visible || !isVisible(entityId))
                    continue;

                const Matrix4 worldMatrix = modelMatrixOf(entityId);
                const Vector3 position = {worldMatrix.matrix[3][0], worldMatrix.matrix[3][1], worldMatrix.matrix[3][2]};
                const Vector3 toCamera = position - viewPos;
                transparentDraws.push_back({entityId, Vector3::Dot(toCamera, toCamera)});
            }

            std::sort(transparentDraws.begin(), transparentDraws.end(), [](const TransparentDraw& a, const TransparentDraw& b) { return a.distanceSq > b.distanceSq; });

            for (const auto& draw : transparentDraws)
                drawEntity(draw.entity, meshes.Get(draw.entity));
        };

        batchGroups.clear();

        for (auto& [entityId, meshComponent] : meshes)
        {
            if (overlayPassOnly)
                break;
            if (!meshComponent.visible || !isVisible(entityId))
                continue;
            if (meshComponent.renderQueue != RenderQueue::Default)
                continue;
            if (typeOf(entityId) != ShaderRenderType::Lit)
                continue;
            if (world.HasComponent<OutlineComponent>(entityId))
                continue;

            unsigned int diffuseTextureId = 0;
            bool doubleSided = false;
            if (materials.Has(entityId))
            {
                auto& mat = materials.Get(entityId);
                if (mat.normalTexture || mat.roughnessTexture || mat.metallicTexture || mat.aoTexture)
                    continue;
                if (mat.diffuseTexture)
                    diffuseTextureId = mat.diffuseTexture->GetId();
                doubleSided = mat.doubleSided;
            }

            batchGroups[{meshComponent.vao.get(), diffuseTextureId, doubleSided}].push_back(entityId);
        }

        batchedEntities.clear();

        for (auto& [key, entityList] : batchGroups)
        {
            if (entityList.size() < 2)
                continue;

            instances.clear();
            instances.reserve(entityList.size());
            for (auto entity : entityList)
            {
                InstanceData instance = {};

                const Matrix4 worldMatrix = modelMatrixOf(entity);
                std::memcpy(instance.world, worldMatrix.matrix, sizeof(float) * 16);

                instance.diffuseColor[0] = 1.0f;
                instance.diffuseColor[1] = 1.0f;
                instance.diffuseColor[2] = 1.0f;
                instance.diffuseColor[3] = 1.0f;
                instance.materialParams[0] = 1.0f;

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

                instances.push_back(instance);
            }

            renderEngine->SetShaderProgram(litInstancedShader);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, key.diffuseTextureId);
            glUniform1i(litInstancedShader->GetUniformLocation("diffuseTexture"), 0);
            glUniform1i(litInstancedShader->GetUniformLocation("hasTexture"), key.diffuseTextureId != 0 ? 1 : 0);
            glUniform1i(litInstancedShader->GetUniformLocation("shadowMap"), ShadowMapTextureUnit);
            glUniform2f(litInstancedShader->GetUniformLocation("psxSnapGrid"), psxSnapGrid[0], psxSnapGrid[1]);

            const auto& firstMesh = meshes.Get(entityList[0]);
            key.vao->UploadInstanceData(instances.data(), static_cast<unsigned int>(instances.size()));
            renderEngine->SetVertexArrayObject(firstMesh.vao);
            const bool cullBatchDisabled = key.doubleSided && glIsEnabled(GL_CULL_FACE) == GL_TRUE;
            if (cullBatchDisabled)
                glDisable(GL_CULL_FACE);
            renderEngine->DrawIndexedTrianglesInstanced(List, firstMesh.indexCount, static_cast<unsigned int>(instances.size()));
            if (cullBatchDisabled)
                glEnable(GL_CULL_FACE);

            for (auto entity : entityList)
                batchedEntities.insert(entity);
        }

        for (auto& [entityId, meshComponent] : meshes)
        {
            if (overlayPassOnly)
                break;
            if (meshComponent.renderQueue != RenderQueue::Default)
                continue;
            if (isTransparent(typeOf(entityId)))
                continue;
            if (batchedEntities.count(entityId))
                continue;
            drawEntity(entityId, meshComponent);
        }

        if (!overlayPassOnly)
            drawTransparent(RenderQueue::Default);

        if (outlineShader && !overlayPassOnly)
        {
            for (auto& [entityId, outlineComponent] : world.GetPool<OutlineComponent>())
            {
                const Entity entity = entityId;
                if (!meshes.Has(entity))
                    continue;

                auto& mesh = meshes.Get(entity);
                if (!mesh.visible || !isVisible(entity))
                    continue;

                Matrix4 worldMatrix = modelMatrixOf(entity);
                uniformBuffer->SetSubData(&worldMatrix, 0, sizeof(Matrix4));

                renderEngine->SetShaderProgram(outlineShader);
                glUniform1f(outlineShader->GetUniformLocation("outlineWidth"), outlineComponent.width);
                glUniform3f(outlineShader->GetUniformLocation("outlineColor"), outlineComponent.color[0], outlineComponent.color[1], outlineComponent.color[2]);

                glCullFace(GL_FRONT);
                renderEngine->SetVertexArrayObject(mesh.vao);
                renderEngine->DrawIndexedTriangles(List, mesh.indexCount);
                glCullFace(GL_BACK);
            }
        }

        bool hasOverlay = false;
        const bool overlayNow = overlayPassOnly || !overlayDeferred;
        for (auto& [entityId, meshComponent] : meshes)
        {
            if (meshComponent.renderQueue == RenderQueue::Overlay && meshComponent.visible && isVisible(entityId))
            {
                hasOverlay = true;
                break;
            }
        }

        if (hasOverlay && overlayNow)
        {
            glClear(GL_DEPTH_BUFFER_BIT);

            for (auto& [entityId, meshComponent] : meshes)
            {
                if (meshComponent.renderQueue != RenderQueue::Overlay)
                    continue;
                if (isTransparent(typeOf(entityId)))
                    continue;
                drawEntity(entityId, meshComponent);
            }

            drawTransparent(RenderQueue::Overlay);
        }

        glActiveTexture(GL_TEXTURE0);
    }

private:
    Vector3 UploadFrameData(EcsWorld& world)
    {
        auto& transforms = world.GetPool<TransformComponent>();

        FrameUniformData data = {};

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

        const bool shadowsAvailable = shadowMapSystem && shadowMapSystem->IsActive();
        int shadowLightIndex = -1;
        int numLights = 0;

        for (auto& [entityId, light] : world.GetPool<LightComponent>())
        {
            if (numLights >= MaxLights)
                break;

            data.lightColorIntensity[numLights][0] = light.color.x;
            data.lightColorIntensity[numLights][1] = light.color.y;
            data.lightColorIntensity[numLights][2] = light.color.z;
            data.lightColorIntensity[numLights][3] = light.intensity;
            data.lightPositionRange[numLights][0] = light.position.x;
            data.lightPositionRange[numLights][1] = light.position.y;
            data.lightPositionRange[numLights][2] = light.position.z;
            data.lightPositionRange[numLights][3] = light.range;
            data.lightDirectionType[numLights][0] = light.direction.x;
            data.lightDirectionType[numLights][1] = light.direction.y;
            data.lightDirectionType[numLights][2] = light.direction.z;
            data.lightDirectionType[numLights][3] = static_cast<float>(light.type);
            data.lightCone[numLights][0] = std::cos(light.innerConeAngleDeg * 0.0174532925f);
            data.lightCone[numLights][1] = std::cos(light.outerConeAngleDeg * 0.0174532925f);

            if (shadowsAvailable && shadowLightIndex < 0 && light.type == LightType::Directional && light.castShadows)
                shadowLightIndex = numLights;

            numLights++;
        }

        data.lightSpaceMatrix = shadowsAvailable ? shadowMapSystem->GetLightSpaceMatrix() : Matrix4();
        data.viewPos[0] = viewPos.x;
        data.viewPos[1] = viewPos.y;
        data.viewPos[2] = viewPos.z;
        data.ambientColor[0] = ambient.color.x;
        data.ambientColor[1] = ambient.color.y;
        data.ambientColor[2] = ambient.color.z;
        data.ambientIntensity = ambient.intensity;
        data.fogColor[0] = fog.color.x;
        data.fogColor[1] = fog.color.y;
        data.fogColor[2] = fog.color.z;
        data.fogStart = fog.start;
        data.fogEnd = fog.end;
        data.fogGlowExponent = fog.glowExponent;
        data.fogGlowStrength = fog.glowStrength;
        data.fogRadialEnabled = fog.radialEnabled ? 1 : 0;
        data.fogRadialOrigin[0] = fog.radialOrigin.x;
        data.fogRadialOrigin[1] = fog.radialOrigin.y;
        data.fogRadialOrigin[2] = fog.radialOrigin.z;
        data.fogRadialStart = fog.radialStart;
        data.fogRadialEnd = fog.radialEnd;
        data.numLights = numLights;
        data.shadowLightIndex = shadowLightIndex;
        data.shadowBias = shadowsAvailable ? shadowMapSystem->GetShadowBias() : 0.0025f;
        data.shadowAmbientOcclusion = shadowsAvailable ? shadowMapSystem->GetShadowAmbientOcclusion() : 1.0f;
        data.shadowNormalBias = shadowsAvailable ? shadowMapSystem->GetShadowNormalBias() : 0.05f;

        psxSnapGrid[0] = 0.0f;
        psxSnapGrid[1] = 0.0f;
        for (auto& [entityId, post] : world.GetPool<PostProcessComponent>())
        {
            if (post.psxEnabled && post.psxVertexSnap)
            {
                GLint viewport[4] = {};
                glGetIntegerv(GL_VIEWPORT, viewport);
                const float aspect = viewport[3] > 0 ? static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]) : 1.0f;
                psxSnapGrid[0] = post.psxVertexSnapHeight * aspect;
                psxSnapGrid[1] = post.psxVertexSnapHeight;
            }
            break;
        }

        frameUniformBuffer->SetData(&data);
        renderEngine->SetUniformBuffer(frameUniformBuffer, FrameUniformBufferSlot);

        glActiveTexture(GL_TEXTURE0 + ShadowMapTextureUnit);
        glBindTexture(GL_TEXTURE_2D, shadowMapSystem ? shadowMapSystem->GetDepthTextureId() : 0);

        return viewPos;
    }

    RenderEngine* renderEngine;
    UniformBufferPtr uniformBuffer;
    UniformBufferPtr frameUniformBuffer;
    ShaderProgramPtr outlineShader;
    ShaderProgramPtr litInstancedShader;
    ShadowMapSystem* shadowMapSystem;

    const AnimatorState* lastUploadedPose = nullptr;
    const ShaderProgram* lastPoseShader = nullptr;

    std::unordered_map<BatchKey, std::vector<Entity>, BatchKeyHash> batchGroups;
    std::unordered_set<Entity> batchedEntities;
    std::vector<InstanceData> instances;
    std::vector<TransparentDraw> transparentDraws;

    float elapsedTime = 0.0f;
    float psxSnapGrid[2] = {0.0f, 0.0f};
};
