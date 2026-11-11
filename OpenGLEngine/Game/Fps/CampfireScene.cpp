#include "CampfireScene.h"
#include <cmath>
#include <imgui.h>
#include "Ecs/Components/AnimatorComponent.h"
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/ColliderComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/NameComponent.h"
#include "Ecs/Components/PostProcessComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Engine/Engine.h"
#include "Input/InputSystem.h"
#include "Math/Matrix4.h"
#include "Physics/PhysicsLayers.h"
#include "Resource/EntityFactory.h"
#include "Systems/EnvironmentBuilder.h"
#include "Systems/ForestBuilder.h"
#include "Window/Window.h"

void CampfireScene::OnLoad(Engine& engine)
{
    time = 0.0f;
    wanderers.clear();

    engine.GetWindow()->SetCursorLocked(true);

    EnvironmentBuilder::Build(engine);

    auto& world = engine.GetWorld();
    for (auto& [entity, post] : world.GetPool<PostProcessComponent>())
    {
        post.psxEnabled = true;
        post.exposure = 1.25f;
        post.vignetteStrength = 0.55f;
    }

    const auto campfire = ForestBuilder::BuildCampfire(engine, {0.0f, ForestBuilder::GroundHeight(world, 0.0f, 0.0f), 0.0f}, fireIntensity);
    fireLightEntity = campfire.light;
    fireLightBase = campfire.position + Vector3(0.0f, 0.8f, 0.0f);

    ForestBuilder::PlaceCampItems(engine);
    ForestBuilder::ScatterForest(engine, 20261111u, 9.0f);
    ForestBuilder::SpawnProp(engine, "Assets/Models/Vehicles/PSXCars/Car 01/Car.obj", 11.0f, -9.0f, 1.5f, 115.0f, 0.1f);
    ForestBuilder::SpawnProp(engine, "Assets/Models/Buildings/Cottage/House_Cottage.fbx", 30.0f, 26.0f, 6.5f, -140.0f, 0.2f);

    SpawnWanderer(engine, "walk", 1.2f, 8.0f, 0.7f);
    SpawnWanderer(engine, "crouch_walk", 2.0f, 6.5f, -0.9f);
    SpawnWanderer(engine, "idle_alt", 1.75f, 9.5f, 0.0f);

    SpawnPlayer(engine);
    ForestBuilder::ApplyPointFiltering(engine);
}

void CampfireScene::OnUnload(Engine& engine)
{
    engine.GetWindow()->SetCursorLocked(false);
}

void CampfireScene::SpawnWanderer(Engine& engine, const char* clip, float startAngle, float radius, float speed)
{
    auto& world = engine.GetWorld();

    const Entity first = EntityFactory::CreateSkinnedModelEntity(world, engine.GetRenderEngine(), "Assets/Models/Enemies/ZombieWoman/glb/zombie_woman_ual.glb", engine.GetShaders().skinned);
    if (first == 0 || !world.HasComponent<AnimatorComponent>(first))
        return;

    const auto state = world.GetComponent<AnimatorComponent>(first).state;
    Animator::Play(*state, clip, true);

    Wanderer wanderer;
    wanderer.angle = startAngle;
    wanderer.radius = radius;
    wanderer.speed = speed;

    for (auto& [entity, animator] : world.GetPool<AnimatorComponent>())
    {
        if (animator.state == state)
            wanderer.parts.push_back(entity);
    }

    wanderers.push_back(std::move(wanderer));
}

void CampfireScene::UpdateWanderers(Engine& engine, float deltaTime)
{
    auto& world = engine.GetWorld();

    for (auto& wanderer : wanderers)
    {
        wanderer.angle += wanderer.speed / wanderer.radius * deltaTime;

        const float x = std::cos(wanderer.angle) * wanderer.radius;
        const float z = std::sin(wanderer.angle) * wanderer.radius;
        const float direction = wanderer.speed >= 0.0f ? 1.0f : -1.0f;
        const float tangentX = -std::sin(wanderer.angle) * direction;
        const float tangentZ = std::cos(wanderer.angle) * direction;
        const Matrix4 rotation = Matrix4::FromAxisAngle({0.0f, 1.0f, 0.0f}, std::atan2(tangentX, tangentZ) + 3.14159265f);
        const Vector3 position = {x, ForestBuilder::GroundHeight(world, x, z), z};

        for (const auto part : wanderer.parts)
        {
            if (!world.HasComponent<TransformComponent>(part))
                continue;
            auto& transform = world.GetComponent<TransformComponent>(part);
            transform.position = position;
            transform.rotation = rotation;
        }
    }
}

