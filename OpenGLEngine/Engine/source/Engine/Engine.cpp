#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>
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
#include "Ecs/Components/PostProcessComponent.h"
#include "Render/PostProcessPipeline.h"
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
#include "Ecs/Systems/HoverSystem.h"
#include "Ecs/Systems/ParticleSystem.h"
#include "Ecs/Systems/ProjectileSystem.h"
#include "Ecs/Systems/PendulumSystem.h"
#include "Ecs/Systems/RenderSystem.h"
#include "Ecs/Systems/RotatorSystem.h"
#include "Ecs/Systems/ShadowMapSystem.h"
#include "Ecs/Systems/SimplePhysicSystem.h"
#include "Ecs/Systems/ProceduralSkySystem.h"
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

Engine::~Engine()
{
}

void Engine::CreateStandardShaders()
{
    uniformBuffer = renderEngine->CreateUniformBuffer({sizeof(UniformData)});

    shaders.unlit = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_Unlit.vert", "Assets/Shaders/SimpleShader_Unlit.frag"
    });
    shaders.unlit->SetUniformBufferSlot("UniformData", 0);

    shaders.lit = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_Lit.vert", "Assets/Shaders/SimpleShader_Lit.frag"
    });
    shaders.lit->SetUniformBufferSlot("UniformData", 0);
    shaders.lit->SetUniformBufferSlot("FrameData", FrameUniformBufferSlot);

    shaders.litInstanced = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_LitInstanced.vert", "Assets/Shaders/SimpleShader_LitInstanced.frag"
    });
    shaders.litInstanced->SetUniformBufferSlot("UniformData", 0);
    shaders.litInstanced->SetUniformBufferSlot("FrameData", FrameUniformBufferSlot);

    shaders.skinned = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_Skinned.vert", "Assets/Shaders/SimpleShader_Lit.frag"
    });
    shaders.skinned->SetUniformBufferSlot("UniformData", 0);
    shaders.skinned->SetUniformBufferSlot("FrameData", FrameUniformBufferSlot);

    shaders.skybox = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_Skybox.vert", "Assets/Shaders/SimpleShader_Skybox.frag"
    });
    shaders.skybox->SetUniformBufferSlot("UniformData", 0);

    shaders.skyProcedural = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_Skybox.vert", "Assets/Shaders/SimpleShader_ProceduralSky.frag"
    });
    shaders.skyProcedural->SetUniformBufferSlot("UniformData", 0);

    shaders.fire = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_Fire.vert", "Assets/Shaders/SimpleShader_Fire.frag"
    });
    shaders.fire->SetUniformBufferSlot("UniformData", 0);

    shaders.particle = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_Particle.vert", "Assets/Shaders/SimpleShader_Particle.frag"
    });
    shaders.particle->SetUniformBufferSlot("UniformData", 0);

    shaders.text = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_Text.vert", "Assets/Shaders/SimpleShader_Text.frag"
    });

    shaders.outline = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_Outline.vert", "Assets/Shaders/SimpleShader_Outline.frag"
    });
    shaders.outline->SetUniformBufferSlot("UniformData", 0);

    shaders.line = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_Line.vert", "Assets/Shaders/SimpleShader_Line.frag"
    });
    shaders.line->SetUniformBufferSlot("UniformData", 0);

    shaders.hud = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_Hud.vert", "Assets/Shaders/SimpleShader_Hud.frag"
    });

    shaders.shadowDepth = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_ShadowDepth.vert", "Assets/Shaders/SimpleShader_ShadowDepth.frag"
    });
    shaders.shadowDepthSkinned = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_ShadowDepthSkinned.vert", "Assets/Shaders/SimpleShader_ShadowDepth.frag"
    });
    shaders.shadowDepth->SetUniformBufferSlot("UniformData", 0);
    shaders.shadowDepthSkinned->SetUniformBufferSlot("UniformData", 0);

    shaders.postProcess = renderEngine->CreateShaderProgram({
        "Assets/Shaders/SimpleShader_PostProcess.vert", "Assets/Shaders/SimpleShader_PostProcess.frag"
    });

    postProcessPipeline = std::make_unique<PostProcessPipeline>(renderEngine.get(), shaders.postProcess);
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

    std::error_code ec;
    const std::filesystem::path playScenePath = std::filesystem::temp_directory_path(ec) / "OpenGLEngine" / (activeSceneName + ".play.scene");
    if (ec || !SceneSerializer::SaveToFile(*this, playScenePath.string()))
    {
        OGL_WARNING("Engine | Cannot write play scene snapshot")
        return false;
    }

    const std::string exePath = ProcessLauncher::GetExecutablePath();
    if (exePath.empty())
    {
        OGL_WARNING("Engine | Cannot resolve executable path")
        return false;
    }

    if (!ProcessLauncher::Launch("\"" + exePath + "\" --play --scene=" + activeSceneName + " \"--scene-file=" + playScenePath.string() + "\""))
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

    const bool wantsLock = mode == EngineMode::Play && !editorPanelOpen && activeScene && activeScene->
        WantsCursorLocked();
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
    systems->Add(std::make_unique<AutoOrbitSystem>(inputSystem.get()), SystemPhase::Update, true);
    systems->Add(std::make_unique<CameraInputSystem>(inputSystem.get()), SystemPhase::Update, true);
    systems->Add(std::make_unique<SimplePhysicSystem>(), SystemPhase::Update, true);
    systems->Add(std::make_unique<ProjectileSystem>(), SystemPhase::Update, true);
    systems->Add(std::make_unique<HitFlashSystem>(), SystemPhase::Update, true);

    if (editing)
    {
        auto editorCamera = std::make_unique<EditorCameraSystem>(inputSystem.get());
        editorCameraSystem = editorCamera.get();
        systems->Add(std::move(editorCamera), SystemPhase::Update);

        auto picking = std::make_unique<EditorPickSystem>(inputSystem.get(), window.get(), &selection, &history);
        editorPickSystem = picking.get();
        systems->Add(std::move(picking), SystemPhase::Update);
    }

    systems->Add(std::make_unique<RotatorSystem>(), SystemPhase::Update, true);
    systems->Add(std::make_unique<PendulumSystem>(), SystemPhase::Update, true);

    systems->Add(std::make_unique<CameraMatrixSystem>(renderEngine.get(), uniformBuffer, window.get()), SystemPhase::Render);
    systems->Add(std::make_unique<SkyboxSystem>(renderEngine.get()), SystemPhase::Render);
    systems->Add(std::make_unique<ProceduralSkySystem>(renderEngine.get()), SystemPhase::Render);
    systems->Add(std::make_unique<HoverSystem>(inputSystem.get(), window.get()), SystemPhase::Render, true);
    systems->Add(std::make_unique<AnimatorSystem>(), SystemPhase::Render);
    systems->Add(std::make_unique<TransformHierarchySystem>(), SystemPhase::Render);
    systems->Add(std::make_unique<BoundsSystem>(), SystemPhase::Render);

    auto culling = std::make_unique<CullingSystem>();
    cullingSystem = culling.get();
    systems->Add(std::move(culling), SystemPhase::Render);

    auto shadowMap = std::make_unique<ShadowMapSystem>(renderEngine.get(), window.get(), uniformBuffer, shaders.shadowDepth,
                                                       shaders.shadowDepthSkinned);
    shadowMapSystem = shadowMap.get();
    systems->Add(std::move(shadowMap), SystemPhase::Render);

    systems->Add(std::make_unique<RenderSystem>(renderEngine.get(), uniformBuffer, shaders.outline,
                                                shaders.litInstanced, shadowMapSystem), SystemPhase::Render);
    systems->Add(std::make_unique<ParticleSystem>(renderEngine.get(), shaders.particle, uniformBuffer), SystemPhase::Render);

    if (editing)
    {
        auto gizmos = std::make_unique<EditorGizmoSystem>(renderEngine.get(), uniformBuffer, shaders.line, &selection);
        editorGizmoSystem = gizmos.get();
        systems->Add(std::move(gizmos), SystemPhase::Render);
    }

    auto fpsCounter = std::make_unique<FpsCounterSystem>(window.get());
    fpsCounterSystem = fpsCounter.get();
    systems->Add(std::move(fpsCounter), SystemPhase::Render);
    systems->Add(std::make_unique<UITextSystem>(renderEngine.get(), shaders.text, window.get()), SystemPhase::Overlay);

    if (editing)
        systems->Add(std::make_unique<EditorSystem>(inputSystem.get(), window.get(), this), SystemPhase::Render);

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
    ApplyQueuedScene();
    const Rect windowSize = window->GetInnerSize();
    renderEngine->SetViewPort(windowSize);

    if (postProcessEnabled && postProcessPipeline)
        postProcessPipeline->BeginScene(windowSize.width, windowSize.height);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const auto currentTime = std::chrono::steady_clock::now();
    float deltaTime = 0.0f;
    if (hasPreviousTime)
        deltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
    previousTime = currentTime;
    hasPreviousTime = true;

    deltaTime = (std::min)(deltaTime, maxDeltaTime);

    audioSystem->Update();

    const bool gameplayEnabled = IsGameplayEnabled();

    systems->Update(SystemPhase::Update, deltaTime, gameplayEnabled);

    if (activeScene)
    {
        if (gameplayEnabled)
            activeScene->OnUpdate(*this, deltaTime);
        else
            activeScene->OnEditorUpdate(*this, deltaTime);
    }

    OnUpdate(deltaTime);
    world.ProcessDeferred();

    const Vector4 clearColor = activeScene ? activeScene->GetClearColor() : Vector4(0, 0, 0, 1);
    renderEngine->Clear(clearColor);
    renderEngine->SetFaceCulling(CullingType::Both);
    renderEngine->SetWindingOrder(ClockWise);

    systems->Update(SystemPhase::Render, deltaTime, gameplayEnabled);

    inputSystem->Update();

    if (postProcessEnabled && postProcessPipeline)
    {
        PostProcessComponent post;
        for (auto& pair : world.GetPool<PostProcessComponent>())
        {
            post = pair.second;
            break;
        }

        postProcessPipeline->Resolve(post);
    }

    systems->Update(SystemPhase::Overlay, deltaTime, gameplayEnabled);

    if (activeScene)
        activeScene->OnRender(*this);

    frameIndex++;
    if (!screenshotPath.empty() && frameIndex == screenshotFrame)
    {
        CaptureScreenshot(windowSize.width, windowSize.height);
        if (screenshotQuits)
            Quit();
        screenshotPath.clear();
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
    if (systemsInitialized)
    {
        queuedScene = std::move(scene);
        queuedSceneName = sceneName;
        return;
    }

    activeSceneName = sceneName;
    pendingScene = std::move(scene);
}

void Engine::CaptureScreenshot(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;

    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    std::ofstream file(screenshotPath, std::ios::binary);
    if (!file.is_open())
    {
        OGL_WARNING("Engine | Cannot write screenshot: " << screenshotPath)
        return;
    }

    file << "P6\n" << width << " " << height << "\n255\n";
    for (int y = height - 1; y >= 0; y--)
        file.write(reinterpret_cast<const char*>(pixels.data() + static_cast<size_t>(y) * width * 3), static_cast<std::streamsize>(width) * 3);

    OGL_INFO("Engine | Screenshot saved: " << screenshotPath)
}

void Engine::ApplyQueuedScene()
{
    if (!queuedScene)
        return;

    if (activeScene)
        activeScene->OnUnload(*this);
    audioSystem->StopAll();
    selection.Clear();
    history.Clear();
    world.Clear();
    BumpSceneStructureVersion();

    activeSceneName = queuedSceneName;
    activeScene = std::move(queuedScene);
    queuedSceneName.clear();
    sceneFileOverride.clear();

    activeScene->OnLoad(*this);

    if (!activeSceneName.empty())
        SceneSerializer::Load(*this, activeSceneName);

    ApplyCursorMode(editorPanelOpen);
}
