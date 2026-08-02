#include <chrono>
#include <cstdio>
#include <thread>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "Window/Window.h"
#include "Engine/Engine.h"
#include "Platform/ProcessLauncher.h"
#include "Math/Vector4.h"
#include "Render/RenderEngine.h"
#include "Render/UniformData.h"
#include "Resource/SceneSerializer.h"
#include "Ecs/Systems/AnimatorSystem.h"
#include "Ecs/Systems/AutoOrbitSystem.h"
#include "Ecs/Systems/CameraInputSystem.h"
#include "Ecs/Systems/CameraMatrixSystem.h"
#include "Ecs/Systems/BoundsSystem.h"
#include "Ecs/Systems/CullingSystem.h"
#include "Ecs/Systems/EditorCameraSystem.h"
#include "Ecs/Systems/EditorGizmoSystem.h"
#include "Ecs/Systems/EditorPickSystem.h"
#include "Ecs/Systems/EditorSystem.h"
#include "Ecs/Systems/FpsCounterSystem.h"
#include "Ecs/Systems/HitFlashSystem.h"
#include "Ecs/Systems/HudSystem.h"
#include "Ecs/Systems/HoverSystem.h"
#include "Ecs/Systems/ParticleSystem.h"
#include "Ecs/Systems/ProjectileSystem.h"
#include "Ecs/Systems/PendulumSystem.h"
#include "Ecs/Systems/RenderSystem.h"
#include "Ecs/Systems/RotatorSystem.h"
#include "Ecs/Systems/SimplePhysicSystem.h"
#include "Ecs/Systems/SkyboxSystem.h"
#include "Ecs/Systems/TransformHierarchySystem.h"
#include "Ecs/Systems/UITextSystem.h"

Engine::Engine(EngineMode startMode) : mode(startMode)
{
    const bool editing = mode == EngineMode::Editor;

    vsyncEnabled = false;
    frameCapEnabled = (mode == EngineMode::Editor);

    window = std::make_unique<Window>(editing, editing ? "OpenGLEngine Editor" : "OpenGLEngine");
    window->SetVSync(vsyncEnabled);
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

    shaders.skinned = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Skinned.vert", "Assets/Shaders/SimpleShader_Lit.frag"});
    shaders.skinned->SetUniformBufferSlot("UniformData", 0);

    shaders.skybox = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Skybox.vert", "Assets/Shaders/SimpleShader_Skybox.frag"});
    shaders.skybox->SetUniformBufferSlot("UniformData", 0);

    shaders.fire = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Fire.vert", "Assets/Shaders/SimpleShader_Fire.frag"});
    shaders.fire->SetUniformBufferSlot("UniformData", 0);

    shaders.particle = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Particle.vert", "Assets/Shaders/SimpleShader_Particle.frag"});
    shaders.particle->SetUniformBufferSlot("UniformData", 0);

    shaders.text = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Text.vert", "Assets/Shaders/SimpleShader_Text.frag"});

    shaders.outline = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Outline.vert", "Assets/Shaders/SimpleShader_Outline.frag"});
    shaders.outline->SetUniformBufferSlot("UniformData", 0);

    shaders.line = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Line.vert", "Assets/Shaders/SimpleShader_Line.frag"});
    shaders.line->SetUniformBufferSlot("UniformData", 0);

    shaders.hud = renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Hud.vert", "Assets/Shaders/SimpleShader_Hud.frag"});
}

void Engine::SetMode(EngineMode newMode)
{
    mode = newMode;
    ApplyMode();
}

bool Engine::LaunchPlayInstance()
{
    if (activeSceneName.empty())
    {
        OGL_WARNING("Engine | Cannot launch play instance: scene has no name")
        return false;
    }

    SceneSerializer::Save(*this, activeSceneName);

    const std::string exePath = ProcessLauncher::GetExecutablePath();
    if (exePath.empty())
    {
        OGL_WARNING("Engine | Cannot resolve executable path")
        return false;
    }

    if (!ProcessLauncher::Launch("\"" + exePath + "\" --play --scene=" + activeSceneName))
    {
        OGL_WARNING("Engine | Failed to launch play instance")
        return false;
    }

    OGL_INFO("Engine | Launched play instance for scene: " << activeSceneName)
    return true;
}