void CampfireScene::SpawnPlayer(Engine& engine)
{
    auto& world = engine.GetWorld();

    cameraEntity = world.CreateEntity();
    world.AddComponent<NameComponent>(cameraEntity).name = "Player";
    auto& transform = world.AddComponent<TransformComponent>(cameraEntity);
    transform.position = {0.0f, ForestBuilder::GroundHeight(world, 0.0f, -5.0f) + 1.7f, -5.0f};
    world.AddComponent<CameraComponent>(cameraEntity);

    auto& collider = world.AddComponent<ColliderComponent>(cameraEntity);
    collider.center = {0.0f, -0.8f, 0.0f};
    collider.size = {0.6f, 1.8f, 0.6f};
    collider.restitution = 0.0f;
    collider.layer = PhysicsLayers::Player;

    characterController.Configure(world, cameraEntity);
    characterController.walkSpeed = 3.0f;
    firstPersonCamera.Configure(cameraEntity);
    flashlight.Configure(world);
    flashlight.SetOn(false);
}

void CampfireScene::OnUpdate(Engine& engine, float deltaTime)
{
    if (cameraEntity == 0)
        return;

    time += deltaTime;

    auto& world = engine.GetWorld();
    auto* input = engine.GetInputSystem();

    firstPersonCamera.Update(world, input, characterController, deltaTime);
    UpdateWanderers(engine, deltaTime);

    const auto& cam = world.GetComponent<CameraComponent>(cameraEntity);
    const auto& camTransform = world.GetComponent<TransformComponent>(cameraEntity);
    flashlight.Update(world, input, camTransform.position, cam.forward);

    if (world.HasComponent<LightComponent>(fireLightEntity))
    {
        const float flicker = 0.78f + 0.12f * std::sin(time * 7.3f) + 0.07f * std::sin(time * 13.1f + 1.7f) + 0.05f * std::sin(time * 23.7f + 4.1f);
        auto& light = world.GetComponent<LightComponent>(fireLightEntity);
        light.intensity = fireIntensity * flicker;
        light.position = fireLightBase + Vector3(0.05f * std::sin(time * 5.1f), 0.04f * std::sin(time * 9.3f), 0.05f * std::cos(time * 6.7f));
    }
}

void CampfireScene::OnDebugUI(Engine& engine)
{
    for (auto& [entity, post] : engine.GetWorld().GetPool<PostProcessComponent>())
    {
        ImGui::TextUnformatted("PS1");
        ImGui::Checkbox("Enabled", &post.psxEnabled);
        ImGui::DragFloat("Resolution Height", &post.psxResolutionHeight, 1.0f, 120.0f, 1080.0f);
        ImGui::DragFloat("Color Levels", &post.psxColorLevels, 0.25f, 2.0f, 256.0f);
        ImGui::DragFloat("Dither", &post.psxDitherStrength, 0.01f, 0.0f, 2.0f);
        ImGui::Checkbox("Vertex Snap", &post.psxVertexSnap);
        ImGui::DragFloat("Snap Height", &post.psxVertexSnapHeight, 1.0f, 60.0f, 720.0f);
        ImGui::Separator();
        ImGui::DragFloat("Exposure", &post.exposure, 0.01f, 0.1f, 4.0f);
        ImGui::DragFloat("Vignette", &post.vignetteStrength, 0.01f, 0.0f, 1.0f);
        break;
    }

    ImGui::DragFloat("Fire Intensity", &fireIntensity, 0.05f, 0.0f, 20.0f);
}
