#include "Ecs/Core/EcsWorld.h"

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
	for (auto e : toDestroy)
	{
		for (auto& pair : pools)
		{
			auto& key = pair.first;
			const auto& pool = pair.second;
			pool->Remove(e);
		}

		alive.erase(std::remove(alive.begin(), alive.end(), e), alive.end());
	}

	toDestroy.clear();
}