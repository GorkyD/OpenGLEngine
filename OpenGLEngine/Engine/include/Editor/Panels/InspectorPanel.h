#pragma once

#include <algorithm>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>
#include <imgui.h>
#include "Ecs/Components/ColliderComponent.h"
#include "Ecs/Components/GroupComponent.h"
#include "Ecs/Components/LocalBoundsComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/MeshNameComponent.h"
#include "Ecs/Components/NameComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/TransformOffsetComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Editor/EditorCommands.h"
#include "Editor/EditorLabels.h"
#include "Engine/Engine.h"
#include "Math/Matrix4.h"

class InspectorPanel
{
public:
    void DrawSelection(Engine& engine, EcsWorld& world)
    {
        auto& selection = engine.GetSelection();
        if (!selection.HasSelection() || !world.HasComponent<TransformComponent>(selection.entity))
        {
            ImGui::TextUnformatted("Nothing selected. Click an object in the viewport.");
            return;
        }

        const Entity entity = selection.entity;
        ImGui::TextUnformatted(EditorLabels::EntityLabel(world, entity).c_str());
        DrawEntityNameField(world, entity);

        auto& transform = world.GetComponent<TransformComponent>(entity);
        ImGui::DragFloat3("Position##sel", &transform.position.x, 0.01f);
        ImGui::DragFloat3("Scale##sel", &transform.scale.x, 0.01f);

        if (world.HasComponent<TransformOffsetComponent>(entity))
        {
            auto& offset = world.GetComponent<TransformOffsetComponent>(entity);
            if (ImGui::DragFloat3("Rotation (deg)##sel", &offset.rotationDegrees.x, 0.5f))
                transform.rotation = offset.GetRotationMatrix();
        }

        DrawCollider(world, entity);

        if (ImGui::Button("Delete") || (ImGui::IsKeyPressed(ImGuiKey_Delete) && !ImGui::GetIO().WantTextInput))
            EditorCommands::DeleteSelection(engine, world);
    }

    void DrawTransforms(Engine& engine, EcsWorld& world)
    {
        RebuildGroupsIfChanged(engine, world);

        for (auto& [name, entities] : groups)
            DrawTransformGroup(engine, world, name, entities);
        for (const auto entity : ungrouped)
            DrawTransformGroup(engine, world, EditorLabels::EntityLabel(world, entity), {entity});
    }

private:
    void DrawCollider(EcsWorld& world, Entity entity)
    {
        ImGui::Separator();

        if (!world.HasComponent<ColliderComponent>(entity))
        {
            if (ImGui::Button("Add Collider"))
                AddColliderFromMesh(world, entity);
            return;
        }

        auto& collider = world.GetComponent<ColliderComponent>(entity);

        ImGui::TextUnformatted("Collider");
        ImGui::DragFloat3("Center##col", &collider.center.x, 0.01f);
        ImGui::DragFloat3("Size##col", &collider.size.x, 0.01f, 0.001f, 1000.0f);

        ImGui::Checkbox("Static", &collider.isStatic);
        ImGui::SameLine();
        ImGui::Checkbox("Trigger", &collider.isTrigger);

        ImGui::SliderFloat("Restitution", &collider.restitution, 0.0f, 1.0f);

        DrawLayerBits("Layer", collider.layer);
        DrawLayerBits("Collides With", collider.collidesWith);

        if (ImGui::Button("Fit To Mesh"))
            FitColliderToMesh(world, entity, collider);

        ImGui::SameLine();
        if (ImGui::Button("Remove Collider"))
            world.GetPool<ColliderComponent>().Remove(entity);
    }

    void DrawLayerBits(const char* label, unsigned int& mask)
    {
        static const char* names[] = {"Default", "Player", "Enemy", "Projectile", "Trigger"};

        ImGui::TextUnformatted(label);
        ImGui::PushID(label);

        for (int bit = 0; bit < 5; bit++)
        {
            const unsigned int flag = 1u << bit;
            bool enabled = (mask & flag) != 0;

            if (bit > 0)
                ImGui::SameLine();

            if (ImGui::Checkbox(names[bit], &enabled))
                mask = enabled ? (mask | flag) : (mask & ~flag);
        }

        ImGui::PopID();
    }

    void AddColliderFromMesh(EcsWorld& world, Entity entity)
    {
        auto& collider = world.AddComponent<ColliderComponent>(entity);
        FitColliderToMesh(world, entity, collider);
    }

    void FitColliderToMesh(EcsWorld& world, Entity entity, ColliderComponent& collider)
    {
        if (!world.HasComponent<LocalBoundsComponent>(entity))
        {
            collider.center = {0, 0, 0};
            collider.size = {1, 1, 1};
            return;
        }

        const auto& bounds = world.GetComponent<LocalBoundsComponent>(entity);
        collider.center = {(bounds.min.x + bounds.max.x) * 0.5f, (bounds.min.y + bounds.max.y) * 0.5f, (bounds.min.z + bounds.max.z) * 0.5f};
        collider.size = {bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, bounds.max.z - bounds.min.z};
    }