void Engine::ApplyCursorMode(bool panelOpen)
{
    editorPanelOpen = panelOpen;

    const bool wantsLock = mode == EngineMode::Play && !editorPanelOpen && activeScene && activeScene->WantsCursorLocked();
    window->SetCursorLocked(wantsLock);
}

void Engine::ApplyMode()
{
    const bool editing = mode == EngineMode::Editor;

    if (editorCameraSystem)
        editorCameraSystem->enabled = editing;
    if (editorGizmoSystem)
        editorGizmoSystem->enabled = editing;
    if (editorPickSystem)
        editorPickSystem->enabled = editing;

    if (!editing)
        selection.Clear();

    audioSystem->SetMuted(editing);

    ApplyCursorMode(editing);
}

void Engine::CreateStandardSystems()
{
    const bool editing = mode == EngineMode::Editor;

    systems = std::make_unique<EcsSystems>(world);
    systems->Add(std::make_unique<AutoOrbitSystem>(inputSystem.get()), true);
    systems->Add(std::make_unique<CameraInputSystem>(inputSystem.get()), true);
    systems->Add(std::make_unique<SimplePhysicSystem>(), true);
    systems->Add(std::make_unique<ProjectileSystem>(), true);
    systems->Add(std::make_unique<HitFlashSystem>(), true);

    if (editing)
    {
        auto editorCamera = std::make_unique<EditorCameraSystem>(inputSystem.get());
        editorCameraSystem = editorCamera.get();
        systems->Add(std::move(editorCamera));

        auto picking = std::make_unique<EditorPickSystem>(inputSystem.get(), window.get(), &selection, &history);
        editorPickSystem = picking.get();
        systems->Add(std::move(picking));
    }

    systems->Add(std::make_unique<CameraMatrixSystem>(renderEngine.get(), uniformBuffer, window.get()));
    systems->Add(std::make_unique<SkyboxSystem>(renderEngine.get()));
    systems->Add(std::make_unique<HoverSystem>(inputSystem.get(), window.get()), true);
    systems->Add(std::make_unique<AnimatorSystem>());
    systems->Add(std::make_unique<TransformHierarchySystem>());
    systems->Add(std::make_unique<BoundsSystem>());

    auto culling = std::make_unique<CullingSystem>();
    cullingSystem = culling.get();
    systems->Add(std::move(culling));

    systems->Add(std::make_unique<RenderSystem>(renderEngine.get(), uniformBuffer, shaders.outline, shaders.litInstanced));
    systems->Add(std::make_unique<ParticleSystem>(renderEngine.get(), shaders.particle, uniformBuffer));
    systems->Add(std::make_unique<RotatorSystem>(), true);
    systems->Add(std::make_unique<PendulumSystem>(), true);

    if (editing)
    {
        auto gizmos = std::make_unique<EditorGizmoSystem>(renderEngine.get(), uniformBuffer, shaders.line, &selection);
        editorGizmoSystem = gizmos.get();
        systems->Add(std::move(gizmos));
    }

    systems->Add(std::make_unique<FpsCounterSystem>(window.get()));
    systems->Add(std::make_unique<UITextSystem>(renderEngine.get(), shaders.text, window.get()));
    systems->Add(std::make_unique<HudSystem>(renderEngine.get(), shaders.hud, window.get()), true);

    if (editing)
        systems->Add(std::make_unique<EditorSystem>(inputSystem.get(), window.get(), this));

    systems->Init();

    ApplyMode();
}

