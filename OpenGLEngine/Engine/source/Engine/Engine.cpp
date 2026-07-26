#include <chrono>
#include <cstdio>
#include "Window/Window.h"
#include "Engine/Engine.h"
#include "Math/Vector4.h"
#include "Render/RenderEngine.h"
#include "Render/UniformData.h"
#include "Ecs/Systems/AutoOrbitSystem.h"
#include "Ecs/Systems/CameraInputSystem.h"
#include "Ecs/Systems/CameraMatrixSystem.h"
#include "Ecs/Systems/FpsCounterSystem.h"
#include "Ecs/Systems/HoverSystem.h"
#include "Ecs/Systems/ParticleSystem.h"
#include "Ecs/Systems/PendulumSystem.h"
#include "Ecs/Systems/RenderSystem.h"
#include "Ecs/Systems/RotatorSystem.h"
#include "Ecs/Systems/SimplePhysicSystem.h"
#include "Ecs/Systems/SkyboxSystem.h"
#include "Ecs/Systems/UITextSystem.h"

Engine::Engine()
{
    window = std::make_unique<Window>();
    renderEngine = std::make_unique<RenderEngine>();
    inputSystem = std::make_shared<InputSystem>(window->GetGLFWWindow());
    audioSystem = std::make_shared<AudioSystem>();
    saveService = std::make_unique<SaveService>("save.txt");
}

Engine::~Engine() {}

void Engine::CreateStandardShaders()
{
    uniformBuffer = renderEngine->CreateUniformBuffer({sizeof(UniformData)});

    shaders.unlit = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Unlit.vert", "Assets/Shaders/SimpleShader_Unlit.frag"});
    shaders.unlit->SetUniformBufferSlot("UniformData", 0);

    shaders.lit = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Lit.vert", "Assets/Shaders/SimpleShader_Lit.frag"});
    shaders.lit->SetUniformBufferSlot("UniformData", 0);

    shaders.litInstanced = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_LitInstanced.vert", "Assets/Shaders/SimpleShader_LitInstanced.frag"});
    shaders.litInstanced->SetUniformBufferSlot("UniformData", 0);

    shaders.skybox = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Skybox.vert", "Assets/Shaders/SimpleShader_Skybox.frag"});
    shaders.skybox->SetUniformBufferSlot("UniformData", 0);

    shaders.fire = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Fire.vert", "Assets/Shaders/SimpleShader_Fire.frag"});
    shaders.fire->SetUniformBufferSlot("UniformData", 0);

    shaders.particle = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Particle.vert", "Assets/Shaders/SimpleShader_Particle.frag"});
    shaders.particle->SetUniformBufferSlot("UniformData", 0);

    shaders.text = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Text.vert", "Assets/Shaders/SimpleShader_Text.frag"});

    shaders.outline = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Outline.vert", "Assets/Shaders/SimpleShader_Outline.frag"});
    shaders.outline->SetUniformBufferSlot("UniformData", 0);
}

void Engine::CreateStandardSystems()
{
    systems = std::make_unique<EcsSystems>(world);
    systems->Add(std::make_unique<AutoOrbitSystem>(inputSystem.get()));
    systems->Add(std::make_unique<CameraInputSystem>(inputSystem.get()));
    systems->Add(std::make_unique<SimplePhysicSystem>());
    systems->Add(std::make_unique<CameraMatrixSystem>(renderEngine.get(), uniformBuffer, window.get()));
    systems->Add(std::make_unique<SkyboxSystem>(renderEngine.get()));
    systems->Add(std::make_unique<HoverSystem>(inputSystem.get(), window.get()));
    systems->Add(std::make_unique<RenderSystem>(renderEngine.get(), uniformBuffer, shaders.outline, shaders.litInstanced));
    systems->Add(std::make_unique<ParticleSystem>(renderEngine.get(), shaders.particle, uniformBuffer));
    systems->Add(std::make_unique<RotatorSystem>());
    systems->Add(std::make_unique<PendulumSystem>());
    systems->Add(std::make_unique<FpsCounterSystem>());
    systems->Add(std::make_unique<UITextSystem>(renderEngine.get(), shaders.text, window.get()));
    systems->Init();
}

void Engine::OnCreate()
{
    if (!audioSystem->Init())
        std::fprintf(stderr, "Engine: AudioSystem::Init failed, continuing without audio\n");
    renderEngine->SetViewPort(window->GetInnerSize());

    CreateStandardShaders();
    CreateStandardSystems();

    systemsInitialized = true;

    if (pendingScene)
    {
        activeScene = std::move(pendingScene);
        activeScene->OnLoad(*this);
    }
}

void Engine::Run()
{
    OnCreate();
    while (is_Running && !window->ShouldClose())
    {
        OnUpdateInternal();
    }
    OnQuit();
}

void Engine::OnUpdateInternal()
{
    window->PollEvents();

    const auto currentTime = std::chrono::system_clock::now();
    auto elapsedSeconds = std::chrono::duration<double>();
    if (previousTime.time_since_epoch().count())
        elapsedSeconds = currentTime - previousTime;
    previousTime = currentTime;

    const auto deltaTime = static_cast<float>(elapsedSeconds.count());

    audioSystem->Update();

    const Vector4 clearColor = activeScene ? activeScene->GetClearColor() : Vector4(0, 0, 0, 1);
    renderEngine->Clear(clearColor);
    renderEngine->SetFaceCulling(CullingType::BackFace);
    renderEngine->SetWindingOrder(ClockWise);

    systems->Update(deltaTime);
    if (activeScene)
        activeScene->OnUpdate(*this, deltaTime);
    OnUpdate(deltaTime);
    inputSystem->Update();

    window->Present(false);
}

void Engine::OnQuit()
{
    if (activeScene)
        activeScene->OnUnload(*this);
    audioSystem->Shutdown();
}

void Engine::Quit()
{
    is_Running = false;
}

void Engine::LoadScene(std::unique_ptr<IScene> scene)
{
    if (systemsInitialized)
    {
        if (activeScene)
            activeScene->OnUnload(*this);
        activeScene = std::move(scene);
        activeScene->OnLoad(*this);
    }
    else
    {
        pendingScene = std::move(scene);
    }
}
