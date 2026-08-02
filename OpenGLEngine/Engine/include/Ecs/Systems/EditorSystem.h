#pragma once

#include <imgui.h>
#include "Ecs/Core/IEcsSystem.h"
#include "Editor/Panels/AssetBrowserPanel.h"
#include "Editor/Panels/CameraPanel.h"
#include "Editor/Panels/HierarchyPanel.h"
#include "Editor/Panels/InspectorPanel.h"
#include "Editor/Panels/LightsPanel.h"
#include "Editor/Panels/ToolbarPanel.h"
#include "Engine/Engine.h"
#include "Input/InputSystem.h"
#include "Window/Window.h"

class EditorSystem : public IEcsSystem
{
public:
    EditorSystem(InputSystem* inputSystem, Window* window, Engine* engine) : inputSystem(inputSystem), window(window), engine(engine) {}

    void Run(EcsWorld& world, float deltaTime) override
    {
        UpdatePanelToggle();

        if (!engine->IsEditorPanelOpen())
            return;

        DrawMainPanel(world);
        hierarchyPanel.Draw(*engine, world);
    }

private:
    void UpdatePanelToggle()
    {
        const bool f1Down = inputSystem->IsKeyDown(Key::F1);
        if (f1Down && !f1WasDown)
            engine->ApplyCursorMode(!engine->IsEditorPanelOpen());
        f1WasDown = f1Down;
    }

    void DrawMainPanel(EcsWorld& world)
    {
        ImGui::Begin("Scene Editor");

        toolbarPanel.Draw(*engine, world);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Scenes", ImGuiTreeNodeFlags_DefaultOpen))
            DrawSceneList();

        if (ImGui::CollapsingHeader("Assets", ImGuiTreeNodeFlags_DefaultOpen))
            assetBrowserPanel.Draw(*engine, world);

        if (ImGui::CollapsingHeader("Camera"))
            cameraPanel.Draw(*engine, world);

        if (ImGui::CollapsingHeader("Selection", ImGuiTreeNodeFlags_DefaultOpen))
            inspectorPanel.DrawSelection(*engine, world);

        if (ImGui::CollapsingHeader("Scene Debug", ImGuiTreeNodeFlags_DefaultOpen))
            DrawSceneDebug();

        if (ImGui::CollapsingHeader("Transforms", ImGuiTreeNodeFlags_DefaultOpen))
            inspectorPanel.DrawTransforms(*engine, world);

        if (ImGui::CollapsingHeader("Lights"))
            lightsPanel.DrawLights(world);

        if (ImGui::CollapsingHeader("Ambient Light"))
            lightsPanel.DrawAmbient(world);

        ImGui::End();
    }

    void DrawSceneList()
    {
        for (const auto& entry : engine->GetSceneRegistry())
        {
            if (ImGui::Button(entry.name.c_str()))
                engine->LoadScene(entry.factory(), entry.name);

            if (!entry.ownerName.empty())
            {
                ImGui::SameLine();
                ImGui::TextDisabled("%s", entry.ownerName.c_str());
            }
        }
    }

    void DrawSceneDebug()
    {
        IScene* activeScene = engine->GetActiveScene();
        if (activeScene)
            activeScene->OnDebugUI(*engine);
        else
            ImGui::TextUnformatted("No active scene");
    }

    InputSystem* inputSystem;
    Window* window;
    Engine* engine;

    ToolbarPanel toolbarPanel;
    AssetBrowserPanel assetBrowserPanel;
    CameraPanel cameraPanel;
    InspectorPanel inspectorPanel;
    HierarchyPanel hierarchyPanel;
    LightsPanel lightsPanel;

    bool f1WasDown = false;
};
