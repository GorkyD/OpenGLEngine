#include "Editor/Panels/InspectorPanel.h"
#include <algorithm>
#include <cstdio>
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

void InspectorPanel::DrawSelection(Engine& engine, EcsWorld& world)
{
    auto& selection = engine.GetSelection();
    if (!selection.HasSelection() || !world.HasComponent<TransformComponent>(selection.entity))
    {
        ImGui::TextUnformatted("Nothing selected. Click an object in the viewport.");
        return;
    }

    const Entity entity = selection.entity;
    ImGui::TextUnformatted(EditorLabels::EntityLabel(world, entity).c_str());
    DrawEntityNameField(engine, world, entity);

    const std::vector<Entity> group = EditorCommands::CollectGroup(world, entity);
    if (group.size() > 1)
        ImGui::TextDisabled("Editing whole group (%d parts)", static_cast<int>(group.size()));

    ImGui::PushID("selection");
    DrawGroupTransform(engine, world, group, entity);
    ImGui::PopID();

    DrawCollider(engine, world, entity);

    if (ImGui::Button("Delete"))
        EditorCommands::DeleteSelection(engine, world);
}

void InspectorPanel::DrawTransforms(Engine& engine, EcsWorld& world)
{
    RebuildGroupsIfChanged(engine, world);

    for (auto& [name, entities] : groups)
        DrawTransformGroup(engine, world, name, entities);
    for (const auto entity : ungrouped)
        DrawTransformGroup(engine, world, EditorLabels::EntityLabel(world, entity), {entity});
}

void InspectorPanel::DrawCollider(Engine& engine, EcsWorld& world, Entity entity)
{
    ImGui::Separator();

    const std::vector<Entity> target = {entity};

    if (!world.HasComponent<ColliderComponent>(entity))
    {
        if (ImGui::Button("Add Collider"))
        {
            auto before = EditorState::Capture(world, target, StateCollider);
            FitColliderToMesh(world, entity, world.AddComponent<ColliderComponent>(entity));
            EditorCommands::PushPropertyEdit(engine, "Add Collider", std::move(before), EditorState::Capture(world, target, StateCollider));
        }
        return;
    }

    auto& collider = world.GetComponent<ColliderComponent>(entity);

    ImGui::TextUnformatted("Collider");
    undo.Edit(engine, world, target, StateCollider, "Collider Center", [&] { return ImGui::DragFloat3("Center##col", &collider.center.x, 0.01f); });
    undo.Edit(engine, world, target, StateCollider, "Collider Size", [&] { return ImGui::DragFloat3("Size##col", &collider.size.x, 0.01f, 0.001f, 1000.0f); });

    undo.Edit(engine, world, target, StateCollider, "Collider Static", [&] { return ImGui::Checkbox("Static", &collider.isStatic); });
    ImGui::SameLine();
    undo.Edit(engine, world, target, StateCollider, "Collider Trigger", [&] { return ImGui::Checkbox("Trigger", &collider.isTrigger); });

    undo.Edit(engine, world, target, StateCollider, "Collider Restitution", [&] { return ImGui::SliderFloat("Restitution", &collider.restitution, 0.0f, 1.0f); });

    {
        auto before = EditorState::Capture(world, target, StateCollider);
        bool changed = DrawLayerBits("Layer", collider.layer);
        changed |= DrawLayerBits("Collides With", collider.collidesWith);
        if (changed)
            EditorCommands::PushPropertyEdit(engine, "Collider Layers", std::move(before), EditorState::Capture(world, target, StateCollider));
    }

    if (ImGui::Button("Fit To Mesh"))
    {
        auto before = EditorState::Capture(world, target, StateCollider);
        FitColliderToMesh(world, entity, collider);
        EditorCommands::PushPropertyEdit(engine, "Fit Collider", std::move(before), EditorState::Capture(world, target, StateCollider));
    }

    ImGui::SameLine();
    if (ImGui::Button("Remove Collider"))
    {
        auto before = EditorState::Capture(world, target, StateCollider);
        world.GetPool<ColliderComponent>().Remove(entity);
        EditorCommands::PushPropertyEdit(engine, "Remove Collider", std::move(before), EditorState::Capture(world, target, StateCollider));
    }
}

bool InspectorPanel::DrawLayerBits(const char* label, unsigned int& mask)
{
    static const char* names[] = {"Default", "Player", "Enemy", "Projectile", "Trigger"};

    ImGui::TextUnformatted(label);
    ImGui::PushID(label);

    bool changed = false;
    for (int bit = 0; bit < 5; bit++)
    {
        const unsigned int flag = 1u << bit;
        bool enabled = (mask & flag) != 0;

        if (bit > 0)
            ImGui::SameLine();

        if (ImGui::Checkbox(names[bit], &enabled))
        {
            mask = enabled ? (mask | flag) : (mask & ~flag);
            changed = true;
        }
    }

    ImGui::PopID();
    return changed;
}

