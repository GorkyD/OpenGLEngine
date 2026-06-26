#pragma once
#include "Engine/Engine.h"
#include <string>

class ExampleGame : public Engine
{
public:
    ExampleGame();
    ~ExampleGame();

    void OnCreate() override;
    void OnUpdate(float deltaTime) override;

private:
    Entity LoadModel(const std::string& path, ShaderProgramPtr shader);
};
