#pragma once
#include "Entity/OEntity.h"

class Player : public OEntity
{
public:
	Player();
	~Player();

	virtual void OnCreate();
	virtual void OnUpdate(float deltaTime);

private:
	OEntity* entity = nullptr;

	float elapsedSeconds = 0.0f;
};