    void RebuildGroupsIfChanged(Engine& engine, EcsWorld& world)
    {
        const size_t transformCount = world.GetPool<TransformComponent>().Size();
        const unsigned int version = engine.GetSceneStructureVersion();

        if (built && version == cachedVersion && transformCount == cachedTransformCount)
            return;

        built = true;
        cachedVersion = version;
        cachedTransformCount = transformCount;

        groups.clear();
        ungrouped.clear();

        for (auto& [entity, transform] : world.GetPool<TransformComponent>())
        {
            if (world.HasComponent<GroupComponent>(entity))
                groups[world.GetComponent<GroupComponent>(entity).name].push_back(entity);
            else
                ungrouped.push_back(entity);
        }

        for (auto& [name, entities] : groups)
            std::sort(entities.begin(), entities.end());

        std::sort(ungrouped.begin(), ungrouped.end());
    }

    void DrawTransformGroup(Engine& engine, EcsWorld& world, const std::string& name, const std::vector<Entity>& entities)
    {
        if (entities.empty())
            return;

        const Entity keyEntity = entities.front();

        std::string label = name;
        if (entities.size() > 1)
            label += " (" + std::to_string(entities.size()) + " parts)";
        label += "###group" + std::to_string(keyEntity);

        if (!ImGui::TreeNode(label.c_str()))
            return;

        DrawRenameField(engine, world, name, entities, keyEntity);
        DrawGroupTransform(world, entities, keyEntity);
        DrawGroupMaterial(engine, world, entities, keyEntity);

        if (entities.size() > 1)
        {
            ImGui::Separator();
            if (ImGui::TreeNode(("Parts###parts" + std::to_string(keyEntity)).c_str()))
            {
                for (const auto entity : entities)
                    DrawPartNode(world, entity);
                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }

    void DrawGroupTransform(EcsWorld& world, const std::vector<Entity>& entities, Entity keyEntity)
    {
        auto& transform = world.GetComponent<TransformComponent>(keyEntity);
        const bool procedural = world.HasComponent<TransformOffsetComponent>(keyEntity) && world.GetComponent<TransformOffsetComponent>(keyEntity).procedural;

        if (procedural)
        {
            auto& offset = world.GetComponent<TransformOffsetComponent>(keyEntity);

            if (ImGui::DragFloat3("Position Offset", &offset.position.x, 0.01f))
                for (const auto entity : entities)
                    world.GetComponent<TransformOffsetComponent>(entity).position = offset.position;

            if (ImGui::DragFloat3("Scale", &offset.scale.x, 0.01f))
                for (const auto entity : entities)
                    world.GetComponent<TransformOffsetComponent>(entity).scale = offset.scale;

            if (ImGui::DragFloat3("Rotation Offset (deg)", &offset.rotationDegrees.x, 0.5f))
                for (const auto entity : entities)
                    world.GetComponent<TransformOffsetComponent>(entity).rotationDegrees = offset.rotationDegrees;

            return;
        }

        if (ImGui::DragFloat3("Position", &transform.position.x, 0.01f))
            for (const auto entity : entities)
                world.GetComponent<TransformComponent>(entity).position = transform.position;

        if (ImGui::DragFloat3("Scale", &transform.scale.x, 0.01f))
            for (const auto entity : entities)
                world.GetComponent<TransformComponent>(entity).scale = transform.scale;

        if (!world.HasComponent<TransformOffsetComponent>(keyEntity))
            return;

        auto& offset = world.GetComponent<TransformOffsetComponent>(keyEntity);
        if (ImGui::DragFloat3("Rotation (deg)", &offset.rotationDegrees.x, 0.5f))
        {
            const Matrix4 rotation = offset.GetRotationMatrix();
            for (const auto entity : entities)
            {
                world.GetComponent<TransformOffsetComponent>(entity).rotationDegrees = offset.rotationDegrees;
                world.GetComponent<TransformComponent>(entity).rotation = rotation;
            }
        }
    }

    void DrawGroupMaterial(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities, Entity keyEntity)
    {
        if (!world.HasComponent<MaterialComponent>(keyEntity))
            return;

        auto& material = world.GetComponent<MaterialComponent>(keyEntity);

        if (materialEditBefore.empty() && !ImGui::IsAnyItemActive())
            materialEditSnapshot = EditorCommands::CaptureMaterials(world, entities);

        bool changed = false;
        changed |= ImGui::ColorEdit4("Diffuse Color", &material.diffuseColor.x);
        changed |= ImGui::ColorEdit3("Emissive Color", &material.emissiveColor.x);
        changed |= ImGui::SliderFloat("Emissive Intensity", &material.emissiveIntensity, 0.0f, 10.0f);
        changed |= ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f);

        if (changed && materialEditBefore.empty())
        {
            materialEditBefore = materialEditSnapshot;
            materialEditEntities = entities;
        }

        if (!materialEditBefore.empty() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            EditorAction action;
            action.type = EditorAction::Type::Material;
            action.entities = materialEditEntities;
            action.materialsBefore = materialEditBefore;
            action.materialsAfter = EditorCommands::CaptureMaterials(world, materialEditEntities);
            engine.GetHistory().Push(std::move(action));

            materialEditBefore.clear();
            materialEditEntities.clear();
        }

        if (!changed)
            return;

        for (const auto entity : entities)
        {
            if (entity != keyEntity && world.HasComponent<MaterialComponent>(entity))
                world.GetComponent<MaterialComponent>(entity) = material;
        }
    }

    void DrawRenameField(Engine& engine, EcsWorld& world, const std::string& currentName, const std::vector<Entity>& entities, Entity keyEntity)
    {
        if (!world.HasComponent<GroupComponent>(keyEntity))
            return;

        char buffer[64] = {};
        std::snprintf(buffer, sizeof(buffer), "%s", currentName.c_str());

        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::InputText("Name", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            const std::string candidate = EditorLabels::Trim(buffer);

            if (candidate.empty())
                renameError = "Name cannot be empty";
            else if (candidate.find('|') != std::string::npos)
                renameError = "Name cannot contain '|'";
            else if (candidate != currentName && EditorLabels::IsGroupNameTaken(world, candidate, keyEntity))
                renameError = "Name '" + candidate + "' is already used";
            else
            {
                EditorCommands::Rename(engine, world, entities, currentName, candidate);
                renameError.clear();
            }
        }

        if (!renameError.empty())
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "%s", renameError.c_str());
    }

