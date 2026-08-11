#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "Ecs/Core/Entity.h"
#include "Extension/Extension.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Scene/IScene.h"
#include "Systems/CharacterControllerSystem.h"
#include "Systems/HudSystem.h"

struct AnimatorState;

class FpsScene : public IScene
{
public:
    void OnLoad(Engine& engine) override;
    void OnUnload(Engine& engine) override;
    void OnUpdate(Engine& engine, float deltaTime) override;
    void OnEditorUpdate(Engine& engine, float deltaTime) override;
    void OnDebugUI(Engine& engine) override;

    Vector4 GetClearColor() const override
    {
        return {0.04f, 0.05f, 0.09f, 1.0f};
    }

    bool WantsCursorLocked() const override
    {
        return true;
    }

private:
    void CreateProceduralSky(Engine& engine);
    void CreateTerrain(Engine& engine);
    void UpdateLookAndMove(Engine& engine, float deltaTime);
    void StartAction(AnimatorState& state, const std::string& clipName);
    float GetClipDurationSeconds(const AnimatorState& state, const std::string& clipName) const;
    Vector3 ResolveAimPoint(class EcsWorld& world, const Vector3& eyePosition, const Vector3& forward) const;
    Vector3 ResolveMuzzlePosition(class EcsWorld& world, const Vector3& eyePosition, const Vector3& forward) const;
    void FireProjectile(Engine& engine);
    void UpdateProjectiles(class EcsWorld& world);
    void SpawnMuzzleFlash(Engine& engine, const Vector3& position, const Vector3& direction);
    void UpdateMuzzleFlashes(class EcsWorld& world, float deltaTime);
    void UpdateViewModel(class EcsWorld& world);
    void AttachWeaponToHand(class EcsWorld& world);

    CharacterControllerSystem characterController;
    HudSystem hudSystem;

    std::unordered_map<Entity, std::vector<Entity>> projectileParts;
    std::vector<Entity> finishedProjectiles;

    struct MuzzleFlashInstance
    {
        Entity entity = 0;
        float timeRemaining = 0.0f;
    };
    std::vector<MuzzleFlashInstance> muzzleFlashes;

    Entity cameraEntity = 0;
    Entity armsEntity = 0;
    Entity weaponEntity = 0;
    Entity muzzleEntity = 0;
    std::vector<Entity> weaponEntities;

    float yaw = 0.0f;
    float pitch = 0.0f;
    float mouseSensitivity = 0.0025f;
    float maxPitch = 1.5533f;
    float moveSpeed = 3.5f;
    float jumpSpeed = 4.5f;

    float bulletSpeed = 120.0f;
    float bulletRadius = 0.03f;
    float bulletLifetime = 3.0f;
    float muzzleForwardOffset = 0.8f;
    float aimRayDistance = 500.0f;

    bool wasMoving = false;
    bool isMovingThisFrame = false;

    bool inAction = false;
    float actionTimer = 0.0f;
    float actionDuration = 0.0f;

    bool firePressedLastFrame = false;
    bool reloadPressedLastFrame = false;

    bool flashlightOn = true;
    bool flashlightKeyWasPressed = false;
    float flashlightIntensity = 3.0f;

    Vector3 armsScale = {3.0f, 3.0f, 3.0f};
    Vector3 armsOffset = {0.35f, -0.55f, 0.85f};

    Vector3 armsOffsetDefault = {-0.12f, 0.04f, -0.72f};
    Vector3 armsRotationDefault = {0.0f, -20.0f, 0.0f};

    Vector3 weaponOffsetDefault = {-0.02f, 0.09f, 0.12f};
    Vector3 weaponRotationDefault = {179.5f, -76.5f, 4.0f};
    Vector3 weaponScaleDefault = {0.015f, 0.015f, 0.015f};

    Vector3 gripCenterOffset = {0.0f, 3.1f, 0.386f};
    float gripPitchDeg = -90.0f;
    float gripRollDeg = 90.0f;
    float gripScale = 0.02f;

    bool editorStaticPlaced = false;
    Vector3 editorStaticPosition = {0.0f, 1.5f, 0.0f};

    std::vector<bool> slideParts;
    Vector3 slideRecoilAxis = {0.0f, 0.0f, -1.0f};
    float slideRecoilDistance = 0.05f;
    float slideRecoilDuration = 0.12f;
    float slideRecoilTimer = -1.0f;
};
