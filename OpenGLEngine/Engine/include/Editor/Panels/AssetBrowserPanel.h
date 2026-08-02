#pragma once

#include <imgui.h>
#include "Ecs/Core/EcsWorld.h"
#include "Editor/AssetLibrary.h"
#include "Editor/EditorCommands.h"
#include "Engine/Engine.h"
#include "Resource/SceneSerializer.h"

class AssetBrowserPanel
{
public:
    void Draw(Engine& engine, EcsWorld& world)
    {
        if (library.IsEmpty())
            library.Refresh();

        if (ImGui::Button("Refresh"))
            library.Refresh();

        ImGui::SameLine();
        const std::string& sceneName = engine.GetActiveSceneName();
        if (ImGui::Button("Save Scene") && !sceneName.empty())
            SceneSerializer::Save(engine, sceneName);

        ImGui::SameLine();
        ImGui::TextUnformatted(sceneName.empty() ? "(unnamed scene)" : sceneName.c_str());

        ImGui::BeginChild("assetList", ImVec2(0, 160), true);
        for (const auto& path : library.GetModelPaths())
        {
            if (ImGui::Selectable(path.c_str()))
                EditorCommands::SpawnAsset(engine, world, path);
        }
        ImGui::EndChild();
    }

private:
    AssetLibrary library;
};
