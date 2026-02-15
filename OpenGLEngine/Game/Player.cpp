#include "Player.h"
#include "Entity/EntitySystem.h"

Player::Player()
{
}

Player::~Player()
{
}

void Player::OnCreate(Entity* entity)
{
	this->entity = entity;
}

void Player::OnUpdate(float deltaTime)
{
	elapsedSeconds += deltaTime;

	if (entity && elapsedSeconds >= 3.0f)
	{
		entity->Release();
		entity = nullptr;
	}

}
