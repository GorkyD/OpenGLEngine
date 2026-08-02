#pragma once

#include <vector>
#include <memory>
#include "IEcsSystem.h"

class EcsSystems
{
public:
    EcsSystems(EcsWorld& w) : world(w) {}

    EcsSystems& Add(std::unique_ptr<IEcsSystem> system, bool gameplayOnly = false);

    void Init() const;
    void Update(float deltaTime, bool gameplayEnabled = true) const;

private:
    struct Entry
    {
        std::unique_ptr<IEcsSystem> system;
        bool gameplayOnly = false;
    };

    EcsWorld& world;
    std::vector<Entry> systems;
};
