#include "Entity/EntitySystem.h"

EntitySystem::EntitySystem()
{
}

EntitySystem::~EntitySystem()
{
}

bool EntitySystem::CreateEntityInternal(Entity* entity, size_t id)
{
	auto ptr = std::unique_ptr<Entity>(entity);
	entities[id].emplace(entity, std::move(ptr));

	entity->entitySystem = this;
	entity->id = id;

	entity->OnCreate();

	return true;
}

void EntitySystem::RemoveEntity(Entity* entity)
{
	OGL_INFO("Remove entity with id: " << entity->id);
	entitiesToDestroy.emplace(entity);
}

void EntitySystem::Update(float deltaTime)
{
	for (auto entity: entitiesToDestroy)
	{
		entities[entity->id].erase(entity);
	}

	entitiesToDestroy.clear();
}
