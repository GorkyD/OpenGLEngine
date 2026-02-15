#pragma once
#include "Player.h"
#include "Engine/Engine.h"

class MyGame : public Engine
{
	public:
		MyGame();
		~MyGame();

	void OnCreate() override;
	void OnUpdate(float deltaTime) override;

	private:
		Player* player = nullptr;

		float elapsedSeconds = 0.0f;
};
