#pragma once

#include <string>
#include <utility>
#include <vector>
#include <imgui.h>
#include "Editor/EditorCommands.h"
#include "Editor/EditorState.h"

class Engine;
class EcsWorld;

class EditorUndoTracker
{
public:
    template <typename Widget> bool Edit(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities, unsigned int mask, const char* label, Widget&& widget)
    {
        std::vector<EntityState> before = EditorState::Capture(world, entities, mask);
        const bool changed = widget();

        if (ImGui::IsItemActivated())
        {
            pendingBefore = before;
            pendingEntities = entities;
            pendingMask = mask;
            hasPending = true;
        }

        if (ImGui::IsItemDeactivatedAfterEdit() && hasPending)
        {
            EditorCommands::PushPropertyEdit(engine, label, std::move(pendingBefore), EditorState::Capture(world, pendingEntities, pendingMask));
            hasPending = false;
        }
        else if (changed && !ImGui::IsItemActive())
        {
            EditorCommands::PushPropertyEdit(engine, label, std::move(before), EditorState::Capture(world, entities, mask));
            hasPending = false;
        }

        return changed;
    }

private:
    std::vector<EntityState> pendingBefore;
    std::vector<Entity> pendingEntities;
    unsigned int pendingMask = 0;
    bool hasPending = false;
};
