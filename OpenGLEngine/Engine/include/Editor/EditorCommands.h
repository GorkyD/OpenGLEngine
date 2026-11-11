#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "Ecs/Core/Entity.h"
#include "Ecs/Systems/EditorHistory.h"
#include "Ecs/Systems/EditorSelection.h"
#include "Editor/EditorState.h"

class EcsWorld;
class Engine;

struct ObjectRecord
{
    std::string assetPath;
    std::vector<EntityState> states;
};

struct ChildLink
{
    uint64_t childId = 0;
    uint64_t parentId = 0;
    std::string attachBone;
};

struct ObjectLifecycle
{
    std::vector<ObjectRecord> records;
    std::vector<ChildLink> children;
};

class EditorCommands
{
public:
    static Entity SpawnAsset(Engine& engine, EcsWorld& world, const std::string& assetPath);
    static void DuplicateSelection(Engine& engine, EcsWorld& world);
    static void DeleteSelection(Engine& engine, EcsWorld& world);
    static void FocusSelection(Engine& engine, EcsWorld& world);
    static void AddLight(Engine& engine, EcsWorld& world);

    static void Rename(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities, const std::string& newName);
    static void Reparent(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities, Entity parentEntity);

    static EditorAction MakePropertyEdit(const std::string& label, std::vector<EntityState> before, std::vector<EntityState> after);
    static void PushPropertyEdit(Engine& engine, const std::string& label, std::vector<EntityState> before, std::vector<EntityState> after);

    static void Undo(Engine& engine, EcsWorld& world);
    static void Redo(Engine& engine, EcsWorld& world);

    static bool SaveScene(Engine& engine);

    static std::vector<Entity> CollectGroup(EcsWorld& world, Entity entity);
    static std::vector<Entity> CollectSelectedGroups(Engine& engine, EcsWorld& world);

private:
    static void DestroyObjects(Engine& engine, EcsWorld& world, ObjectLifecycle& lifecycle);
    static std::vector<Entity> RespawnObjects(Engine& engine, EcsWorld& world, const ObjectLifecycle& lifecycle);
    static void PushLifecycle(Engine& engine, const std::string& label, std::shared_ptr<ObjectLifecycle> lifecycle, bool createdByAction);
};
