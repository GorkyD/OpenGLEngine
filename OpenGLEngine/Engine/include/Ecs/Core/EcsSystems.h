#pragma once

#include <vector>
#include <memory>
#include "IEcsSystem.h"

enum class SystemPhase
{
    Update,
    Render,
    Overlay
};

class EcsSystems
{
public:
    EcsSystems(EcsWorld& w) : world(w) {}

    EcsSystems& Add(std::unique_ptr<IEcsSystem> system, SystemPhase phase, bool gameplayOnly = false);

    void Init() const;
    void Update(SystemPhase phase, float deltaTime, bool gameplayEnabled = true) const;

private:
    struct Entry
    {
        std::unique_ptr<IEcsSystem> system;
        SystemPhase phase = SystemPhase::Update;
        bool gameplayOnly = false;
    };

    EcsWorld& world;
    std::vector<Entry> systems;
};
