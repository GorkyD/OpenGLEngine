#include "Editor/EditorCommands.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "Ecs/Components/AABB.h"
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/EditorIdComponent.h"
#include "Ecs/Components/GroupComponent.h"
#include "Ecs/Components/HitFlashComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ParentComponent.h"
#include "Ecs/Components/PrefabComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/TransformOffsetComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Engine/Engine.h"
#include "Extension/Extension.h"
#include "Resource/EntityFactory.h"
#include "Resource/SceneSerializer.h"

std::vector<Entity> EditorCommands::CollectGroup(EcsWorld& world, Entity entity)
{
    std::vector<Entity> entities;

    if (world.HasComponent<GroupComponent>(entity))
    {
        const std::string groupName = world.GetComponent<GroupComponent>(entity).name;
        for (auto& [candidate, group] : world.GetPool<GroupComponent>())
        {
            if (group.name == groupName)
                entities.push_back(candidate);
        }

        std::sort(entities.begin(), entities.end());
        return entities;
    }

    entities.push_back(entity);
    return entities;
}

std::vector<Entity> EditorCommands::CollectSelectedGroups(Engine& engine, EcsWorld& world)
{
    std::vector<Entity> result;
    std::unordered_set<Entity> seen;

    for (const auto selected : engine.GetSelection().All())
    {
        for (const auto entity : CollectGroup(world, selected))
        {
            if (seen.insert(entity).second)
                result.push_back(entity);
        }
    }

    return result;
}

static void ConfigurePlacedEntity(EcsWorld& world, Entity entity, const std::string& assetPath, const std::string& groupName)
{
    world.AddComponent<PrefabComponent>(entity).assetPath = assetPath;
    world.AddComponent<GroupComponent>(entity).name = groupName;
    world.AddComponent<TransformOffsetComponent>(entity);
    world.AddComponent<HitFlashComponent>(entity);
}

static Vector3 SpawnPointInFrontOfCamera(EcsWorld& world)
{
    for (auto& [entity, camera] : world.GetPool<CameraComponent>())
    {
        if (!camera.isActive || !world.HasComponent<TransformComponent>(entity))
            continue;

        return world.GetComponent<TransformComponent>(entity).position + camera.forward * 5.0f;
    }

    return {0, 0, 0};
}

void EditorCommands::DestroyObjects(Engine& engine, EcsWorld& world, ObjectLifecycle& lifecycle)
{
    std::unordered_set<Entity> destroyed;

    for (auto& record : lifecycle.records)
    {
        for (auto& state : record.states)
        {
            const Entity entity = EditorState::Resolve(world, state.id);
            if (entity == INVALID_ENTITY)
                continue;

            state = EditorState::Capture(world, entity, StateAll);
            destroyed.insert(entity);
        }
    }

    lifecycle.children.clear();
    for (auto& [child, parent] : world.GetPool<ParentComponent>())
    {
        if (destroyed.count(child) || !destroyed.count(parent.parent))
            continue;

        lifecycle.children.push_back({EditorState::IdOf(world, child), EditorState::IdOf(world, parent.parent), parent.attachBone});
    }

    auto& selection = engine.GetSelection();
    for (const auto entity : destroyed)
    {
        if (selection.Contains(entity))
            selection.Clear();

        world.GetPool<EditorIdComponent>().Remove(entity);
        world.DestroyEntity(entity);
    }

    engine.BumpSceneStructureVersion();
}

std::vector<Entity> EditorCommands::RespawnObjects(Engine& engine, EcsWorld& world, const ObjectLifecycle& lifecycle)
{
    std::vector<Entity> roots;

    for (const auto& record : lifecycle.records)
    {
        if (record.states.empty())
            continue;

        if (record.assetPath.empty())
        {
            const Entity entity = world.CreateEntity();
            EditorState::AssignId(world, entity, record.states.front().id);
            roots.push_back(entity);
            continue;
        }

        std::vector<Entity> created;
        if (EntityFactory::CreateModelEntity(world, engine.GetRenderEngine(), record.assetPath, engine.GetShaders().lit, &created) == 0)
        {
            OGL_WARNING("EditorCommands | Cannot respawn " << record.assetPath)
            continue;
        }

        const std::string groupName = record.states.front().group.present ? record.states.front().group.value.name : SceneSerializer::MakeUniqueGroupName(world, record.assetPath);

        for (size_t i = 0; i < created.size(); i++)
        {
            ConfigurePlacedEntity(world, created[i], record.assetPath, groupName);
            if (i < record.states.size())
                EditorState::AssignId(world, created[i], record.states[i].id);
        }

        roots.push_back(created.front());
    }

    for (const auto& record : lifecycle.records)
        EditorState::Restore(world, record.states);

    for (const auto& link : lifecycle.children)
    {
        const Entity child = EditorState::Resolve(world, link.childId);
        if (child == INVALID_ENTITY)
            continue;

        auto& parent = world.AddComponent<ParentComponent>(child);
        parent.parent = EditorState::Resolve(world, link.parentId);
        parent.attachBone = link.attachBone;
    }

    engine.BumpSceneStructureVersion();
    return roots;
}

