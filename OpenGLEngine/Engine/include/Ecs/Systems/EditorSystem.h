#pragma once

#include <imgui.h>
#include <string>
#include "Ecs/Core/IEcsSystem.h"
#include "Editor/EditorCommands.h"
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
        HandleShortcuts(world);

        if (engine->IsEditorPanelOpen())
        {
            DrawMainPanel(world);
            hierarchyPanel.Draw(*engine, world);
        }

        DrawUnsavedChangesPopup();
    }

private:
    void UpdatePanelToggle()
    {
        const bool f1Down = inputSystem->IsKeyDown(Key::F1);
        if (f1Down && !f1WasDown)
            engine->ApplyCursorMode(!engine->IsEditorPanelOpen());
        f1WasDown = f1Down;
    }

    void HandleShortcuts(EcsWorld& world)
    {
        const auto& io = ImGui::GetIO();
        if (io.WantTextInput)
            return;

        auto& selection = engine->GetSelection();

        if (io.KeyCtrl)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
            {
                if (io.KeyShift)
                    EditorCommands::Redo(*engine, world);
                else
                    EditorCommands::Undo(*engine, world);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
                EditorCommands::Redo(*engine, world);
            if (ImGui::IsKeyPressed(ImGuiKey_D, false))
                EditorCommands::DuplicateSelection(*engine, world);
            if (ImGui::IsKeyPressed(ImGuiKey_S, false))
                EditorCommands::SaveScene(*engine);
            return;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
            EditorCommands::DeleteSelection(*engine, world);
        if (ImGui::IsKeyPressed(ImGuiKey_F, false))
            EditorCommands::FocusSelection(*engine, world);

        if (ImGui::IsKeyPressed(ImGuiKey_W, false))
            selection.mode = GizmoMode::Move;
        if (ImGui::IsKeyPressed(ImGuiKey_E, false))
            selection.mode = GizmoMode::Rotate;
        if (ImGui::IsKeyPressed(ImGuiKey_R, false))
            selection.mode = GizmoMode::Scale;
    }

    void RequestSceneSwitch(const Engine::SceneEntry& entry)
    {
        if (!engine->GetHistory().IsDirty())
        {
            engine->LoadScene(entry.factory(), entry.name);
            return;
        }

        pendingSceneName = entry.name;
        openUnsavedPopup = true;
    }

    void DrawUnsavedChangesPopup()
    {
        if (openUnsavedPopup)
        {
            ImGui::OpenPopup("Unsaved changes");
            openUnsavedPopup = false;
        }

        if (!ImGui::BeginPopupModal("Unsaved changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;

        ImGui::Text("Scene '%s' has unsaved changes.", engine->GetActiveSceneName().c_str());

        const Engine::SceneEntry* entry = engine->FindSceneEntry(pendingSceneName);
        bool close = entry == nullptr;

        if (ImGui::Button("Save and switch") && entry)
        {
            EditorCommands::SaveScene(*engine);
            engine->LoadScene(entry->factory(), entry->name);
            close = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard") && entry)
        {
            engine->LoadScene(entry->factory(), entry->name);
            close = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            close = true;

        if (close)
        {
            pendingSceneName.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
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
            lightsPanel.DrawLights(*engine, world);

        if (ImGui::CollapsingHeader("Ambient Light"))
            lightsPanel.DrawAmbient(*engine, world);

        ImGui::End();
    }

    void DrawSceneList()
    {
        for (const auto& entry : engine->GetSceneRegistry())
        {
            if (ImGui::Button(entry.name.c_str()))
                RequestSceneSwitch(entry);

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
    std::string pendingSceneName;
    bool openUnsavedPopup = false;
};
