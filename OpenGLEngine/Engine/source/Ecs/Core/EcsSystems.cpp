#include "Ecs/Core/EcsSystems.h"

EcsSystems& EcsSystems::Add(std::unique_ptr<IEcsSystem> system, SystemPhase phase, bool gameplayOnly)
{
    systems.push_back({std::move(system), phase, gameplayOnly});
    return *this;
}

void EcsSystems::Init() const
{
    for (auto& s : systems)
        s.system->Init(world);
}

void EcsSystems::Update(SystemPhase phase, float deltaTime, bool gameplayEnabled) const
{
    for (auto& s : systems)
    {
        if (s.phase != phase)
            continue;
        if (s.gameplayOnly && !gameplayEnabled)
            continue;
        s.system->Run(world, deltaTime);
    }

    world.ProcessDeferred();
}
