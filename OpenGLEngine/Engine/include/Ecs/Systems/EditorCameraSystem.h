#pragma once

#include <cmath>
#include <imgui.h>
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Input/InputSystem.h"
#include "Math/Vector3.h"

class EditorCameraSystem : public IEcsSystem
{
public:
    EditorCameraSystem(InputSystem* inputSystem) : inputSystem(inputSystem) {}

    bool enabled = true;
    float moveSpeed = 6.0f;
    float mouseSensitivity = 0.0025f;

    float fovY = 0.7854f;
    float nearPlane = 0.05f;
    float farPlane = 2000.0f;

    void Run(EcsWorld& world, float deltaTime) override
    {
        for (auto& [entity, camera] : world.GetPool<CameraComponent>())
        {
            if (!camera.isActive || !world.HasComponent<TransformComponent>(entity))
                continue;

            if (!enabled)
            {
                RestoreIfNeeded(camera);
                return;
            }

            if (!savedOriginal)
            {
                originalFovY = camera.fovY;
                originalNearPlane = camera.nearPlane;
                originalFarPlane = camera.farPlane;
                savedOriginal = true;
            }

            camera.fovY = fovY;
            camera.nearPlane = nearPlane;
            camera.farPlane = farPlane;

            UpdateCamera(world.GetComponent<TransformComponent>(entity), camera, deltaTime);
            return;
        }
    }

private:
    void RestoreIfNeeded(CameraComponent& camera)
    {
        if (!savedOriginal)
            return;

        camera.fovY = originalFovY;
        camera.nearPlane = originalNearPlane;
        camera.farPlane = originalFarPlane;
        savedOriginal = false;
    }

    void UpdateCamera(TransformComponent& transform, CameraComponent& camera, float deltaTime)
    {
        const bool uiCaptured = ImGui::GetIO().WantCaptureMouse;
        const bool looking = inputSystem->IsLeftMouseDown() && !uiCaptured;

        if (looking)
        {
            camera.yaw += inputSystem->GetMouseDeltaX() * mouseSensitivity;
            camera.pitch -= inputSystem->GetMouseDeltaY() * mouseSensitivity;

            constexpr float maxPitch = 1.5533f;
            if (camera.pitch < -maxPitch)
                camera.pitch = -maxPitch;
            if (camera.pitch > maxPitch)
                camera.pitch = maxPitch;
        }

        camera.forward.x = std::cos(camera.pitch) * std::sin(camera.yaw);
        camera.forward.y = std::sin(camera.pitch);
        camera.forward.z = std::cos(camera.pitch) * std::cos(camera.yaw);
        camera.forward = Vector3::Normalize(camera.forward);

        camera.right = Vector3::Normalize(Vector3::Cross({0.0f, 1.0f, 0.0f}, camera.forward));
        camera.up = Vector3::Cross(camera.forward, camera.right);

        if (ImGui::GetIO().WantCaptureKeyboard)
            return;

        Vector3 moveDir = {0, 0, 0};
        if (inputSystem->IsKeyDown(Key::W))
            moveDir += camera.forward;
        if (inputSystem->IsKeyDown(Key::S))
            moveDir -= camera.forward;
        if (inputSystem->IsKeyDown(Key::D))
            moveDir += camera.right;
        if (inputSystem->IsKeyDown(Key::A))
            moveDir -= camera.right;
        if (inputSystem->IsKeyDown(Key::E))
            moveDir += Vector3(0.0f, 1.0f, 0.0f);
        if (inputSystem->IsKeyDown(Key::Q))
            moveDir -= Vector3(0.0f, 1.0f, 0.0f);

        if (moveDir.x == 0.0f && moveDir.y == 0.0f && moveDir.z == 0.0f)
            return;

        float speed = moveSpeed;
        if (inputSystem->IsKeyDown(Key::LShift))
            speed *= 3.0f;

        transform.position += Vector3::Normalize(moveDir) * (speed * deltaTime);
    }

    InputSystem* inputSystem;

    bool savedOriginal = false;
    float originalFovY = 0.7854f;
    float originalNearPlane = 0.01f;
    float originalFarPlane = 100.0f;
};
