#pragma once

#include "Ecs/Core/Entity.h"
#include "Math/Vector3.h"

class EcsWorld;
class InputSystem;

class FlashlightSystem
{
public:
    void Configure(EcsWorld& world);
    void Update(EcsWorld& world, InputSystem* input, const Vector3& position, const Vector3& forward);

    void SetOn(bool value)
    {
        on = value;
    }

    bool IsOn() const
    {
        return on;
    }

    float intensity = 3.0f;
    float range = 25.0f;
    float innerConeAngleDeg = 40.0f;
    float outerConeAngleDeg = 55.0f;

private:
    Entity lightEntity = 0;
    bool on = true;
    bool keyWasPressed = false;
};
