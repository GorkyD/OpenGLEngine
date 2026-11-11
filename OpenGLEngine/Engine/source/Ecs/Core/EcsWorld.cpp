#include "Ecs/Core/EcsWorld.h"

Entity EcsWorld::CreateEntity()
{
    return nextEntity++;
}

void EcsWorld::DestroyEntity(Entity entity)
{
    toDestroy.push_back(entity);
}

void EcsWorld::ProcessDeferred()
{
    if (toDestroy.empty())
        return;

    for (auto& pool : pools)
    {
        if (!pool)
            continue;

        for (const Entity e : toDestroy)
            pool->Remove(e);
    }

    toDestroy.clear();
}

void EcsWorld::Clear()
{
    for (auto& pool : pools)
    {
        if (pool)
            pool->Clear();
    }

    toDestroy.clear();
}
