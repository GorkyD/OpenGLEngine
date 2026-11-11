#include "FpsScene.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <imgui.h>
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/ColliderComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/NameComponent.h"
#include "Ecs/Components/PostProcessComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Ecs/Systems/FpsCounterSystem.h"
#include "Engine/Engine.h"
#include "Input/InputSystem.h"
#include "Physics/Physics.h"
#include "Systems/EnvironmentBuilder.h"
#include "Systems/ForestBuilder.h"
#include "Window/Window.h"

namespace
{
constexpr float EyeHeight = 1.7f;
constexpr float PickupRange = 2.6f;
constexpr float FireUseRange = 2.8f;
constexpr float PistolBodyDamage = 25.0f;
constexpr float HeadshotMultiplier = 2.5f;
constexpr float MeleeDamage = 34.0f;
constexpr float MeleeRange = 2.2f;
constexpr float GunshotNoiseRadius = 35.0f;
constexpr float BandageHeal = 40.0f;
constexpr float FireOutDeathTime = 30.0f;

enum KeySlot
{
    KeyUse,
    KeyHeal,
    KeyReload,
    KeyMelee,
    KeyRestart
};
}

void FpsScene::OnLoad(Engine& engine)
{
    auto& world = engine.GetWorld();

    engine.GetWindow()->SetCursorLocked(true);

    playerHealth = 100.0f;
    playerDead = false;
    deathReason.clear();
    hurtTimer = 0.0f;
    meleeCooldown = 0.0f;
    pickups.clear();
    inventory.Clear();
    enemySystem.Clear();

    hudSystem.Init(engine.GetRenderEngine(), engine.GetShaders().hud, engine.GetWindow());
    gameHud.Init(engine);

    if (auto* fpsCounter = engine.GetFpsCounterSystem())
        fpsCounter->color = {0.6f, 0.56f, 0.48f, 1.0f};

    EnvironmentBuilder::Build(engine);
    for (auto& [entity, post] : world.GetPool<PostProcessComponent>())
    {
        post.psxEnabled = true;
        post.exposure = 1.25f;
        post.vignetteStrength = baseVignette;
    }

    BuildWorld(engine);
    SpawnPlayer(engine);
    ForestBuilder::ApplyPointFiltering(engine);

    inventory.Add(ItemId::PistolAmmo, 4);
    inventory.Add(ItemId::Bandage, 1);
    weapon.magazine = weapon.magazineCapacity;
}

void FpsScene::BuildWorld(Engine& engine)
{
    auto& world = engine.GetWorld();

    const auto fire = ForestBuilder::BuildCampfire(engine, {0.0f, ForestBuilder::GroundHeight(world, 0.0f, 0.0f), 0.0f}, 9.0f);
    campfire.Configure(world, fire);

    ForestBuilder::PlaceCampItems(engine);
    ForestBuilder::ScatterForest(engine, 7331u, 10.0f);

    const auto car = ForestBuilder::SpawnProp(engine, "Assets/Models/Vehicles/PSXCars/Car 01/Car.obj", 12.0f, -10.0f, 1.5f, 115.0f, 0.1f);
    ForestBuilder::AddBoxCollider(engine, car, 0.3f);

    const auto cottage = ForestBuilder::SpawnProp(engine, "Assets/Models/Buildings/Cottage/House_Cottage.fbx", 26.0f, 24.0f, 6.5f, 180.0f, 0.2f);
    ForestBuilder::AddBoxCollider(engine, cottage, 0.6f);

    Vector3 cottageMin;
    Vector3 cottageMax;
    ForestBuilder::Bounds(world, cottage, cottageMin, cottageMax);
    const float frontZ = cottageMin.z - 1.4f;
    const float centerX = (cottageMin.x + cottageMax.x) * 0.5f;

    const std::string survival = "Assets/Models/Items/SurvivalPSX/Models/Survival.fbx#";
    SpawnPickup(engine, ItemId::PistolAmmo, 6, survival + "Ammunition", centerX - 2.5f, frontZ, 0.25f, true);
    SpawnPickup(engine, ItemId::Bandage, 1, "Assets/Models/Items/Medkit/SM_Bandage_PSX.fbx", centerX + 1.5f, frontZ - 0.6f, 0.22f, true);
    SpawnPickup(engine, ItemId::Wood, 2, survival + "Trunk_01", cottageMax.x + 1.6f, (cottageMin.z + cottageMax.z) * 0.5f, 1.0f, true);
    SpawnPickup(engine, ItemId::Battery, 1, survival + "Battery", centerX + 3.5f, frontZ + 0.3f, 0.14f, true);
    SpawnPickup(engine, ItemId::PistolAmmo, 3, survival + "Ammunition", 13.5f, -7.4f, 0.25f, true);

    const float woodSpots[][2] = {{-14.0f, 6.0f}, {-9.0f, -15.0f}, {6.0f, 17.0f}, {18.0f, 3.0f}, {-22.0f, -4.0f}, {3.0f, -21.0f}};
    for (const auto& spot : woodSpots)
        SpawnPickup(engine, ItemId::Wood, 1, survival + "Trunk_01", spot[0], spot[1], 1.0f, true);

    enemySystem.Spawn(engine, {22.0f, 0.0f, 16.0f}, 200.0f);
    enemySystem.Spawn(engine, {-18.0f, 0.0f, 22.0f}, 120.0f);
    enemySystem.Spawn(engine, {6.0f, 0.0f, 31.0f}, 180.0f);
}

