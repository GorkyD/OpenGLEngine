#include "Entity/OEntitySystem.h"

OEntitySystem::OEntitySystem()
{
}

OEntitySystem::~OEntitySystem()
{
}

bool OEntitySystem::CreateEntityInternal(OEntity* entity, size_t id)
{
	auto ptr = std::unique_ptr<OEntity>(entity);
	entities[id].emplace(entity, std::move(ptr));

	entity->entitySystem = this;
	entity->id = id;

	entity->OnCreate();

	return true;
}

void OEntitySystem::RemoveEntity(OEntity* entity)
{
	OGL_INFO("Remove entity with id: " << entity->id);
	entitiesToDestroy.emplace(entity);
}

void OEntitySystem::Update(float deltaTime)
{
	for (auto entity: entitiesToDestroy)
	{
		entities[entity->id].erase(entity);
	}

	entitiesToDestroy.clear();
}
