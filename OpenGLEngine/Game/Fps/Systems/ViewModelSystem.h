#pragma once

#include <memory>
#include <string>
#include <vector>
#include "Ecs/Core/Entity.h"
#include "Math/Vector3.h"

struct AnimatorState;
class Engine;
class EcsWorld;

struct ViewModelActions
{
    bool fired = false;
    bool reloaded = false;
};

class ViewModelSystem
{
public:
    void Spawn(Engine& engine, Entity cameraEntity);
    ViewModelActions Update(Engine& engine, bool isMoving, bool firePressed, bool reloadPressed, float deltaTime);
    void OnEditorUpdate(EcsWorld& world);
    void OnDebugUI(EcsWorld& world);

    Entity GetArmsEntity() const
    {
        return parts.empty() ? 0 : parts.front();
    }
    Entity GetMuzzleEntity() const
    {
        return muzzleEntity;
    }
    bool IsBusy() const
    {
        return actionTimer < actionDuration;
    }
    const std::string& CurrentAction() const
    {
        return currentAction;
    }

private:
    void Place(EcsWorld& world, float deltaTime, bool isMoving);
    void StartAction(const std::string& clip);
    float ClipSeconds(const std::string& clip) const;

    Entity cameraEntity = 0;
    Entity muzzleEntity = 0;
    std::vector<Entity> parts;
    std::shared_ptr<AnimatorState> animator;

    std::string currentAction;
    float actionTimer = 0.0f;
    float actionDuration = 0.0f;
    bool firePressedLastFrame = false;
    bool reloadPressedLastFrame = false;

    float bobTime = 0.0f;
    float bobWeight = 0.0f;

    Vector3 offset = {0.0f, 0.0f, 0.0f};
    Vector3 muzzleLocal = {0.13f, -0.09f, 0.55f};
    float bobAmount = 0.012f;
    float bobSpeed = 8.5f;
};