Entity FpsScene::SpawnPickup(Engine& engine, ItemId item, int count, const std::string& model, float x, float z, float size, bool fitLargestSide)
{
    auto& world = engine.GetWorld();

    const auto parts = ForestBuilder::SpawnProp(engine, model, x, z, size, static_cast<float>((static_cast<int>(x * 37.0f + z * 11.0f) % 360 + 360) % 360), 0.0f, fitLargestSide);
    if (parts.empty())
        return 0;

    for (const auto part : parts)
    {
        if (!world.HasComponent<MaterialComponent>(part))
            continue;
        auto& material = world.GetComponent<MaterialComponent>(part);
        material.emissiveColor = {1.0f, 0.85f, 0.55f};
        material.emissiveIntensity = 0.18f;
    }

    const Entity entity = world.CreateEntity();
    world.AddComponent<NameComponent>(entity).name = std::string("Pickup ") + ItemName(item);
    auto& pickup = world.AddComponent<PickupComponent>(entity);
    pickup.item = item;
    pickup.count = count;
    pickup.parts = parts;
    pickup.center = ForestBuilder::BoundsCenter(world, parts);

    pickups.push_back(entity);
    return entity;
}

void FpsScene::SpawnPlayer(Engine& engine)
{
    auto& world = engine.GetWorld();

    cameraEntity = world.CreateEntity();
    world.AddComponent<NameComponent>(cameraEntity).name = "Player";
    auto& transform = world.AddComponent<TransformComponent>(cameraEntity);
    transform.position = {0.0f, ForestBuilder::GroundHeight(world, 0.0f, -5.0f) + EyeHeight, -5.0f};
    world.AddComponent<CameraComponent>(cameraEntity).fovY = 70.0f * 3.14159265f / 180.0f;

    auto& collider = world.AddComponent<ColliderComponent>(cameraEntity);
    collider.center = {0.0f, -0.8f, 0.0f};
    collider.size = {0.6f, 1.8f, 0.6f};
    collider.restitution = 0.0f;
    collider.layer = PhysicsLayers::Player;

    characterController.Configure(world, cameraEntity);
    characterController.walkSpeed = 3.2f;
    characterController.jumpSpeed = 4.5f;

    firstPersonCamera.Configure(cameraEntity);
    firstPersonCamera.SetLook(0.0f, -0.1f);
    flashlight.Configure(world);
    flashlight.SetOn(false);

    viewModel.Spawn(engine, cameraEntity);
    weapon.Configure(cameraEntity, viewModel.GetMuzzleEntity());
}

void FpsScene::SetPlayerPose(Engine& engine, const Vector3& eyePosition, float yaw, float pitch)
{
    auto& world = engine.GetWorld();
    if (!world.HasComponent<TransformComponent>(cameraEntity))
        return;

    world.GetComponent<TransformComponent>(cameraEntity).position = eyePosition;
    firstPersonCamera.SetLook(yaw, pitch);
}

void FpsScene::OnUnload(Engine& engine)
{
    engine.GetWindow()->SetCursorLocked(false);
}

float FpsScene::CampfireFuel(Engine& engine) const
{
    const auto* fire = campfire.Get(engine.GetWorld());
    return fire ? fire->fuel : 0.0f;
}

bool FpsScene::KeyPressed(Engine& engine, int slot, bool down)
{
    (void)engine;
    const bool pressed = down && !keyWasDown[slot];
    keyWasDown[slot] = down;
    return pressed;
}

void FpsScene::Kill(const std::string& reason)
{
    if (playerDead)
        return;
    playerDead = true;
    playerHealth = (std::max)(0.0f, playerHealth);
    deathReason = reason;
}