    void DrawEntityNameField(EcsWorld& world, Entity entity)
    {
        const std::string currentName = world.HasComponent<NameComponent>(entity) ? world.GetComponent<NameComponent>(entity).name : "";

        char buffer[64] = {};
        std::snprintf(buffer, sizeof(buffer), "%s", currentName.c_str());

        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::InputText("Name", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            const std::string candidate = EditorLabels::Trim(buffer);

            if (world.HasComponent<NameComponent>(entity))
                world.GetComponent<NameComponent>(entity).name = candidate;
            else if (!candidate.empty())
                world.AddComponent<NameComponent>(entity).name = candidate;
        }

        if (world.HasComponent<MeshNameComponent>(entity))
        {
            ImGui::SameLine();
            ImGui::TextDisabled("mesh: %s", world.GetComponent<MeshNameComponent>(entity).name.c_str());
        }
    }

    void DrawPartNode(EcsWorld& world, Entity entity)
    {
        ImGui::PushID(static_cast<int>(entity));

        if (world.HasComponent<MeshComponent>(entity))
        {
            ImGui::Checkbox("##visible", &world.GetComponent<MeshComponent>(entity).visible);
            ImGui::SameLine();
        }

        const std::string label = EditorLabels::EntityLabel(world, entity) + "###part";
        if (!ImGui::TreeNode(label.c_str()))
        {
            ImGui::PopID();
            return;
        }

        DrawEntityNameField(world, entity);

        const bool procedural = world.HasComponent<TransformOffsetComponent>(entity) && world.GetComponent<TransformOffsetComponent>(entity).procedural;

        if (procedural)
        {
            auto& offset = world.GetComponent<TransformOffsetComponent>(entity);
            ImGui::DragFloat3("Position Offset", &offset.position.x, 0.01f);
            ImGui::DragFloat3("Rotation Offset (deg)", &offset.rotationDegrees.x, 0.5f);
            ImGui::DragFloat3("Scale", &offset.scale.x, 0.01f);
        }
        else if (world.HasComponent<TransformComponent>(entity))
        {
            auto& transform = world.GetComponent<TransformComponent>(entity);
            ImGui::DragFloat3("Position", &transform.position.x, 0.01f);
            ImGui::DragFloat3("Scale", &transform.scale.x, 0.01f);

            if (world.HasComponent<TransformOffsetComponent>(entity))
            {
                auto& offset = world.GetComponent<TransformOffsetComponent>(entity);
                if (ImGui::DragFloat3("Rotation (deg)", &offset.rotationDegrees.x, 0.5f))
                    transform.rotation = offset.GetRotationMatrix();
            }
        }

        if (world.HasComponent<MaterialComponent>(entity))
            ImGui::ColorEdit4("Diffuse Color", &world.GetComponent<MaterialComponent>(entity).diffuseColor.x);

        ImGui::TreePop();
        ImGui::PopID();
    }

    std::unordered_map<std::string, std::vector<Entity>> groups;
    std::vector<Entity> ungrouped;

    unsigned int cachedVersion = 0;
    size_t cachedTransformCount = 0;
    bool built = false;

    std::string renameError;
    std::vector<MaterialSnapshot> materialEditSnapshot;
    std::vector<MaterialSnapshot> materialEditBefore;
    std::vector<Entity> materialEditEntities;
};
