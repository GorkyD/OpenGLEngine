#pragma once
#include "Player.h"
#include "Engine/OEngine.h"

class MyGame : public OEngine
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