void FpsScene::ApplyPlayerDamage(Engine& engine, float amount)
{
    (void)engine;
    if (playerDead || amount <= 0.0f)
        return;

    playerHealth -= amount;
    hurtTimer = 0.6f;
    if (playerHealth <= 0.0f)
    {
        playerHealth = 0.0f;
        Kill("YOU DIED");
    }
}

void FpsScene::UpdateInteraction(Engine& engine, bool usePressed)
{
    auto& world = engine.GetWorld();
    const auto& cam = world.GetComponent<CameraComponent>(cameraEntity);
    const Vector3 eye = world.GetComponent<TransformComponent>(cameraEntity).position;

    targetPickup = 0;
    targetFire = false;
    float bestDistance = PickupRange;

    for (const auto entity : pickups)
    {
        if (!world.HasComponent<PickupComponent>(entity))
            continue;
        const auto& pickup = world.GetComponent<PickupComponent>(entity);
        const Vector3 toPickup = pickup.center - eye;
        const float distance = toPickup.Length();
        if (distance > bestDistance || distance < 0.001f)
            continue;
        if (Vector3::Dot(toPickup * (1.0f / distance), cam.forward) < 0.82f)
            continue;
        bestDistance = distance;
        targetPickup = entity;
    }

    const Vector3 firePosition = campfire.Position();
    Vector3 toFire = firePosition - eye;
    const float fireDistance = toFire.Length();
    if (targetPickup == 0 && fireDistance < FireUseRange && fireDistance > 0.001f && Vector3::Dot(toFire * (1.0f / fireDistance), cam.forward) > 0.5f)
        targetFire = true;

    prompt.clear();
    if (targetPickup != 0)
    {
        const auto& pickup = world.GetComponent<PickupComponent>(targetPickup);
        prompt = std::string("[E] Take ") + ItemName(pickup.item);
        if (pickup.count > 1)
            prompt += " x" + std::to_string(pickup.count);
    }
    else if (targetFire)
    {
        prompt = inventory.Count(ItemId::Wood) > 0 ? "[E] Feed the fire" : "Need firewood";
    }

    if (!usePressed)
        return;

    if (targetPickup != 0)
    {
        auto& pickup = world.GetComponent<PickupComponent>(targetPickup);
        const int taken = inventory.Add(pickup.item, pickup.count);
        pickup.count -= taken;
        if (taken == 0)
        {
            prompt = "Inventory full";
            return;
        }
        if (pickup.count <= 0)
        {
            for (const auto part : pickup.parts)
                world.DestroyEntity(part);
            world.DestroyEntity(targetPickup);
            pickups.erase(std::remove(pickups.begin(), pickups.end(), targetPickup), pickups.end());
            targetPickup = 0;
        }
        return;
    }

    if (targetFire && inventory.Count(ItemId::Wood) > 0 && campfire.Feed(world))
        inventory.Remove(ItemId::Wood, 1);
}

void FpsScene::UpdateCombat(Engine& engine, float deltaTime)
{
    auto& world = engine.GetWorld();
    auto* input = engine.GetInputSystem();
    const Vector3 eye = world.GetComponent<TransformComponent>(cameraEntity).position;

    meleeCooldown = (std::max)(0.0f, meleeCooldown - deltaTime);

    const bool uiCapturing = ImGui::GetIO().WantCaptureMouse;
    const bool fireHeld = input->IsKeyDown(Key::F) || (input->IsLeftMouseDown() && !uiCapturing);
    const bool reloadHeld = input->IsKeyDown(Key::R) && weapon.CanReload(inventory);

    const ViewModelActions actions = viewModel.Update(engine, firstPersonCamera.IsMoving(), fireHeld && weapon.CanFire(), reloadHeld, deltaTime);

    if (actions.fired)
    {
        const ShotResult shot = weapon.Fire(engine);
        enemySystem.HearNoise(world, eye, GunshotNoiseRadius);

        if (shot.hit)
        {
            const bool isEnemy = world.HasComponent<EnemyComponent>(shot.entity);
            Vector3 back = shot.origin - shot.point;
            const float length = back.Length();
            back = length > 0.001f ? back * (1.0f / length) : Vector3(0, 1, 0);
            weapon.SpawnImpact(engine, shot.point, back, isEnemy);

            if (isEnemy)
            {
                const float feet = world.GetComponent<TransformComponent>(shot.entity).position.y;
                const bool headshot = shot.point.y >= feet + EnemySystem::HeadHeight;
                enemySystem.Damage(engine, shot.entity, PistolBodyDamage * (headshot ? HeadshotMultiplier : 1.0f));
            }
        }
    }

    if (actions.reloaded)
        weapon.Reload(inventory);

    if (KeyPressed(engine, KeyMelee, input->IsKeyDown(Key::V)) && meleeCooldown <= 0.0f)
    {
        meleeCooldown = 0.9f;
        const ShotResult swing = weapon.Trace(engine, MeleeRange);
        if (swing.hit && world.HasComponent<EnemyComponent>(swing.entity))
        {
            weapon.SpawnImpact(engine, swing.point, Vector3::Normalize(swing.origin - swing.point), true);
            enemySystem.Damage(engine, swing.entity, MeleeDamage);
        }
    }

    if (KeyPressed(engine, KeyHeal, input->IsKeyDown(Key::H)) && playerHealth < 100.0f && inventory.Count(ItemId::Bandage) > 0)
    {
        inventory.Remove(ItemId::Bandage, 1);
        playerHealth = (std::min)(100.0f, playerHealth + BandageHeal);
    }
}

