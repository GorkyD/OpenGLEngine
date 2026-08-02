#pragma once

#include <imgui.h>
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Ecs/Systems/EditorCameraSystem.h"
#include "Engine/Engine.h"

class CameraPanel
{
public:
    void Draw(Engine& engine, EcsWorld& world)
    {
        auto* editorCamera = engine.GetEditorCameraSystem();
        if (!editorCamera)
            return;

        constexpr float rad2deg = 180.0f / 3.14159265f;
        constexpr float deg2rad = 3.14159265f / 180.0f;

        float fovDegrees = editorCamera->fovY * rad2deg;
        if (ImGui::SliderFloat("FOV (deg)", &fovDegrees, 20.0f, 120.0f))
            editorCamera->fovY = fovDegrees * deg2rad;

        ImGui::DragFloat("Near Plane", &editorCamera->nearPlane, 0.01f, 0.001f, 10.0f);
        ImGui::DragFloat("Far Plane", &editorCamera->farPlane, 10.0f, 10.0f, 100000.0f);
        ImGui::DragFloat("Fly Speed", &editorCamera->moveSpeed, 0.5f, 0.1f, 500.0f);

        for (auto& [entity, camera] : world.GetPool<CameraComponent>())
        {
            if (!camera.isActive || !world.HasComponent<TransformComponent>(entity))
                continue;

            ImGui::DragFloat3("Camera Position", &world.GetComponent<TransformComponent>(entity).position.x, 0.1f);
            break;
        }
    }
};
