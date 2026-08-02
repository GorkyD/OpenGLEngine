#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>
#include <imgui.h>
#include "Ecs/Components/ParentComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Editor/EditorCommands.h"
#include "Editor/EditorLabels.h"
#include "Engine/Engine.h"

class HierarchyPanel
{
public:
    void Draw(Engine& engine, EcsWorld& world)
    {
        RebuildIfChanged(engine, world);

        ImGui::Begin("Hierarchy");

        ImGui::TextDisabled("Drag a node onto another to reparent, drop below to unparent");
        ImGui::Separator();

        for (const auto& root : roots)
            DrawNode(engine, world, root);

        ImGui::Dummy(ImVec2(-1.0f, 40.0f));
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadId))
                Reparent(engine, world, static_cast<const char*>(payload->Data), "");
            ImGui::EndDragDropTarget();
        }

        if (!error.empty())
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "%s", error.c_str());

        ImGui::End();
    }

private:
    static constexpr const char* payloadId = "HIERARCHY_NODE";

    struct Node
    {
        std::vector<Entity> entities;
        std::vector<std::string> children;
        std::string parentGroup;
        std::string attachBone;
    };

    void RebuildIfChanged(Engine& engine, EcsWorld& world)
    {
        const size_t transformCount = world.GetPool<TransformComponent>().Size();
        const unsigned int version = engine.GetSceneStructureVersion();

        if (built && version == cachedVersion && transformCount == cachedTransformCount)
            return;

        built = true;
        cachedVersion = version;
        cachedTransformCount = transformCount;
        BuildTree(world);
    }

    void BuildTree(EcsWorld& world)
    {
        nodes.clear();
        roots.clear();

        for (auto& [entity, transform] : world.GetPool<TransformComponent>())
            nodes[EditorLabels::GroupKey(world, entity)].entities.push_back(entity);

        for (auto& [groupName, node] : nodes)
            std::sort(node.entities.begin(), node.entities.end());

        for (auto& [groupName, node] : nodes)
        {
            const Entity keyEntity = node.entities.front();
            if (!world.HasComponent<ParentComponent>(keyEntity))
                continue;

            const auto& parentComp = world.GetComponent<ParentComponent>(keyEntity);
            if (parentComp.parent == INVALID_ENTITY || !world.HasComponent<TransformComponent>(parentComp.parent))
                continue;

            node.parentGroup = EditorLabels::GroupKey(world, parentComp.parent);
            node.attachBone = parentComp.attachBone;
        }

        for (auto& [groupName, node] : nodes)
        {
            if (node.parentGroup.empty() || nodes.find(node.parentGroup) == nodes.end())
                roots.push_back(groupName);
            else
                nodes[node.parentGroup].children.push_back(groupName);
        }

        std::sort(roots.begin(), roots.end());
    }

    void DrawNode(Engine& engine, EcsWorld& world, const std::string& groupName)
    {
        auto it = nodes.find(groupName);
        if (it == nodes.end())
            return;

        Node& node = it->second;
        const Entity keyEntity = node.entities.front();
        auto& selection = engine.GetSelection();

        std::string label = groupName;
        if (node.entities.size() > 1)
            label += " [" + std::to_string(node.entities.size()) + "]";
        label += "###hier" + groupName;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
        if (node.children.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;
        if (std::any_of(node.entities.begin(), node.entities.end(), [&](Entity e) { return selection.Contains(e); }))
            flags |= ImGuiTreeNodeFlags_Selected;

        const bool opened = ImGui::TreeNodeEx(label.c_str(), flags);

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoDisableHover))
        {
            ImGui::SetDragDropPayload(payloadId, groupName.c_str(), groupName.size() + 1);
            ImGui::TextUnformatted(groupName.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadId))
                Reparent(engine, world, static_cast<const char*>(payload->Data), groupName);
            ImGui::EndDragDropTarget();
        }

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            if (ImGui::GetIO().KeyCtrl)
            {
                selection.Toggle(keyEntity);
            }
            else
            {
                selection.additional.clear();
                selection.entity = keyEntity;
            }
        }

        if (!opened)
            return;

        DrawChildren(engine, world, node, groupName);
        ImGui::TreePop();
    }

    void DrawChildren(Engine& engine, EcsWorld& world, Node& node, const std::string& groupName)
    {
        std::sort(node.children.begin(), node.children.end());

        std::unordered_map<std::string, std::vector<std::string>> boneBuckets;
        std::vector<std::string> directChildren;

        for (const auto& child : node.children)
        {
            const std::string& bone = nodes[child].attachBone;
            if (bone.empty())
                directChildren.push_back(child);
            else
                boneBuckets[bone].push_back(child);
        }

        for (auto& [boneName, boneChildren] : boneBuckets)
        {
            const std::string boneLabel = "[bone] " + boneName + "###bone" + groupName + boneName;
            if (ImGui::TreeNodeEx(boneLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (const auto& child : boneChildren)
                    DrawNode(engine, world, child);
                ImGui::TreePop();
            }
        }

        for (const auto& child : directChildren)
            DrawNode(engine, world, child);
    }

    void Reparent(Engine& engine, EcsWorld& world, const std::string& childGroup, const std::string& parentGroup)
    {
        if (childGroup.empty() || childGroup == parentGroup)
            return;

        auto childIt = nodes.find(childGroup);
        if (childIt == nodes.end())
            return;

        if (!parentGroup.empty() && IsDescendant(parentGroup, childGroup))
        {
            error = "Cannot parent '" + childGroup + "' to its own child";
            return;
        }

        Entity parentEntity = INVALID_ENTITY;
        if (!parentGroup.empty())
        {
            auto parentIt = nodes.find(parentGroup);
            if (parentIt == nodes.end())
                return;
            parentEntity = parentIt->second.entities.front();
        }

        EditorCommands::Reparent(engine, world, childIt->second.entities, parentEntity);
        error.clear();
    }

    bool IsDescendant(const std::string& candidate, const std::string& ancestor) const
    {
        std::string current = candidate;
        for (int guard = 0; guard < 256; guard++)
        {
            auto it = nodes.find(current);
            if (it == nodes.end() || it->second.parentGroup.empty())
                return false;
            if (it->second.parentGroup == ancestor)
                return true;
            current = it->second.parentGroup;
        }

        return false;
    }

    std::unordered_map<std::string, Node> nodes;
    std::vector<std::string> roots;
    std::string error;

    unsigned int cachedVersion = 0;
    size_t cachedTransformCount = 0;
    bool built = false;
};
