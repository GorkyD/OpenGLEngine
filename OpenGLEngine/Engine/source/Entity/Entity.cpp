#include "Entity/Entity.h"
#include "Entity/EntitySystem.h"

Entity::Entity()
{
}

Entity::~Entity()
{
}

void Entity::Release()
{
	entitySystem->RemoveEntity(this);
}
