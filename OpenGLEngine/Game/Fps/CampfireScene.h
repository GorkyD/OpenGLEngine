#pragma once

#include <string>
#include <vector>
#include "Ecs/Core/Entity.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Scene/IScene.h"
#include "Systems/CharacterControllerSystem.h"
#include "Systems/FirstPersonCameraSystem.h"
#include "Systems/FlashlightSystem.h"

class CampfireScene : public IScene
{
public:
    void OnLoad(Engine& engine) override;
    void OnUnload(Engine& engine) override;
    void OnUpdate(Engine& engine, float deltaTime) override;
    void OnDebugUI(Engine& engine) override;

    Vector4 GetClearColor() const override
    {
        return {0.02f, 0.025f, 0.045f, 1.0f};
    }

    bool WantsCursorLocked() const override
    {
        return true;
    }

private:
    void SpawnPlayer(Engine& engine);
    void SpawnWanderer(Engine& engine, const char* clip, float startAngle, float radius, float speed);
    void UpdateWanderers(Engine& engine, float deltaTime);

    struct Wanderer
    {
        std::vector<Entity> parts;
        float angle = 0.0f;
        float radius = 10.0f;
        float speed = 0.8f;
    };

    CharacterControllerSystem characterController;
    FirstPersonCameraSystem firstPersonCamera;
    FlashlightSystem flashlight;

    std::vector<Wanderer> wanderers;

    Entity cameraEntity = 0;
    Entity fireLightEntity = 0;
    Vector3 fireLightBase = {0, 0, 0};
    float fireIntensity = 9.0f;
    float time = 0.0f;
};
