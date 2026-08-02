#include "Editor/EditorCommands.h"
#include <algorithm>
#include "Ecs/Components/AABB.h"
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/GroupComponent.h"
#include "Ecs/Components/HitFlashComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/ParentComponent.h"
#include "Ecs/Components/PrefabComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/TransformOffsetComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Engine/Engine.h"
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

static void ConfigurePlacedEntity(EcsWorld& world, Entity entity, const std::string& assetPath, const std::string& groupName)
{
    world.AddComponent<PrefabComponent>(entity).assetPath = assetPath;
    world.AddComponent<GroupComponent>(entity).name = groupName;
    world.AddComponent<TransformOffsetComponent>(entity);
    world.AddComponent<HitFlashComponent>(entity);
}

Entity EditorCommands::SpawnAsset(Engine& engine, EcsWorld& world, const std::string& assetPath)
{
    Vector3 spawnPosition = {0, 0, 0};
    for (auto& [entity, camera] : world.GetPool<CameraComponent>())
    {
        if (!camera.isActive || !world.HasComponent<TransformComponent>(entity))
            continue;

        spawnPosition = world.GetComponent<TransformComponent>(entity).position + camera.forward * 5.0f;
        break;
    }

    std::vector<Entity> created;
    if (EntityFactory::CreateModelEntity(world, engine.GetRenderEngine(), assetPath, engine.GetShaders().lit, &created) == 0)
        return INVALID_ENTITY;

    const std::string groupName = SceneSerializer::MakeUniqueGroupName(world, assetPath);

    for (const auto entity : created)
    {
        ConfigurePlacedEntity(world, entity, assetPath, groupName);
        world.GetComponent<TransformComponent>(entity).position = spawnPosition;
    }

    EditorAction action;
    action.type = EditorAction::Type::Spawn;
    action.entities = created;

    SpawnedObject spawned;
    spawned.assetPath = assetPath;
    spawned.position = spawnPosition;
    action.objects.push_back(spawned);

    engine.GetHistory().Push(std::move(action));
    engine.BumpSceneStructureVersion();

    return created.empty() ? INVALID_ENTITY : created.front();
}

void EditorCommands::DuplicateSelection(Engine& engine, EcsWorld& world)
{
    auto& selection = engine.GetSelection();
    if (!selection.HasSelection() || !world.HasComponent<PrefabComponent>(selection.entity))
        return;

    const Entity source = selection.entity;
    const std::string assetPath = world.GetComponent<PrefabComponent>(source).assetPath;
    const auto sourceTransform = world.GetComponent<TransformComponent>(source);

    std::vector<Entity> created;
    if (EntityFactory::CreateModelEntity(world, engine.GetRenderEngine(), assetPath, engine.GetShaders().lit, &created) == 0)
        return;

    const std::string groupName = SceneSerializer::MakeUniqueGroupName(world, assetPath);
    const Vector3 offsetPosition = sourceTransform.position + Vector3(1.0f, 0.0f, 0.0f);

    for (const auto entity : created)
    {
        ConfigurePlacedEntity(world, entity, assetPath, groupName);

        auto& offset = world.GetComponent<TransformOffsetComponent>(entity);
        if (world.HasComponent<TransformOffsetComponent>(source))
            offset.rotationDegrees = world.GetComponent<TransformOffsetComponent>(source).rotationDegrees;

        auto& transform = world.GetComponent<TransformComponent>(entity);
        transform.position = offsetPosition;
        transform.scale = sourceTransform.scale;
        transform.rotation = offset.GetRotationMatrix();

        if (world.HasComponent<MaterialComponent>(source) && world.HasComponent<MaterialComponent>(entity))
            world.GetComponent<MaterialComponent>(entity) = world.GetComponent<MaterialComponent>(source);
    }

    EditorAction action;
    action.type = EditorAction::Type::Spawn;
    action.entities = created;

    SpawnedObject spawned;
    spawned.assetPath = assetPath;
    spawned.position = offsetPosition;
    spawned.scale = sourceTransform.scale;
    action.objects.push_back(spawned);

    engine.GetHistory().Push(std::move(action));
    engine.BumpSceneStructureVersion();

    selection.additional.clear();
    selection.entity = created.front();
}

