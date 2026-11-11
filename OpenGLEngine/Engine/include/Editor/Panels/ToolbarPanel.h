#pragma once

#include <imgui.h>
#include "Ecs/Core/EcsWorld.h"
#include "Ecs/Systems/CullingSystem.h"
#include "Ecs/Systems/EditorGizmoSystem.h"
#include "Editor/EditorCommands.h"
#include "Engine/Engine.h"

class ToolbarPanel
{
public:
    void Draw(Engine& engine, EcsWorld& world)
    {
        const bool dirty = engine.GetHistory().IsDirty();
        if (ImGui::Button(dirty ? "Save*###save" : "Save###save"))
            EditorCommands::SaveScene(engine);

        ImGui::SameLine();
        if (ImGui::Button("Play"))
            engine.LaunchPlayInstance();

        ImGui::SameLine();
        ImGui::Checkbox("VSync", &engine.vsyncEnabled);

        ImGui::SameLine();
        ImGui::TextDisabled("Play launches a separate window");

        auto* gizmos = engine.GetGizmoSystem();
        if (!gizmos)
            return;

        ImGui::Checkbox("Grid", &gizmos->showGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Axes", &gizmos->showAxes);
        ImGui::SameLine();
        ImGui::Checkbox("Collisions", &gizmos->showCollisions);

        DrawCullingStats(engine);
        DrawGizmoModes(engine);
        DrawSnapSettings(engine);
        DrawCommands(engine, world);
    }

private:
    void DrawCullingStats(Engine& engine)
    {
        auto* culling = engine.GetCullingSystem();
        if (!culling)
            return;

        ImGui::Checkbox("Frustum Culling", &culling->enabled);
        ImGui::SameLine();
        ImGui::Text("visible: %d  culled: %d", culling->GetVisibleCount(), culling->GetCulledCount());
    }

    void DrawGizmoModes(Engine& engine)
    {
        auto& selection = engine.GetSelection();

        int mode = static_cast<int>(selection.mode);
        ImGui::RadioButton("Move (W)", &mode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Rotate (E)", &mode, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Scale (R)", &mode, 2);
        selection.mode = static_cast<GizmoMode>(mode);
    }

    void DrawSnapSettings(Engine& engine)
    {
        auto& selection = engine.GetSelection();

        ImGui::Checkbox("Snap", &selection.snapEnabled);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);

        if (selection.mode == GizmoMode::Move)
            ImGui::DragFloat("Step", &selection.moveSnap, 0.05f, 0.01f, 100.0f);
        else if (selection.mode == GizmoMode::Rotate)
            ImGui::DragFloat("Step", &selection.rotateSnap, 1.0f, 1.0f, 180.0f);
        else
            ImGui::DragFloat("Step", &selection.scaleSnap, 0.05f, 0.01f, 100.0f);
    }

    void DrawCommands(Engine& engine, EcsWorld& world)
    {
        if (ImGui::Button("Duplicate (Ctrl+D)"))
            EditorCommands::DuplicateSelection(engine, world);

        ImGui::SameLine();
        if (ImGui::Button("Focus (F)"))
            EditorCommands::FocusSelection(engine, world);

        ImGui::SameLine();
        if (ImGui::Button("Add Light"))
            EditorCommands::AddLight(engine, world);

        ImGui::BeginDisabled(!engine.GetHistory().CanUndo());
        if (ImGui::Button("Undo (Ctrl+Z)"))
            EditorCommands::Undo(engine, world);
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::BeginDisabled(!engine.GetHistory().CanRedo());
        if (ImGui::Button("Redo (Ctrl+Y)"))
            EditorCommands::Redo(engine, world);
        ImGui::EndDisabled();
    }
};
