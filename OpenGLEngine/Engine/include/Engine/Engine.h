#pragma once

#include <functional>
#include <memory>
#include <chrono>
#include <string>
#include <utility>
#include <vector>
#include "Audio/AudioSystem.h"
#include "Extension/Extension.h"
#include "Input/InputSystem.h"
#include "Ecs/Core/EcsSystems.h"
#include "Ecs/Systems/EditorHistory.h"
#include "Ecs/Systems/EditorSelection.h"
#include "Save/SaveService.h"
#include "Scene/IScene.h"

class RenderEngine;
class Window;

enum class EngineMode
{
    Editor,
    Play
};

struct StandardShaders
{
    ShaderProgramPtr unlit;
    ShaderProgramPtr lit;
    ShaderProgramPtr litInstanced;
    ShaderProgramPtr skinned;
    ShaderProgramPtr skybox;
    ShaderProgramPtr fire;
    ShaderProgramPtr particle;
    ShaderProgramPtr text;
    ShaderProgramPtr outline;
    ShaderProgramPtr line;
    ShaderProgramPtr hud;
};

class Engine
{
public:
    explicit Engine(EngineMode startMode = EngineMode::Editor);
    virtual ~Engine();

    void Run();
    void Quit();

    void LoadScene(std::unique_ptr<IScene> scene, const std::string& sceneName = "");

    const std::string& GetActiveSceneName() const
    {
        return activeSceneName;
    }

    using SceneFactory = std::function<std::unique_ptr<IScene>()>;
    void RegisterScene(const std::string& name, SceneFactory factory)
    {
        sceneRegistry.emplace_back(name, std::move(factory));
    }
    const std::vector<std::pair<std::string, SceneFactory>>& GetSceneRegistry() const
    {
        return sceneRegistry;
    }
    IScene* GetActiveScene() const
    {
        return activeScene.get();
    }

    EngineMode GetMode() const
    {
        return mode;
    }
    void SetMode(EngineMode newMode);
    bool LaunchPlayInstance();
    void ApplyCursorMode(bool panelOpen);
    bool IsEditorPanelOpen() const
    {
        return editorPanelOpen;
    }
    bool IsGameplayEnabled() const
    {
        return mode == EngineMode::Play;
    }
    class EditorGizmoSystem* GetGizmoSystem() const
    {
        return editorGizmoSystem;
    }
    class EditorCameraSystem* GetEditorCameraSystem() const
    {
        return editorCameraSystem;
    }
    class CullingSystem* GetCullingSystem() const
    {
        return cullingSystem;
    }
    EditorSelection& GetSelection()
    {
        return selection;
    }
    EditorHistory& GetHistory()
    {
        return history;
    }

    unsigned int GetSceneStructureVersion() const
    {
        return sceneStructureVersion;
    }
    void BumpSceneStructureVersion()
    {
        sceneStructureVersion++;
    }

    EcsWorld& GetWorld()
    {
        return world;
    }
    RenderEngine* GetRenderEngine() const
    {
        return renderEngine.get();
    }
    Window* GetWindow() const
    {
        return window.get();
    }
    InputSystem* GetInputSystem() const
    {
        return inputSystem.get();
    }
    AudioSystem* GetAudioSystem() const
    {
        return audioSystem.get();
    }
    SaveService* GetSaveService() const
    {
        return saveService.get();
    }
    const StandardShaders& GetShaders() const
    {
        return shaders;
    }
    UniformBufferPtr GetUniformBuffer() const
    {
        return uniformBuffer;
    }

private:
    void OnUpdateInternal();
    void ThrottleWhenUnfocused() const;
    void CreateStandardShaders();
    void CreateStandardSystems();
    void ApplyMode();

protected:
    virtual void OnCreate();
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnQuit();

    EcsWorld world;

    std::unique_ptr<EcsSystems> systems;
    std::unique_ptr<RenderEngine> renderEngine;
    std::unique_ptr<Window> window;

    std::unique_ptr<IScene> pendingScene;
    std::unique_ptr<IScene> activeScene;

    std::vector<std::pair<std::string, SceneFactory>> sceneRegistry;
    std::string activeSceneName;

    std::shared_ptr<InputSystem> inputSystem;
    std::shared_ptr<AudioSystem> audioSystem;
    std::unique_ptr<SaveService> saveService;

    StandardShaders shaders;
    UniformBufferPtr uniformBuffer;

    std::chrono::system_clock::time_point previousTime;

    EngineMode mode = EngineMode::Editor;

    class EditorCameraSystem* editorCameraSystem = nullptr;
    class EditorGizmoSystem* editorGizmoSystem = nullptr;
    class EditorPickSystem* editorPickSystem = nullptr;
    class CullingSystem* cullingSystem = nullptr;
    EditorSelection selection;
    EditorHistory history;
    unsigned int sceneStructureVersion = 0;
    bool editorPanelOpen = true;

    static constexpr int unfocusedFrameDelayMs = 30;

public:
    bool vsyncEnabled = true;

private:

    bool systemsInitialized = false;
    bool is_Running = true;
};
