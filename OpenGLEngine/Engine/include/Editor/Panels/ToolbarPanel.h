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

        if (ImGui::GetIO().WantTextInput)
            return;

        if (ImGui::IsKeyPressed(ImGuiKey_W))
            selection.mode = GizmoMode::Move;
        if (ImGui::IsKeyPressed(ImGuiKey_E))
            selection.mode = GizmoMode::Rotate;
        if (ImGui::IsKeyPressed(ImGuiKey_R))
            selection.mode = GizmoMode::Scale;
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
        const auto& io = ImGui::GetIO();

        if (ImGui::Button("Duplicate") || (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D)))
            EditorCommands::DuplicateSelection(engine, world);

        ImGui::SameLine();
        if (ImGui::Button("Focus") || (!io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F)))
            EditorCommands::FocusSelection(engine, world);

        ImGui::SameLine();
        if (ImGui::Button("Add Light"))
            EditorCommands::AddLight(world);

        if (ImGui::Button("Undo") || (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)))
            EditorCommands::Undo(engine, world);

        ImGui::SameLine();
        if (ImGui::Button("Redo") || (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)))
            EditorCommands::Redo(engine, world);

        ImGui::SameLine();
        ImGui::Text("undo: %s  redo: %s", engine.GetHistory().CanUndo() ? "yes" : "no", engine.GetHistory().CanRedo() ? "yes" : "no");
    }
};
