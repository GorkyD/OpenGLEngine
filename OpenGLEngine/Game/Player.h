#pragma once
#include "Entity/Entity.h"

class Player : public Entity
{
public:
	Player();
	~Player();

	virtual void OnCreate();
	virtual void OnUpdate(float deltaTime);

private:
	Entity* entity = nullptr;

	float elapsedSeconds = 0.0f;
};
