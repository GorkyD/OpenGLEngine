#include "Ecs/Core/EcsSystems.h"

EcsSystems& EcsSystems::Add(std::unique_ptr<IEcsSystem> system, bool gameplayOnly)
{
    systems.push_back({std::move(system), gameplayOnly});
    return *this;
}

void EcsSystems::Init() const
{
    for (auto& s : systems)
        s.system->Init(world);
}

void EcsSystems::Update(float deltaTime, bool gameplayEnabled) const
{
    for (auto& s : systems)
    {
        if (s.gameplayOnly && !gameplayEnabled)
            continue;
        s.system->Run(world, deltaTime);
    }

    world.ProcessDeferred();
}
