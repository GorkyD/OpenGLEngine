#pragma once

#include "Ecs/Core/Entity.h"
#include "Extension/Extension.h"
#include "Math/Vector3.h"
#include "Scene/IScene.h"

class DemoScene : public IScene
{
public:
    void OnLoad(Engine& engine) override;
    void OnUpdate(Engine& engine, float deltaTime) override;

private:
    void CreateSkybox(Engine& engine, ShaderProgramPtr shader);
    void CreateTorch(Engine& engine, const Vector3& basePosition, ShaderProgramPtr handleShader, ShaderProgramPtr fireShader);
    void CreateHangingLamp(Engine& engine, const Vector3& pivot, ShaderProgramPtr shader);
    void CreateRain(Engine& engine);
    void CreateMaterialGallery(Engine& engine, ShaderProgramPtr shader, float platformTopY, float platformHalfWidth);
    void CreateUiText(Engine& engine);

    VertexArrayObjectPtr CreateFlameMesh(Engine& engine, unsigned int& outIndexCount);

    Entity fpsTextEntity = 0;
    Entity cameraEntity = 0;

    float fpsUpdateTimer = 0.0f;
    int fpsFrameCount = 0;
};
