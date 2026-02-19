#pragma once
#include "Engine/Engine.h"

class ExampleGame : public Engine
{
	public:
		ExampleGame();
		~ExampleGame();

	void OnCreate() override;
	void OnUpdate(float deltaTime) override;

};
