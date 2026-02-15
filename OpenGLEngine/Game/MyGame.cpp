#include "MyGame.h"

#include "Entity/EntitySystem.h"

MyGame::MyGame()
{
}

MyGame::~MyGame()
{
}

void MyGame::OnCreate()
{
	Engine::OnCreate();
	player = GetEntitySystem()->CreateEntity<Player>();
}

void MyGame::OnUpdate(float deltaTime)
{
	/*elapsedSeconds += deltaTime;

	if (player && elapsedSeconds >= 3.0f)
	{
		player->Release();
		player = nullptr;
	}*/
}