void EditorCommands::DeleteSelection(Engine& engine, EcsWorld& world)
{
    auto& selection = engine.GetSelection();
    if (!selection.HasSelection())
        return;

    const std::vector<Entity> toDelete = CollectGroup(world, selection.entity);

    EditorAction action;
    action.type = EditorAction::Type::Delete;
    action.entities = toDelete;

    for (const auto entity : toDelete)
    {
        if (!world.HasComponent<PrefabComponent>(entity))
            continue;

        SpawnedObject object;
        object.assetPath = world.GetComponent<PrefabComponent>(entity).assetPath;
        object.position = world.GetComponent<TransformComponent>(entity).position;
        object.scale = world.GetComponent<TransformComponent>(entity).scale;
        if (world.HasComponent<TransformOffsetComponent>(entity))
            object.rotationDegrees = world.GetComponent<TransformOffsetComponent>(entity).rotationDegrees;

        action.objects.push_back(object);
        break;
    }

    for (const auto entity : toDelete)
        world.DestroyEntity(entity);

    engine.GetHistory().Push(std::move(action));
    engine.BumpSceneStructureVersion();
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

void EditorCommands::AddLight(EcsWorld& world)
{
    const Entity entity = world.CreateEntity();
    auto& light = world.AddComponent<LightComponent>(entity);
    light.type = LightType::Point;
    light.intensity = 1.0f;
    light.range = 15.0f;

    for (auto& [cameraEntity, camera] : world.GetPool<CameraComponent>())
    {
        if (!camera.isActive || !world.HasComponent<TransformComponent>(cameraEntity))
            continue;

        light.position = world.GetComponent<TransformComponent>(cameraEntity).position + camera.forward * 5.0f;
        break;
    }
}

void EditorCommands::Rename(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities, const std::string& currentName, const std::string& newName)
{
    EditorAction action;
    action.type = EditorAction::Type::Rename;
    action.entities = entities;
    action.nameBefore = currentName;
    action.nameAfter = newName;
    engine.GetHistory().Push(std::move(action));
    engine.BumpSceneStructureVersion();

    ApplyName(world, entities, newName);
}

void EditorCommands::Reparent(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities, Entity parentEntity)
{
    EditorAction action;
    action.type = EditorAction::Type::Reparent;
    action.entities = entities;
    action.parentsBefore = CaptureParents(world, entities);

    for (const auto entity : entities)
    {
        auto& parentComp = world.HasComponent<ParentComponent>(entity) ? world.GetComponent<ParentComponent>(entity) : world.AddComponent<ParentComponent>(entity);
        parentComp.parent = parentEntity;
        parentComp.attachBone.clear();
    }

    action.parentsAfter = CaptureParents(world, entities);
    engine.GetHistory().Push(std::move(action));
    engine.BumpSceneStructureVersion();
}

std::vector<TransformSnapshot> EditorCommands::CaptureTransforms(EcsWorld& world, const std::vector<Entity>& entities)
{
    std::vector<TransformSnapshot> snapshots;

    for (const auto entity : entities)
    {
        if (!world.HasComponent<TransformComponent>(entity))
            continue;

        TransformSnapshot snapshot;
        snapshot.entity = entity;
        snapshot.position = world.GetComponent<TransformComponent>(entity).position;
        snapshot.scale = world.GetComponent<TransformComponent>(entity).scale;
        if (world.HasComponent<TransformOffsetComponent>(entity))
            snapshot.rotationDegrees = world.GetComponent<TransformOffsetComponent>(entity).rotationDegrees;

        snapshots.push_back(snapshot);
    }

    return snapshots;
}

std::vector<ParentSnapshot> EditorCommands::CaptureParents(EcsWorld& world, const std::vector<Entity>& entities)
{
    std::vector<ParentSnapshot> snapshots;

    for (const auto entity : entities)
    {
        ParentSnapshot snapshot;
        snapshot.entity = entity;
        if (world.HasComponent<ParentComponent>(entity))
        {
            snapshot.parent = world.GetComponent<ParentComponent>(entity).parent;
            snapshot.attachBone = world.GetComponent<ParentComponent>(entity).attachBone;
        }
        snapshots.push_back(snapshot);
    }

    return snapshots;
}

std::vector<MaterialSnapshot> EditorCommands::CaptureMaterials(EcsWorld& world, const std::vector<Entity>& entities)
{
    std::vector<MaterialSnapshot> snapshots;

    for (const auto entity : entities)
    {
        if (!world.HasComponent<MaterialComponent>(entity))
            continue;

        MaterialSnapshot snapshot;
        snapshot.entity = entity;
        snapshot.material = world.GetComponent<MaterialComponent>(entity);
        snapshots.push_back(snapshot);
    }

    return snapshots;
}

void EditorCommands::ApplyTransforms(EcsWorld& world, const std::vector<TransformSnapshot>& snapshots)
{
    for (const auto& snapshot : snapshots)
    {
        if (!world.HasComponent<TransformComponent>(snapshot.entity))
            continue;

        auto& transform = world.GetComponent<TransformComponent>(snapshot.entity);
        transform.position = snapshot.position;
        transform.scale = snapshot.scale;

        if (world.HasComponent<TransformOffsetComponent>(snapshot.entity))
        {
            auto& offset = world.GetComponent<TransformOffsetComponent>(snapshot.entity);
            offset.rotationDegrees = snapshot.rotationDegrees;
            transform.rotation = offset.GetRotationMatrix();
        }
    }
}

void EditorCommands::ApplyName(EcsWorld& world, const std::vector<Entity>& entities, const std::string& name)
{
    for (const auto entity : entities)
    {
        if (world.HasComponent<GroupComponent>(entity))
            world.GetComponent<GroupComponent>(entity).name = name;
    }
}

void EditorCommands::ApplyParents(EcsWorld& world, const std::vector<ParentSnapshot>& snapshots)
{
    for (const auto& snapshot : snapshots)
    {
        auto& parentComp =
            world.HasComponent<ParentComponent>(snapshot.entity) ? world.GetComponent<ParentComponent>(snapshot.entity) : world.AddComponent<ParentComponent>(snapshot.entity);
        parentComp.parent = snapshot.parent;
        parentComp.attachBone = snapshot.attachBone;
    }
}

void EditorCommands::ApplyMaterials(EcsWorld& world, const std::vector<MaterialSnapshot>& snapshots)
{
    for (const auto& snapshot : snapshots)
    {
        if (world.HasComponent<MaterialComponent>(snapshot.entity))
            world.GetComponent<MaterialComponent>(snapshot.entity) = snapshot.material;
    }
}

void EditorCommands::DestroyEntities(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities)
{
    for (const auto entity : entities)
        world.DestroyEntity(entity);

    engine.GetSelection().Clear();
}

void EditorCommands::RespawnObjects(Engine& engine, EcsWorld& world, EditorAction& action)
{
    action.entities.clear();

    for (const auto& object : action.objects)
    {
        std::vector<Entity> created;
        if (EntityFactory::CreateModelEntity(world, engine.GetRenderEngine(), object.assetPath, engine.GetShaders().lit, &created) == 0)
            continue;

        const std::string groupName = SceneSerializer::MakeUniqueGroupName(world, object.assetPath);

        for (const auto entity : created)
        {
            ConfigurePlacedEntity(world, entity, object.assetPath, groupName);

            auto& offset = world.GetComponent<TransformOffsetComponent>(entity);
            offset.rotationDegrees = object.rotationDegrees;

            auto& transform = world.GetComponent<TransformComponent>(entity);
            transform.position = object.position;
            transform.scale = object.scale;
            transform.rotation = offset.GetRotationMatrix();

            action.entities.push_back(entity);
        }
    }
}

void EditorCommands::ApplyAction(Engine& engine, EcsWorld& world, EditorAction& action, bool undo)
{
    switch (action.type)
    {
    case EditorAction::Type::Transform:
        ApplyTransforms(world, undo ? action.before : action.after);
        break;
    case EditorAction::Type::Spawn:
        if (undo)
            DestroyEntities(engine, world, action.entities);
        else
            RespawnObjects(engine, world, action);
        break;
    case EditorAction::Type::Delete:
        if (undo)
            RespawnObjects(engine, world, action);
        else
            DestroyEntities(engine, world, action.entities);
        break;
    case EditorAction::Type::Rename:
        ApplyName(world, action.entities, undo ? action.nameBefore : action.nameAfter);
        break;
    case EditorAction::Type::Reparent:
        ApplyParents(world, undo ? action.parentsBefore : action.parentsAfter);
        break;
    case EditorAction::Type::Material:
        ApplyMaterials(world, undo ? action.materialsBefore : action.materialsAfter);
        break;
    }
}

void EditorCommands::Undo(Engine& engine, EcsWorld& world)
{
    EditorAction action;
    if (!engine.GetHistory().PopUndo(action))
        return;

    ApplyAction(engine, world, action, true);
    engine.BumpSceneStructureVersion();
}

void EditorCommands::Redo(Engine& engine, EcsWorld& world)
{
    EditorAction action;
    if (!engine.GetHistory().PopRedo(action))
        return;

    ApplyAction(engine, world, action, false);
    engine.BumpSceneStructureVersion();
}