void InspectorPanel::FitColliderToMesh(EcsWorld& world, Entity entity, ColliderComponent& collider)
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

void InspectorPanel::RebuildGroupsIfChanged(Engine& engine, EcsWorld& world)
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

void InspectorPanel::DrawTransformGroup(Engine& engine, EcsWorld& world, const std::string& name, const std::vector<Entity>& entities)
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
    DrawGroupTransform(engine, world, entities, keyEntity);
    DrawGroupMaterial(engine, world, entities, keyEntity);

    if (entities.size() > 1)
    {
        ImGui::Separator();
        if (ImGui::TreeNode(("Parts###parts" + std::to_string(keyEntity)).c_str()))
        {
            for (const auto entity : entities)
                DrawPartNode(engine, world, entity);
            ImGui::TreePop();
        }
    }

    ImGui::TreePop();
}

void InspectorPanel::DrawGroupTransform(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities, Entity keyEntity)
{
    auto& transform = world.GetComponent<TransformComponent>(keyEntity);
    const bool hasOffset = world.HasComponent<TransformOffsetComponent>(keyEntity);
    const bool procedural = hasOffset && world.GetComponent<TransformOffsetComponent>(keyEntity).procedural;

    auto forEachOffset = [&](auto&& fn)
    {
        for (const auto entity : entities)
        {
            if (world.HasComponent<TransformOffsetComponent>(entity))
                fn(world.GetComponent<TransformOffsetComponent>(entity));
        }
    };

    auto forEachTransform = [&](auto&& fn)
    {
        for (const auto entity : entities)
        {
            if (world.HasComponent<TransformComponent>(entity))
                fn(world.GetComponent<TransformComponent>(entity));
        }
    };

    if (procedural)
    {
        auto& offset = world.GetComponent<TransformOffsetComponent>(keyEntity);

        undo.Edit(engine, world, entities, StateTransform, "Position Offset",
                  [&]
                  {
                      if (!ImGui::DragFloat3("Position Offset", &offset.position.x, 0.01f))
                          return false;
                      forEachOffset([&](TransformOffsetComponent& o) { o.position = offset.position; });
                      return true;
                  });

        undo.Edit(engine, world, entities, StateTransform, "Scale Offset",
                  [&]
                  {
                      if (!ImGui::DragFloat3("Scale", &offset.scale.x, 0.01f))
                          return false;
                      forEachOffset([&](TransformOffsetComponent& o) { o.scale = offset.scale; });
                      return true;
                  });

        undo.Edit(engine, world, entities, StateTransform, "Rotation Offset",
                  [&]
                  {
                      if (!ImGui::DragFloat3("Rotation Offset (deg)", &offset.rotationDegrees.x, 0.5f))
                          return false;
                      forEachOffset([&](TransformOffsetComponent& o) { o.rotationDegrees = offset.rotationDegrees; });
                      return true;
                  });
        return;
    }

    undo.Edit(engine, world, entities, StateTransform, "Move",
              [&]
              {
                  if (!ImGui::DragFloat3("Position", &transform.position.x, 0.01f))
                      return false;
                  forEachTransform([&](TransformComponent& t) { t.position = transform.position; });
                  return true;
              });

    undo.Edit(engine, world, entities, StateTransform, "Scale",
              [&]
              {
                  if (!ImGui::DragFloat3("Scale", &transform.scale.x, 0.01f))
                      return false;
                  forEachTransform([&](TransformComponent& t) { t.scale = transform.scale; });
                  return true;
              });

    if (!hasOffset)
        return;

    auto& offset = world.GetComponent<TransformOffsetComponent>(keyEntity);
    undo.Edit(engine, world, entities, StateTransform, "Rotate",
              [&]
              {
                  if (!ImGui::DragFloat3("Rotation (deg)", &offset.rotationDegrees.x, 0.5f))
                      return false;
                  const Matrix4 rotation = offset.GetRotationMatrix();
                  forEachOffset([&](TransformOffsetComponent& o) { o.rotationDegrees = offset.rotationDegrees; });
                  forEachTransform([&](TransformComponent& t) { t.rotation = rotation; });
                  return true;
              });
}

