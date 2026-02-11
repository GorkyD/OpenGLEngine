#include "Player.h"
#include "Entity/OEntitySystem.h"

Player::Player()
{
}

Player::~Player()
{
}

void Player::OnCreate()
{
	entity = GetEntitySystem()->CreateEntity<OEntity>();
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