void EditorCommands::PushLifecycle(Engine& engine, const std::string& label, std::shared_ptr<ObjectLifecycle> lifecycle, bool createdByAction)
{
    EditorAction action;
    action.label = label;

    auto destroy = [lifecycle](Engine& e, EcsWorld& w) { DestroyObjects(e, w, *lifecycle); };
    auto respawn = [lifecycle](Engine& e, EcsWorld& w) { RespawnObjects(e, w, *lifecycle); };

    if (createdByAction)
    {
        action.undo = destroy;
        action.redo = respawn;
    }
    else
    {
        action.undo = respawn;
        action.redo = destroy;
    }

    engine.GetHistory().Push(std::move(action));
}

Entity EditorCommands::SpawnAsset(Engine& engine, EcsWorld& world, const std::string& assetPath)
{
    const Vector3 spawnPosition = SpawnPointInFrontOfCamera(world);

    std::vector<Entity> created;
    if (EntityFactory::CreateModelEntity(world, engine.GetRenderEngine(), assetPath, engine.GetShaders().lit, &created) == 0)
        return INVALID_ENTITY;

    const std::string groupName = SceneSerializer::MakeUniqueGroupName(world, assetPath);

    for (const auto entity : created)
    {
        ConfigurePlacedEntity(world, entity, assetPath, groupName);
        world.GetComponent<TransformComponent>(entity).position = spawnPosition;
    }

    auto lifecycle = std::make_shared<ObjectLifecycle>();
    lifecycle->records.push_back({assetPath, EditorState::Capture(world, created, StateAll)});
    PushLifecycle(engine, "Spawn " + groupName, lifecycle, true);
    engine.BumpSceneStructureVersion();

    return created.front();
}

void EditorCommands::DuplicateSelection(Engine& engine, EcsWorld& world)
{
    auto& selection = engine.GetSelection();
    if (!selection.HasSelection())
        return;

    auto lifecycle = std::make_shared<ObjectLifecycle>();
    std::unordered_map<uint64_t, uint64_t> remap;
    std::unordered_set<Entity> seen;
    std::unordered_set<std::string> reservedNames;

    for (const auto selected : selection.All())
    {
        if (seen.count(selected) || !world.HasComponent<PrefabComponent>(selected))
            continue;

        const std::vector<Entity> group = CollectGroup(world, selected);
        seen.insert(group.begin(), group.end());

        ObjectRecord record;
        record.assetPath = world.GetComponent<PrefabComponent>(selected).assetPath;
        record.states = EditorState::Capture(world, group, StateAll);

        for (auto& state : record.states)
            remap[state.id] = EditorState::NewId();

        lifecycle->records.push_back(std::move(record));
    }

    if (lifecycle->records.empty())
        return;

    for (auto& record : lifecycle->records)
    {
        std::string groupName = SceneSerializer::MakeUniqueGroupName(world, record.assetPath);
        while (reservedNames.count(groupName))
            groupName += "+";
        reservedNames.insert(groupName);

        for (auto& state : record.states)
        {
            const bool parentInCopy = state.hasParent && remap.count(state.parentId);

            state.id = remap[state.id];
            if (parentInCopy)
                state.parentId = remap[state.parentId];
            if (state.group.present)
                state.group.value.name = groupName;
            if (state.transform.present && !parentInCopy)
                state.transform.value.position += Vector3(1.0f, 0.0f, 0.0f);
        }
    }

    const std::vector<Entity> roots = RespawnObjects(engine, world, *lifecycle);
    if (roots.empty())
        return;

    PushLifecycle(engine, "Duplicate", lifecycle, true);

    selection.Clear();
    for (const auto root : roots)
        selection.Toggle(root);
}

void EditorCommands::DeleteSelection(Engine& engine, EcsWorld& world)
{
    auto& selection = engine.GetSelection();
    if (!selection.HasSelection())
        return;

    auto lifecycle = std::make_shared<ObjectLifecycle>();
    std::unordered_set<Entity> seen;

    for (const auto selected : selection.All())
    {
        if (seen.count(selected))
            continue;

        const std::vector<Entity> group = CollectGroup(world, selected);
        seen.insert(group.begin(), group.end());

        ObjectRecord record;
        if (world.HasComponent<PrefabComponent>(selected))
            record.assetPath = world.GetComponent<PrefabComponent>(selected).assetPath;
        else if (world.HasComponent<MeshComponent>(selected))
        {
            OGL_WARNING("EditorCommands | Skipping delete of scene-generated object (entity " << selected << "), it cannot be restored")
            continue;
        }

        for (const auto entity : group)
            record.states.push_back(EditorState::Capture(world, entity, StateAll));

        lifecycle->records.push_back(std::move(record));
    }

    if (lifecycle->records.empty())
        return;

    DestroyObjects(engine, world, *lifecycle);
    PushLifecycle(engine, "Delete", lifecycle, false);
    selection.Clear();
}