void Engine::OnCreate()
{
    if (!audioSystem->Init())
        std::fprintf(stderr, "Engine: AudioSystem::Init failed, continuing without audio\n");
    renderEngine->SetViewPort(window->GetInnerSize());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window->GetGLFWWindow(), false);
    ImGui_ImplOpenGL3_Init("#version 400");
    glfwSetScrollCallback(window->GetGLFWWindow(), ImGui_ImplGlfw_ScrollCallback);
    glfwSetCharCallback(window->GetGLFWWindow(), ImGui_ImplGlfw_CharCallback);

    CreateStandardShaders();
    CreateStandardSystems();

    systemsInitialized = true;

    if (pendingScene)
    {
        activeScene = std::move(pendingScene);
        activeScene->OnLoad(*this);

        if (!activeSceneName.empty())
            SceneSerializer::Load(*this, activeSceneName);
    }

    ApplyCursorMode(mode == EngineMode::Editor);
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
    const auto frameStart = std::chrono::high_resolution_clock::now();

    window->PollEvents();
    renderEngine->SetViewPort(window->GetInnerSize());

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const auto currentTime = std::chrono::system_clock::now();
    auto elapsedSeconds = std::chrono::duration<double>();
    if (previousTime.time_since_epoch().count())
        elapsedSeconds = currentTime - previousTime;
    previousTime = currentTime;

    const auto deltaTime = static_cast<float>(elapsedSeconds.count());

    audioSystem->Update();

    const Vector4 clearColor = activeScene ? activeScene->GetClearColor() : Vector4(0, 0, 0, 1);
    renderEngine->Clear(clearColor);
    renderEngine->SetFaceCulling(CullingType::Both);
    renderEngine->SetWindingOrder(ClockWise);

    {
        const auto perfStart = std::chrono::high_resolution_clock::now();
        systems->Update(deltaTime, IsGameplayEnabled());
        const auto perfMid = std::chrono::high_resolution_clock::now();
        const double systemsMs = std::chrono::duration_cast<std::chrono::microseconds>(perfMid - perfStart).count() / 1000.0;
        OGL_INFO("Perf | systems->Update: " << systemsMs << " ms");

        if (activeScene)
        {
            if (IsGameplayEnabled())
                activeScene->OnUpdate(*this, deltaTime);
            else
                activeScene->OnEditorUpdate(*this, deltaTime);
        }

        OnUpdate(deltaTime);
        inputSystem->Update();

        const auto perfEnd = std::chrono::high_resolution_clock::now();
        const double frameAfterSceneMs = std::chrono::duration_cast<std::chrono::microseconds>(perfEnd - perfMid).count() / 1000.0;
        OGL_INFO("Perf | scene+input: " << frameAfterSceneMs << " ms");
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    window->Present();

    const bool editorIdle = mode == EngineMode::Editor && !window->IsFocused();
    if (frameCapEnabled && frameCap > 0 && !editorIdle)
        WaitForFrameDeadline(frameStart);

    ThrottleWhenUnfocused();
}

void Engine::WaitForFrameDeadline(const std::chrono::high_resolution_clock::time_point& frameStart) const
{
    using namespace std::chrono;

    const duration<double, std::milli> target(1000.0 / static_cast<double>(frameCap));
    const auto deadline = frameStart + duration_cast<high_resolution_clock::duration>(target);

    const auto coarseDeadline = deadline - milliseconds(spinMarginMs);
    if (high_resolution_clock::now() < coarseDeadline)
        std::this_thread::sleep_until(coarseDeadline);

    while (high_resolution_clock::now() < deadline)
        std::this_thread::yield();
}

void Engine::ThrottleWhenUnfocused() const
{
    if (mode != EngineMode::Editor || window->IsFocused())
        return;

    std::this_thread::sleep_for(std::chrono::milliseconds(unfocusedFrameDelayMs));
}

void Engine::OnQuit()
{
    if (activeScene)
        activeScene->OnUnload(*this);
    audioSystem->Shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Engine::Quit()
{
    is_Running = false;
}

void Engine::LoadScene(std::unique_ptr<IScene> scene, const std::string& sceneName)
{
    activeSceneName = sceneName;

    if (systemsInitialized)
    {
        if (activeScene)
            activeScene->OnUnload(*this);
        audioSystem->StopAll();
        selection.Clear();
        history.Clear();
        world.Clear();
        BumpSceneStructureVersion();
        activeScene = std::move(scene);
        activeScene->OnLoad(*this);

        if (!activeSceneName.empty())
            SceneSerializer::Load(*this, activeSceneName);
    }
    else
    {
        pendingScene = std::move(scene);
    }
}
