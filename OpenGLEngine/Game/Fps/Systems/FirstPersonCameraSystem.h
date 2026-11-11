#pragma once

#include "Ecs/Core/Entity.h"

class EcsWorld;
class InputSystem;
class CharacterControllerSystem;

class FirstPersonCameraSystem
{
public:
    void Configure(Entity cameraEntity);
    void Update(EcsWorld& world, InputSystem* input, CharacterControllerSystem& controller, float deltaTime);

    bool IsMoving() const
    {
        return isMoving;
    }

    void SetLook(float newYaw, float newPitch)
    {
        yaw = newYaw;
        pitch = newPitch;
    }

    float GetYaw() const
    {
        return yaw;
    }

    float mouseSensitivity = 0.0025f;
    float maxPitch = 1.5533f;

private:
    Entity cameraEntity = 0;
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool isMoving = false;
};
