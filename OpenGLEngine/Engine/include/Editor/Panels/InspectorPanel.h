#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "Ecs/Core/Entity.h"
#include "Editor/EditorUndoTracker.h"

class Engine;
class EcsWorld;
struct ColliderComponent;

class InspectorPanel
{
public:
    void DrawSelection(Engine& engine, EcsWorld& world);
    void DrawTransforms(Engine& engine, EcsWorld& world);

private:
    void DrawCollider(Engine& engine, EcsWorld& world, Entity entity);
    bool DrawLayerBits(const char* label, unsigned int& mask);
    static void FitColliderToMesh(EcsWorld& world, Entity entity, ColliderComponent& collider);

    void RebuildGroupsIfChanged(Engine& engine, EcsWorld& world);
    void DrawTransformGroup(Engine& engine, EcsWorld& world, const std::string& name, const std::vector<Entity>& entities);
    void DrawGroupTransform(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities, Entity keyEntity);
    void DrawGroupMaterial(Engine& engine, EcsWorld& world, const std::vector<Entity>& entities, Entity keyEntity);
    void DrawRenameField(Engine& engine, EcsWorld& world, const std::string& currentName, const std::vector<Entity>& entities, Entity keyEntity);
    void DrawEntityNameField(Engine& engine, EcsWorld& world, Entity entity);
    void DrawPartNode(Engine& engine, EcsWorld& world, Entity entity);

    std::unordered_map<std::string, std::vector<Entity>> groups;
    std::vector<Entity> ungrouped;

    unsigned int cachedVersion = 0;
    size_t cachedTransformCount = 0;
    bool built = false;

    std::string renameError;
    EditorUndoTracker undo;
};
