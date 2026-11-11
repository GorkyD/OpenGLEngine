#include "Systems/ViewModelSystem.h"
#include <algorithm>
#include <cmath>
#include <imgui.h>
#include "Ecs/Components/AnimatorComponent.h"
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/NameComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Engine/Engine.h"
#include "Math/Matrix4.h"
#include "Resource/EntityFactory.h"

namespace
{
constexpr const char* ViewModelPath = "Assets/Models/Weapons/PSXFirearms/glb/fp_pistol.glb";
}

void ViewModelSystem::Spawn(Engine& engine, Entity playerCameraEntity)
{
    cameraEntity = playerCameraEntity;
    parts.clear();
    animator.reset();

    auto& world = engine.GetWorld();

    const Entity first = EntityFactory::CreateSkinnedModelEntity(world, engine.GetRenderEngine(), ViewModelPath, engine.GetShaders().skinned);
    if (first == 0 || !world.HasComponent<AnimatorComponent>(first))
        return;

    animator = world.GetComponent<AnimatorComponent>(first).state;
    for (auto& [entity, component] : world.GetPool<AnimatorComponent>())
    {
        if (component.state != animator)
            continue;
        parts.push_back(entity);
        if (world.HasComponent<MeshComponent>(entity))
        {
            auto& mesh = world.GetComponent<MeshComponent>(entity);
            mesh.castsShadow = false;
            mesh.renderQueue = RenderQueue::Overlay;
        }
    }

    muzzleEntity = world.CreateEntity();
    world.AddComponent<NameComponent>(muzzleEntity).name = "Muzzle";
    world.AddComponent<TransformComponent>(muzzleEntity);

    if (engine.GetMode() == EngineMode::Editor)
    {
        Animator::Play(*animator, "idle", true);
        return;
    }

    StartAction("draw");
}

float ViewModelSystem::ClipSeconds(const std::string& clip) const
{
    if (!animator || !animator->clips)
        return 0.0f;
    for (const auto& candidate : *animator->clips)
    {
        if (candidate.name == clip)
            return candidate.durationTicks / (candidate.ticksPerSecond > 0.0f ? candidate.ticksPerSecond : 25.0f);
    }
    return 0.0f;
}

void ViewModelSystem::StartAction(const std::string& clip)
{
    if (!animator)
        return;
    currentAction = clip;
    actionTimer = 0.0f;
    actionDuration = ClipSeconds(clip);
    Animator::Play(*animator, clip, false);
}

void ViewModelSystem::Place(EcsWorld& world, float deltaTime, bool isMoving)
{
    if (cameraEntity == 0 || !world.HasComponent<CameraComponent>(cameraEntity))
        return;

    const auto& cam = world.GetComponent<CameraComponent>(cameraEntity);
    const Vector3 eye = world.GetComponent<TransformComponent>(cameraEntity).position;

    bobWeight += ((isMoving ? 1.0f : 0.0f) - bobWeight) * (std::min)(1.0f, deltaTime * 8.0f);
    bobTime += deltaTime * bobSpeed * bobWeight;
    const float bobX = std::sin(bobTime) * bobAmount * bobWeight;
    const float bobY = -std::fabs(std::cos(bobTime)) * bobAmount * 0.8f * bobWeight;

    Matrix4 rotation;
    rotation.matrix[0][0] = -cam.right.x;
    rotation.matrix[0][1] = -cam.right.y;
    rotation.matrix[0][2] = -cam.right.z;
    rotation.matrix[1][0] = cam.up.x;
    rotation.matrix[1][1] = cam.up.y;
    rotation.matrix[1][2] = cam.up.z;
    rotation.matrix[2][0] = -cam.forward.x;
    rotation.matrix[2][1] = -cam.forward.y;
    rotation.matrix[2][2] = -cam.forward.z;

    const Vector3 position = eye + cam.right * (offset.x + bobX) + cam.up * (offset.y + bobY) + cam.forward * offset.z;

    for (const auto part : parts)
    {
        if (!world.HasComponent<TransformComponent>(part))
            continue;
        auto& transform = world.GetComponent<TransformComponent>(part);
        transform.position = position;
        transform.rotation = rotation;
        transform.scale = {1.0f, 1.0f, 1.0f};
    }

    if (muzzleEntity != 0 && world.HasComponent<TransformComponent>(muzzleEntity))
        world.GetComponent<TransformComponent>(muzzleEntity).position = position + cam.right * muzzleLocal.x + cam.up * muzzleLocal.y + cam.forward * muzzleLocal.z;
}

ViewModelActions ViewModelSystem::Update(Engine& engine, bool isMoving, bool firePressed, bool reloadPressed, float deltaTime)
{
    ViewModelActions actions;
    auto& world = engine.GetWorld();

    Place(world, deltaTime, isMoving);

    if (!animator)
        return actions;

    actionTimer += deltaTime;
    const bool busy = actionTimer < actionDuration;

    if (!busy && currentAction != "idle")
    {
        currentAction = "idle";
        Animator::Play(*animator, "idle", true);
    }

    const bool firePressedNow = firePressed && !firePressedLastFrame;
    const bool reloadPressedNow = reloadPressed && !reloadPressedLastFrame;
    const bool canInterrupt = !busy || currentAction == "draw" || (currentAction == "fire" && actionTimer > actionDuration * 0.6f);

    if (firePressedNow && canInterrupt && currentAction != "reload")
    {
        StartAction("fire");
        actions.fired = true;
    }
    else if (reloadPressedNow && !busy)
    {
        StartAction("reload");
        actions.reloaded = true;
    }

    firePressedLastFrame = firePressed;
    reloadPressedLastFrame = reloadPressed;
    return actions;
}

void ViewModelSystem::OnEditorUpdate(EcsWorld& world)
{
    for (const auto part : parts)
    {
        if (!world.HasComponent<TransformComponent>(part))
            continue;
        auto& transform = world.GetComponent<TransformComponent>(part);
        transform.position = {0.0f, 1.5f, 0.0f};
        transform.scale = {1.0f, 1.0f, 1.0f};
    }
}

void ViewModelSystem::OnDebugUI(EcsWorld& world)
{
    (void)world;
    ImGui::TextUnformatted("View model");
    ImGui::DragFloat3("Offset (R/U/F)", &offset.x, 0.005f);
    ImGui::DragFloat3("Muzzle (R/U/F)", &muzzleLocal.x, 0.005f);
    ImGui::DragFloat("Bob amount", &bobAmount, 0.001f, 0.0f, 0.1f);
    ImGui::DragFloat("Bob speed", &bobSpeed, 0.1f, 0.0f, 30.0f);
    ImGui::Text("action: %s  %.2f / %.2f", currentAction.c_str(), actionTimer, actionDuration);
}
