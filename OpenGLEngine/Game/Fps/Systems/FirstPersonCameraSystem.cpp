#include "Systems/FirstPersonCameraSystem.h"
#include <cmath>
#include <imgui.h>
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Input/InputSystem.h"
#include "Math/Vector3.h"
#include "Systems/CharacterControllerSystem.h"

void FirstPersonCameraSystem::Configure(Entity playerCameraEntity)
{
    cameraEntity = playerCameraEntity;
}

void FirstPersonCameraSystem::Update(EcsWorld& world, InputSystem* input, CharacterControllerSystem& controller, float deltaTime)
{
    auto& cam = world.GetComponent<CameraComponent>(cameraEntity);

    const bool uiCapturingInput = ImGui::GetIO().WantCaptureMouse;
    if (!uiCapturingInput)
    {
        yaw += input->GetMouseDeltaX() * mouseSensitivity;
        pitch -= input->GetMouseDeltaY() * mouseSensitivity;
    }

    if (pitch < -maxPitch)
        pitch = -maxPitch;
    if (pitch > maxPitch)
        pitch = maxPitch;

    cam.forward.x = std::cos(pitch) * std::sin(yaw);
    cam.forward.y = std::sin(pitch);
    cam.forward.z = std::cos(pitch) * std::cos(yaw);
    cam.forward = Vector3::Normalize(cam.forward);

    const Vector3 worldUp(0, 1, 0);
    cam.right = Vector3::Normalize(Vector3::Cross(worldUp, cam.forward));
    cam.up = Vector3::Cross(cam.forward, cam.right);

    const Vector3 forwardOnGround = Vector3::Normalize({cam.forward.x, 0.0f, cam.forward.z});
    const Vector3 rightOnGround = Vector3::Normalize({cam.right.x, 0.0f, cam.right.z});

    Vector3 moveDir(0, 0, 0);
    if (input->IsKeyDown(Key::W))
        moveDir += forwardOnGround;
    if (input->IsKeyDown(Key::S))
        moveDir -= forwardOnGround;
    if (input->IsKeyDown(Key::D))
        moveDir += rightOnGround;
    if (input->IsKeyDown(Key::A))
        moveDir -= rightOnGround;

    const bool hasInput = Vector3::Dot(moveDir, moveDir) > 0.0001f;
    if (hasInput)
        moveDir = Vector3::Normalize(moveDir);

    const bool sprinting = input->IsKeyDown(Key::LShift);
    const bool wantsJump = input->IsKeyDown(Key::Space);

    controller.Update(world, moveDir, wantsJump, sprinting, deltaTime);

    isMoving = hasInput && controller.IsGrounded();
}
