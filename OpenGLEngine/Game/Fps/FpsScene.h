#pragma once

#include <string>
#include <vector>
#include "Ecs/Core/Entity.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Scene/IScene.h"
#include "Systems/CampfireSystem.h"
#include "Systems/CharacterControllerSystem.h"
#include "Systems/EnemySystem.h"
#include "Systems/FirstPersonCameraSystem.h"
#include "Systems/FlashlightSystem.h"
#include "Systems/GameHud.h"
#include "Systems/HudSystem.h"
#include "Systems/Inventory.h"
#include "Systems/ViewModelSystem.h"
#include "Systems/WeaponSystem.h"

class FpsScene : public IScene
{
public:
    void OnLoad(Engine& engine) override;
    void OnUnload(Engine& engine) override;
    void OnUpdate(Engine& engine, float deltaTime) override;
    void OnEditorUpdate(Engine& engine, float deltaTime) override;
    void OnRender(Engine& engine) override;
    void OnDebugUI(Engine& engine) override;

    Vector4 GetClearColor() const override
    {
        return {0.02f, 0.025f, 0.045f, 1.0f};
    }

    bool WantsCursorLocked() const override
    {
        return true;
    }

    float PlayerHealth() const
    {
        return playerHealth;
    }
    bool IsPlayerDead() const
    {
        return playerDead;
    }
    const std::string& DeathReason() const
    {
        return deathReason;
    }
    const Inventory& PlayerInventory() const
    {
        return inventory;
    }
    int Magazine() const
    {
        return weapon.magazine;
    }
    float CampfireFuel(Engine& engine) const;
    Vector3 CampfirePosition() const
    {
        return campfire.Position();
    }
    const std::vector<Entity>& Enemies() const
    {
        return enemySystem.Enemies();
    }
    const std::vector<Entity>& Pickups() const
    {
        return pickups;
    }
    Entity PlayerEntity() const
    {
        return cameraEntity;
    }
    const std::string& CurrentPrompt() const
    {
        return prompt;
    }

    void SetPlayerPose(Engine& engine, const Vector3& eyePosition, float yaw, float pitch);

private:
    void BuildWorld(Engine& engine);
    void SpawnPlayer(Engine& engine);
    Entity SpawnPickup(Engine& engine, ItemId item, int count, const std::string& model, float x, float z, float size, bool fitLargestSide);

    void UpdateInteraction(Engine& engine, bool usePressed);
    void UpdateCombat(Engine& engine, float deltaTime);
    void ApplyPlayerDamage(Engine& engine, float amount);
    void Kill(const std::string& reason);
    void UpdateFeedback(Engine& engine, float deltaTime);

    bool KeyPressed(Engine& engine, int key, bool down);

    CharacterControllerSystem characterController;
    FirstPersonCameraSystem firstPersonCamera;
    FlashlightSystem flashlight;
    ViewModelSystem viewModel;
    WeaponSystem weapon;
    HudSystem hudSystem;
    GameHud gameHud;
    EnemySystem enemySystem;
    CampfireSystem campfire;
    Inventory inventory;

    std::vector<Entity> pickups;
    Entity targetPickup = 0;
    bool targetFire = false;
    std::string prompt;

    Entity cameraEntity = 0;
    float playerHealth = 100.0f;
    float hurtTimer = 0.0f;
    float meleeCooldown = 0.0f;
    float baseVignette = 0.55f;
    bool playerDead = false;
    std::string deathReason;

    bool keyWasDown[8] = {};
};
