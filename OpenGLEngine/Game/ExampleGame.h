#pragma once
#include <string>
#include "Engine/Engine.h"

class ExampleGame : public Engine
{
public:
	ExampleGame();
	~ExampleGame();

	void OnCreate() override;
	void OnUpdate(float deltaTime) override;

private:
	void LoadModel(const std::string& path, ShaderProgramPtr shader);
};
