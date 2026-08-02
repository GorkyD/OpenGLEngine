#pragma once

#include <string>
#include <vector>
#include "Ecs/Core/Entity.h"
#include "Ecs/Systems/EditorHistory.h"
#include "Ecs/Systems/EditorSelection.h"

class EcsWorld;
class Engine;

class EditorCommands
{
public:
    static Entity SpawnAsset(Engine& engine, EcsWorld& world, const std::string& assetPath);
    static void DuplicateSelection(Engine& engine, EcsWorld& world);
    static void DeleteSelection(Engine& engine, EcsWorld& world);
    static void FocusSelection(Engine& engine, EcsWorld& world);
    static void AddLight(EcsWorld& world);

    static void Rename(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities, const std::string& currentName, const std::string& newName);
    static void Reparent(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities, Entity parentEntity);

    static void Undo(Engine& engine, EcsWorld& world);
    static void Redo(Engine& engine, EcsWorld& world);

    static std::vector<TransformSnapshot> CaptureTransforms(EcsWorld& world, const std::vector<Entity>& entities);
    static std::vector<ParentSnapshot> CaptureParents(EcsWorld& world, const std::vector<Entity>& entities);
    static std::vector<MaterialSnapshot> CaptureMaterials(EcsWorld& world, const std::vector<Entity>& entities);

    static std::vector<Entity> CollectGroup(EcsWorld& world, Entity entity);

private:
    static void ApplyTransforms(EcsWorld& world, const std::vector<TransformSnapshot>& snapshots);
    static void ApplyName(EcsWorld& world, const std::vector<Entity>& entities, const std::string& name);
    static void ApplyParents(EcsWorld& world, const std::vector<ParentSnapshot>& snapshots);
    static void ApplyMaterials(EcsWorld& world, const std::vector<MaterialSnapshot>& snapshots);

    static void DestroyEntities(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities);
    static void RespawnObjects(Engine& engine, EcsWorld& world, EditorAction& action);

    static void ApplyAction(Engine& engine, EcsWorld& world, EditorAction& action, bool undo);
};