void InspectorPanel::DrawGroupMaterial(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities, Entity keyEntity)
{
    if (!world.HasComponent<MaterialComponent>(keyEntity))
        return;

    auto& material = world.GetComponent<MaterialComponent>(keyEntity);

    auto forEachMaterial = [&](auto&& fn)
    {
        for (const auto entity : entities)
        {
            if (world.HasComponent<MaterialComponent>(entity))
                fn(world.GetComponent<MaterialComponent>(entity));
        }
    };

    undo.Edit(engine, world, entities, StateMaterial, "Diffuse Color",
              [&]
              {
                  if (!ImGui::ColorEdit4("Diffuse Color", &material.diffuseColor.x))
                      return false;
                  forEachMaterial([&](MaterialComponent& m) { m.diffuseColor = material.diffuseColor; });
                  return true;
              });

    undo.Edit(engine, world, entities, StateMaterial, "Emissive Color",
              [&]
              {
                  if (!ImGui::ColorEdit3("Emissive Color", &material.emissiveColor.x))
                      return false;
                  forEachMaterial([&](MaterialComponent& m) { m.emissiveColor = material.emissiveColor; });
                  return true;
              });

    undo.Edit(engine, world, entities, StateMaterial, "Emissive Intensity",
              [&]
              {
                  if (!ImGui::SliderFloat("Emissive Intensity", &material.emissiveIntensity, 0.0f, 10.0f))
                      return false;
                  forEachMaterial([&](MaterialComponent& m) { m.emissiveIntensity = material.emissiveIntensity; });
                  return true;
              });

    undo.Edit(engine, world, entities, StateMaterial, "Roughness",
              [&]
              {
                  if (!ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f))
                      return false;
                  forEachMaterial([&](MaterialComponent& m) { m.roughness = material.roughness; });
                  return true;
              });

    undo.Edit(engine, world, entities, StateMaterial, "Metallic",
              [&]
              {
                  if (!ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f))
                      return false;
                  forEachMaterial([&](MaterialComponent& m) { m.metallic = material.metallic; });
                  return true;
              });
}

void InspectorPanel::DrawRenameField(Engine& engine, EcsWorld& world, const std::string& currentName, const std::vector<Entity>& entities, Entity keyEntity)
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
            if (candidate != currentName)
                EditorCommands::Rename(engine, world, entities, candidate);
            renameError.clear();
        }
    }

    if (!renameError.empty())
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "%s", renameError.c_str());
}

void InspectorPanel::DrawEntityNameField(Engine& engine, EcsWorld& world, Entity entity)
{
    const std::string currentName = world.HasComponent<NameComponent>(entity) ? world.GetComponent<NameComponent>(entity).name : "";

    char buffer[64] = {};
    std::snprintf(buffer, sizeof(buffer), "%s", currentName.c_str());

    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::InputText("Name", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        const std::string candidate = EditorLabels::Trim(buffer);
        if (candidate != currentName)
        {
            const std::vector<Entity> target = {entity};
            auto before = EditorState::Capture(world, target, StateName);

            if (candidate.empty())
                world.GetPool<NameComponent>().Remove(entity);
            else
                world.AddComponent<NameComponent>(entity).name = candidate;

            EditorCommands::PushPropertyEdit(engine, "Rename Entity", std::move(before), EditorState::Capture(world, target, StateName));
            engine.BumpSceneStructureVersion();
        }
    }

    if (world.HasComponent<MeshNameComponent>(entity))
    {
        ImGui::SameLine();
        ImGui::TextDisabled("mesh: %s", world.GetComponent<MeshNameComponent>(entity).name.c_str());
    }
}

void InspectorPanel::DrawPartNode(Engine& engine, EcsWorld& world, Entity entity)
{
    ImGui::PushID(static_cast<int>(entity));

    const std::vector<Entity> target = {entity};

    if (world.HasComponent<MeshComponent>(entity))
    {
        auto& mesh = world.GetComponent<MeshComponent>(entity);
        undo.Edit(engine, world, target, StateVisibility, "Visibility", [&] { return ImGui::Checkbox("##visible", &mesh.visible); });
        ImGui::SameLine();
    }

    const std::string label = EditorLabels::EntityLabel(world, entity) + "###part";
    if (!ImGui::TreeNode(label.c_str()))
    {
        ImGui::PopID();
        return;
    }

    DrawEntityNameField(engine, world, entity);

    if (world.HasComponent<TransformComponent>(entity))
        DrawGroupTransform(engine, world, target, entity);

    if (world.HasComponent<MaterialComponent>(entity))
    {
        auto& material = world.GetComponent<MaterialComponent>(entity);
        undo.Edit(engine, world, target, StateMaterial, "Diffuse Color", [&] { return ImGui::ColorEdit4("Diffuse Color", &material.diffuseColor.x); });
    }

    ImGui::TreePop();
    ImGui::PopID();
}
