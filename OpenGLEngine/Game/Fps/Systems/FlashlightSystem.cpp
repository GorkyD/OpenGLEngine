#include "Systems/FlashlightSystem.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Input/InputSystem.h"

void FlashlightSystem::Configure(EcsWorld& world)
{
    lightEntity = world.CreateEntity();

    auto& flashlight = world.AddComponent<LightComponent>(lightEntity);
    flashlight.type = LightType::Spot;
    flashlight.color = {1.0f, 0.97f, 0.88f};
    flashlight.intensity = intensity;
    flashlight.range = range;
    flashlight.innerConeAngleDeg = innerConeAngleDeg;
    flashlight.outerConeAngleDeg = outerConeAngleDeg;
    flashlight.serialized = false;
}

void FlashlightSystem::Update(EcsWorld& world, InputSystem* input, const Vector3& position, const Vector3& forward)
{
    if (lightEntity == 0)
        return;

    const bool keyPressed = input->IsKeyDown(Key::L);
    if (keyPressed && !keyWasPressed)
        on = !on;
    keyWasPressed = keyPressed;

    auto& light = world.GetComponent<LightComponent>(lightEntity);
    light.position = position;
    light.direction = forward;
    light.intensity = on ? intensity : 0.0f;
}
