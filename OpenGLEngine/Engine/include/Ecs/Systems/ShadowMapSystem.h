#pragma once

#include <algorithm>
#include <cmath>
#include <glad/glad.h>
#include "Ecs/Components/AABB.h"
#include "Ecs/Components/AnimatorComponent.h"
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/WorldTransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Render/ShadowMap.h"
#include "Render/Texture.h"
#include "Render/UniformBuffer.h"
#include "Render/VertexArrayObject.h"
#include "Window/Window.h"

class ShadowMapSystem : public IEcsSystem
{
public:
    ShadowMapSystem(RenderEngine* re, Window* window, UniformBufferPtr uniformBuffer, ShaderProgramPtr depthShader, ShaderProgramPtr depthSkinnedShader, int resolution = 4096) :
        renderEngine(re), window(window), uniformBuffer(std::move(uniformBuffer)), depthShader(std::move(depthShader)), depthSkinnedShader(std::move(depthSkinnedShader)), shadowMap(resolution)
    {
    }

    void Run(EcsWorld& world, float deltaTime) override
    {
        active = false;

        LightComponent* castingLight = nullptr;
        for (auto& [entityId, light] : world.GetPool<LightComponent>())
        {
            if (light.type == LightType::Directional && light.castShadows)
            {
                castingLight = &light;
                break;
            }
        }

        if (!castingLight)
            return;

        Vector3 camPos;
        Vector3 camForward = {0, 0, 1};
        for (auto& [entityId, cam] : world.GetPool<CameraComponent>())
        {
            if (!cam.isActive || !world.HasComponent<TransformComponent>(entityId))
                continue;
            camPos = world.GetComponent<TransformComponent>(entityId).position;
            camForward = cam.forward;
            break;
        }

        const Vector3 toLight = Vector3::Normalize(castingLight->direction);
        const Vector3 target = camPos + camForward * castingLight->shadowFocusDistance;

        Vector3 up = {0, 1, 0};
        if (std::fabs(Vector3::Dot(toLight, up)) > 0.999f)
            up = Vector3(0, 0, 1);

        const Vector3 eye = target + toLight * castingLight->shadowDistance;

        Matrix4 lightView;
        lightView.SetLookAtLeftHanded(eye, target, up);

        Matrix4 lightProjection;
        const float size = castingLight->shadowOrthoSize;
        lightProjection.SetOrthogonalLeftHanded(size * 2.0f, size * 2.0f, 0.1f, castingLight->shadowDistance * 2.0f);

        lightSpaceMatrix = lightView * lightProjection;

        const float halfResolution = static_cast<float>(shadowMap.GetResolution()) * 0.5f;
        const Vector3 originNdc = lightSpaceMatrix.TransformPoint(Vector3(0, 0, 0));
        const float originTexelX = originNdc.x * halfResolution;
        const float originTexelY = originNdc.y * halfResolution;
        lightSpaceMatrix.matrix[3][0] += (std::round(originTexelX) - originTexelX) / halfResolution;
        lightSpaceMatrix.matrix[3][1] += (std::round(originTexelY) - originTexelY) / halfResolution;
        shadowBias = castingLight->shadowBias;
        shadowAmbientOcclusion = castingLight->shadowAmbientOcclusion;
        shadowNormalBias = castingLight->shadowNormalBias;

        const Rect restoreViewport = window->GetInnerSize();

        GLint previousFramebuffer = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousFramebuffer);

        shadowMap.BeginWrite();
        renderEngine->SetFaceCulling(CullingType::Both);
        renderEngine->SetWindingOrder(ClockWise);

        auto& meshes = world.GetPool<MeshComponent>();
        auto& shaders = world.GetPool<ShaderComponent>();
        auto& transforms = world.GetPool<TransformComponent>();
        auto& worldTransforms = world.GetPool<WorldTransformComponent>();
        auto& materials = world.GetPool<MaterialComponent>();
        auto& bounds = world.GetPool<AABB>();

        auto isInsideLightVolume = [&](Entity entity)
        {
            if (!bounds.Has(entity))
                return true;

            const AABB& box = bounds.Get(entity);
            Vector3 ndcMin = {1e30f, 1e30f, 1e30f};
            Vector3 ndcMax = {-1e30f, -1e30f, -1e30f};
            for (int corner = 0; corner < 8; corner++)
            {
                const Vector3 point = {(corner & 1) ? box.max.x : box.min.x, (corner & 2) ? box.max.y : box.min.y, (corner & 4) ? box.max.z : box.min.z};
                const Vector3 ndc = lightSpaceMatrix.TransformPoint(point);
                ndcMin = {(std::min)(ndcMin.x, ndc.x), (std::min)(ndcMin.y, ndc.y), (std::min)(ndcMin.z, ndc.z)};
                ndcMax = {(std::max)(ndcMax.x, ndc.x), (std::max)(ndcMax.y, ndc.y), (std::max)(ndcMax.z, ndc.z)};
            }

            return ndcMax.x >= -1.0f && ndcMin.x <= 1.0f && ndcMax.y >= -1.0f && ndcMin.y <= 1.0f && ndcMax.z >= -1.0f && ndcMin.z <= 1.0f;
        };

