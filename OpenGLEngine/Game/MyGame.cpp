#include "MyGame.h"

#include "Entity/OEntitySystem.h"

MyGame::MyGame()
{
}

MyGame::~MyGame()
{
}

void MyGame::OnCreate()
{
	OEngine::OnCreate();
	auto player = GetEntitySystem()->CreateEntity<Player>();
}

void MyGame::OnUpdate(float deltaTime)
{
	elapsedSeconds += deltaTime;

	if (player && elapsedSeconds >= 3.0f)
	{
		player->Release();
		player = nullptr;
	}
}
