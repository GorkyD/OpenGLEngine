#pragma once

#include <cmath>
#include <glad/glad.h>
#include "Ecs/Components/AnimatorComponent.h"
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/WorldTransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Render/ShadowMap.h"
#include "Render/VertexArrayObject.h"
#include "Window/Window.h"

class ShadowMapSystem : public IEcsSystem
{
public:
    ShadowMapSystem(RenderEngine* re, Window* window, ShaderProgramPtr depthShader, ShaderProgramPtr depthSkinnedShader, int resolution = 4096) :
        renderEngine(re), window(window), depthShader(std::move(depthShader)), depthSkinnedShader(std::move(depthSkinnedShader)), shadowMap(resolution)
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
        shadowBias = castingLight->shadowBias;
        shadowAmbientOcclusion = castingLight->shadowAmbientOcclusion;
        shadowNormalBias = castingLight->shadowNormalBias;

        const Rect restoreViewport = window->GetInnerSize();

        shadowMap.BeginWrite();
        renderEngine->SetFaceCulling(CullingType::Both);
        renderEngine->SetWindingOrder(ClockWise);

        auto& meshes = world.GetPool<MeshComponent>();
        auto& shaders = world.GetPool<ShaderComponent>();
        auto& transforms = world.GetPool<TransformComponent>();
        auto& worldTransforms = world.GetPool<WorldTransformComponent>();

        auto modelMatrixOf = [&](Entity entity) -> Matrix4
        {
            if (worldTransforms.Has(entity))
                return worldTransforms.Get(entity).matrix;
            if (transforms.Has(entity))
                return transforms.Get(entity).GetModelMatrix();
            return Matrix4();
        };

        for (auto& [entityId, mesh] : meshes)
        {
            if (!mesh.visible || !shaders.Has(entityId))
                continue;

            const auto& shaderComp = shaders.Get(entityId);
            const bool isSkinned = shaderComp.shaderType == ShaderRenderType::Skinned;
            if (shaderComp.shaderType != ShaderRenderType::Lit && !isSkinned)
                continue;

            const auto& shader = isSkinned ? depthSkinnedShader : depthShader;
            renderEngine->SetShaderProgram(shader);

            const Matrix4 worldMatrix = modelMatrixOf(entityId);
            glUniformMatrix4fv(shader->GetUniformLocation("world"), 1, GL_FALSE, &worldMatrix.matrix[0][0]);
            glUniformMatrix4fv(shader->GetUniformLocation("lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix.matrix[0][0]);

            if (isSkinned && world.HasComponent<AnimatorComponent>(entityId))
            {
                auto& animState = world.GetComponent<AnimatorComponent>(entityId).state;
                if (animState && !animState->boneMatrices.empty())
                {
                    glUniformMatrix4fv(shader->GetUniformLocation("boneMatrices[0]"), static_cast<int>(animState->boneMatrices.size()), GL_FALSE,
                                        &animState->boneMatrices[0].matrix[0][0]);
                }
            }

            renderEngine->SetVertexArrayObject(mesh.vao);
            renderEngine->DrawIndexedTriangles(List, mesh.indexCount);
        }

        shadowMap.EndWrite();
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
    ShaderProgramPtr depthShader;
    ShaderProgramPtr depthSkinnedShader;
    ShadowMap shadowMap;

    Matrix4 lightSpaceMatrix;
    float shadowBias = 0.0025f;
    float shadowAmbientOcclusion = 0.4f;
    float shadowNormalBias = 0.05f;
    bool active = false;
};