void FpsScene::UpdateFeedback(Engine& engine, float deltaTime)
{
    hurtTimer = (std::max)(0.0f, hurtTimer - deltaTime);
    const float lowHealth = playerHealth < 30.0f ? 0.25f : 0.0f;
    for (auto& [entity, post] : engine.GetWorld().GetPool<PostProcessComponent>())
    {
        post.vignetteStrength = (std::min)(1.0f, baseVignette + hurtTimer * 0.7f + lowHealth + (playerDead ? 0.4f : 0.0f));
        break;
    }
}

void FpsScene::OnUpdate(Engine& engine, float deltaTime)
{
    if (cameraEntity == 0)
        return;

    auto& world = engine.GetWorld();
    auto* input = engine.GetInputSystem();

    const bool usePressed = KeyPressed(engine, KeyUse, input->IsKeyDown(Key::E));
    const bool restartPressed = KeyPressed(engine, KeyRestart, input->IsKeyDown(Key::R));

    campfire.Update(world, deltaTime);
    const auto* fire = campfire.Get(world);

    if (!playerDead)
    {
        firstPersonCamera.Update(world, input, characterController, deltaTime);

        const auto& cam = world.GetComponent<CameraComponent>(cameraEntity);
        const auto& camTransform = world.GetComponent<TransformComponent>(cameraEntity);
        flashlight.Update(world, input, camTransform.position, cam.forward);

        UpdateInteraction(engine, usePressed);
        UpdateCombat(engine, deltaTime);

        if (fire && fire->outTimer >= FireOutDeathTime)
            Kill("THE FIRE WENT OUT");
    }
    else
    {
        prompt.clear();
        if (restartPressed)
        {
            engine.LoadScene(std::make_unique<FpsScene>(), "Fps");
            return;
        }
    }

    weapon.Update(world, deltaTime);

    const Vector3 eye = world.GetComponent<TransformComponent>(cameraEntity).position;
    EnemyFrameInput enemyInput;
    enemyInput.playerEyePosition = eye;
    enemyInput.playerPosition = eye - Vector3(0.0f, EyeHeight, 0.0f);
    enemyInput.playerAlive = !playerDead;
    enemyInput.flashlightOn = flashlight.IsOn();
    enemyInput.firePosition = campfire.Position();
    enemyInput.fireSafeRadius = fire ? fire->SafeRadius() : 0.0f;
    ApplyPlayerDamage(engine, enemySystem.Update(engine, enemyInput, deltaTime));

    UpdateFeedback(engine, deltaTime);

    gameHud.Update(engine, playerHealth, weapon.magazine, inventory.Count(ItemId::PistolAmmo), fire ? fire->FuelRatio() : 0.0f, inventory, prompt, playerDead ? deathReason : std::string(),
                   playerDead ? "Press R to try again" : std::string());
}

void FpsScene::OnEditorUpdate(Engine& engine, float deltaTime)
{
    (void)deltaTime;
    viewModel.OnEditorUpdate(engine.GetWorld());
}

void FpsScene::OnRender(Engine& engine)
{
    (void)engine;
    if (cameraEntity == 0 || playerDead)
        return;

    hudSystem.Draw();
}

void FpsScene::OnDebugUI(Engine& engine)
{
    auto& world = engine.GetWorld();

    ImGui::Text("HP %.0f  mag %d  fuel %.1f  alive enemies %d", playerHealth, weapon.magazine, CampfireFuel(engine), enemySystem.AliveCount(world));

    for (auto& [entity, post] : world.GetPool<PostProcessComponent>())
    {
        ImGui::Checkbox("PS1", &post.psxEnabled);
        ImGui::DragFloat("Exposure", &post.exposure, 0.01f, 0.1f, 4.0f);
        ImGui::DragFloat("Vignette", &baseVignette, 0.01f, 0.0f, 1.0f);
        ImGui::Separator();
        break;
    }

    viewModel.OnDebugUI(world);
}
