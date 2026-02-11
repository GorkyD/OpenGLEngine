#include "Entity/OEntity.h"
#include "Entity/OEntitySystem.h"

OEntity::OEntity()
{
}

OEntity::~OEntity()
{
}

void OEntity::Release()
{
	entitySystem->RemoveEntity(this);
}
