#pragma once
#include <memory>
#include <chrono>

#include "Audio/AudioSystem.h"
#include "Extension/Extension.h"
#include "Input/InputSystem.h"
#include "Ecs/Core/EcsSystems.h"
#include "Scene/IScene.h"

class RenderEngine;
class Window;

struct StandardShaders
{
    ShaderProgramPtr unlit;
    ShaderProgramPtr lit;
    ShaderProgramPtr skybox;
    ShaderProgramPtr fire;
    ShaderProgramPtr particle;
    ShaderProgramPtr text;
};

class Engine
{
public:
    Engine();
    virtual ~Engine();

    void Run();
    void Quit();

    void LoadScene(std::unique_ptr<IScene> scene);

    EcsWorld& GetWorld() { return world; }
    RenderEngine* GetRenderEngine() const { return renderEngine.get(); }
    Window* GetWindow() const { return window.get(); }
    InputSystem* GetInputSystem() const { return inputSystem.get(); }
    AudioSystem* GetAudioSystem() const { return audioSystem.get(); }
    const StandardShaders& GetShaders() const { return shaders; }
    UniformBufferPtr GetUniformBuffer() const { return uniformBuffer; }

private:
    void OnUpdateInternal();
    void CreateStandardShaders();
    void CreateStandardSystems();

protected:
    virtual void OnCreate();
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnQuit();

    EcsWorld world;
    std::unique_ptr<EcsSystems> systems;

    std::unique_ptr<RenderEngine> renderEngine;
    std::unique_ptr<Window> window;

    std::shared_ptr<InputSystem> inputSystem;

    std::shared_ptr<AudioSystem> audioSystem;

    StandardShaders shaders;
    UniformBufferPtr uniformBuffer;

    std::unique_ptr<IScene> pendingScene;
    std::unique_ptr<IScene> activeScene;
    bool systemsInitialized = false;

    std::chrono::system_clock::time_point previousTime;

    bool is_Running = true;
};
