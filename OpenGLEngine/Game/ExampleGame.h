#pragma once
#include "Engine/Engine.h"
#include "Math/Vector3.h"
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
    void CreateSkybox(ShaderProgramPtr shader);
    void CreateTorch(const Vector3& basePosition, ShaderProgramPtr handleShader, ShaderProgramPtr fireShader);
    VertexArrayObjectPtr CreateFlameMesh(unsigned int& outIndexCount);
    void CreateUiText();

    Entity fpsTextEntity = 0;
    float fpsUpdateTimer = 0.0f;
    int fpsFrameCount = 0;

    Entity cameraEntity = 0;
};
