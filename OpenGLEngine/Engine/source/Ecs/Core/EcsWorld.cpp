#include "Ecs/Core/EcsWorld.h"
#include <algorithm>

Entity EcsWorld::CreateEntity()
{
    Entity e = nextEntity++;
    alive.push_back(e);
    return e;
}

void EcsWorld::DestroyEntity(Entity entity)
{
    toDestroy.push_back(entity);
}

void EcsWorld::ProcessDeferred()
{
    if (toDestroy.empty())
        return;

    for (auto e : toDestroy)
    {
        for (auto& pool : pools)
        {
            if (pool)
                pool->Remove(e);
        }

        alive.erase(std::remove(alive.begin(), alive.end(), e), alive.end());
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

    alive.clear();
    toDestroy.clear();
}