void EditorCommands::FocusSelection(Engine& engine, EcsWorld& world)
{
    auto& selection = engine.GetSelection();
    if (!selection.HasSelection() || !world.HasComponent<AABB>(selection.entity))
        return;

    const auto& aabb = world.GetComponent<AABB>(selection.entity);
    const Vector3 center = {(aabb.min.x + aabb.max.x) * 0.5f, (aabb.min.y + aabb.max.y) * 0.5f, (aabb.min.z + aabb.max.z) * 0.5f};

    const float sizeX = aabb.max.x - aabb.min.x;
    const float sizeY = aabb.max.y - aabb.min.y;
    const float sizeZ = aabb.max.z - aabb.min.z;
    const float radius = (std::max)(0.5f, (std::max)(sizeX, (std::max)(sizeY, sizeZ)));

    for (auto& [entity, camera] : world.GetPool<CameraComponent>())
    {
        if (!camera.isActive || !world.HasComponent<TransformComponent>(entity))
            continue;

        world.GetComponent<TransformComponent>(entity).position = center - camera.forward * (radius * 3.0f);
        break;
    }
}

void EditorCommands::AddLight(Engine& engine, EcsWorld& world)
{
    const Entity entity = world.CreateEntity();
    auto& light = world.AddComponent<LightComponent>(entity);
    light.type = LightType::Point;
    light.intensity = 1.0f;
    light.range = 15.0f;
    light.position = SpawnPointInFrontOfCamera(world);

    auto lifecycle = std::make_shared<ObjectLifecycle>();
    lifecycle->records.push_back({"", {EditorState::Capture(world, entity, StateAll)}});
    PushLifecycle(engine, "Add Light", lifecycle, true);
    engine.BumpSceneStructureVersion();
}

EditorAction EditorCommands::MakePropertyEdit(const std::string& label, std::vector<EntityState> before, std::vector<EntityState> after)
{
    EditorAction action;
    action.label = label;
    action.undo = [before = std::move(before)](Engine& e, EcsWorld& w)
    {
        EditorState::Restore(w, before);
        e.BumpSceneStructureVersion();
    };
    action.redo = [after = std::move(after)](Engine& e, EcsWorld& w)
    {
        EditorState::Restore(w, after);
        e.BumpSceneStructureVersion();
    };
    return action;
}

void EditorCommands::PushPropertyEdit(Engine& engine, const std::string& label, std::vector<EntityState> before, std::vector<EntityState> after)
{
    if (before.empty())
        return;

    engine.GetHistory().Push(MakePropertyEdit(label, std::move(before), std::move(after)));
}

void EditorCommands::Rename(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities, const std::string& newName)
{
    auto before = EditorState::Capture(world, entities, StateGroup);

    for (const auto entity : entities)
    {
        if (world.HasComponent<GroupComponent>(entity))
            world.GetComponent<GroupComponent>(entity).name = newName;
    }

    PushPropertyEdit(engine, "Rename", std::move(before), EditorState::Capture(world, entities, StateGroup));
    engine.BumpSceneStructureVersion();
}

void EditorCommands::Reparent(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities, Entity parentEntity)
{
    auto before = EditorState::Capture(world, entities, StateParent);

    for (const auto entity : entities)
    {
        auto& parentComp = world.AddComponent<ParentComponent>(entity);
        parentComp.parent = parentEntity;
        parentComp.attachBone.clear();
    }

    PushPropertyEdit(engine, "Reparent", std::move(before), EditorState::Capture(world, entities, StateParent));
    engine.BumpSceneStructureVersion();
}

void EditorCommands::Undo(Engine& engine, EcsWorld& world)
{
    if (engine.GetHistory().Undo(engine, world))
        engine.BumpSceneStructureVersion();
}

void EditorCommands::Redo(Engine& engine, EcsWorld& world)
{
    if (engine.GetHistory().Redo(engine, world))
        engine.BumpSceneStructureVersion();
}

bool EditorCommands::SaveScene(Engine& engine)
{
    const std::string& sceneName = engine.GetActiveSceneName();
    if (sceneName.empty())
        return false;

    if (!SceneSerializer::Save(engine, sceneName))
        return false;

    engine.GetHistory().MarkSaved();
    return true;
}