        auto modelMatrixOf = [&](Entity entity) -> Matrix4
        {
            if (worldTransforms.Has(entity))
                return worldTransforms.Get(entity).matrix;
            if (transforms.Has(entity))
                return transforms.Get(entity).GetModelMatrix();
            return Matrix4();
        };

        const ShaderProgram* lastShader = nullptr;

        for (auto& [entityId, mesh] : meshes)
        {
            if (!mesh.visible || !mesh.castsShadow || !shaders.Has(entityId))
                continue;

            const auto& shaderComp = shaders.Get(entityId);
            const bool isSkinned = shaderComp.shaderType == ShaderRenderType::Skinned;
            if (shaderComp.shaderType != ShaderRenderType::Lit && !isSkinned)
                continue;
            if (!isInsideLightVolume(entityId))
                continue;

            const auto& shader = isSkinned ? depthSkinnedShader : depthShader;
            renderEngine->SetShaderProgram(shader);

            if (shader.get() != lastShader)
            {
                glUniformMatrix4fv(shader->GetUniformLocation("lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix.matrix[0][0]);
                glUniform1i(shader->GetUniformLocation("diffuseTexture"), 0);
                lastShader = shader.get();
            }

            Matrix4 worldMatrix = modelMatrixOf(entityId);
            uniformBuffer->SetSubData(&worldMatrix, 0, sizeof(Matrix4));

            Vector4 diffuseColor = {1.0f, 1.0f, 1.0f, 1.0f};
            int hasTexture = 0;
            if (materials.Has(entityId))
            {
                auto& mat = materials.Get(entityId);
                diffuseColor = mat.diffuseColor;
                if (mat.diffuseTexture)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mat.diffuseTexture->GetId());
                    hasTexture = 1;
                }
            }
            glUniform1i(shader->GetUniformLocation("hasTexture"), hasTexture);
            glUniform4f(shader->GetUniformLocation("diffuseColor"), diffuseColor.x, diffuseColor.y, diffuseColor.z, diffuseColor.w);

            if (isSkinned && world.HasComponent<AnimatorComponent>(entityId))
            {
                auto& animState = world.GetComponent<AnimatorComponent>(entityId).state;
                if (animState && !animState->boneMatrices.empty())
                {
                    glUniformMatrix4fv(shader->GetUniformLocation("boneMatrices[0]"), static_cast<int>(animState->boneMatrices.size()), GL_FALSE,
                                        &animState->boneMatrices[0].matrix[0][0]);
                }
            }

            const bool doubleSided = materials.Has(entityId) && materials.Get(entityId).doubleSided && glIsEnabled(GL_CULL_FACE) == GL_TRUE;
            if (doubleSided)
                glDisable(GL_CULL_FACE);

            renderEngine->SetVertexArrayObject(mesh.vao);
            renderEngine->DrawIndexedTriangles(List, mesh.indexCount);

            if (doubleSided)
                glEnable(GL_CULL_FACE);
        }

        shadowMap.EndWrite();
        glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
        renderEngine->SetViewPort(restoreViewport);

        active = true;
    }

    bool IsActive() const
    {
        return active;
    }
    const Matrix4& GetLightSpaceMatrix() const
    {
        return lightSpaceMatrix;
    }
    float GetShadowBias() const
    {
        return shadowBias;
    }
    float GetShadowAmbientOcclusion() const
    {
        return shadowAmbientOcclusion;
    }
    float GetShadowNormalBias() const
    {
        return shadowNormalBias;
    }
    unsigned int GetDepthTextureId() const
    {
        return shadowMap.GetDepthTextureId();
    }

private:
    RenderEngine* renderEngine;
    Window* window;
    UniformBufferPtr uniformBuffer;
    ShaderProgramPtr depthShader;
    ShaderProgramPtr depthSkinnedShader;
    ShadowMap shadowMap;

    Matrix4 lightSpaceMatrix;
    float shadowBias = 0.0025f;
    float shadowAmbientOcclusion = 0.4f;
    float shadowNormalBias = 0.05f;
    bool active = false;
};
