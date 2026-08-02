#pragma once

#include <string>
#include <vector>
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Core/Entity.h"
#include "Math/Vector3.h"

struct TransformSnapshot
{
    Entity entity = INVALID_ENTITY;
    Vector3 position = {0, 0, 0};
    Vector3 scale = {1, 1, 1};
    Vector3 rotationDegrees = {0, 0, 0};
};

struct SpawnedObject
{
    std::string assetPath;
    Vector3 position = {0, 0, 0};
    Vector3 scale = {1, 1, 1};
    Vector3 rotationDegrees = {0, 0, 0};
};

struct MaterialSnapshot
{
    Entity entity = INVALID_ENTITY;
    MaterialComponent material;
};

struct ParentSnapshot
{
    Entity entity = INVALID_ENTITY;
    Entity parent = INVALID_ENTITY;
    std::string attachBone;
};

struct EditorAction
{
    enum class Type
    {
        Transform,
        Spawn,
        Delete,
        Rename,
        Reparent,
        Material
    };

    Type type = Type::Transform;

    std::vector<TransformSnapshot> before;
    std::vector<TransformSnapshot> after;

    std::vector<SpawnedObject> objects;
    std::vector<Entity> entities;

    std::string nameBefore;
    std::string nameAfter;

    std::vector<ParentSnapshot> parentsBefore;
    std::vector<ParentSnapshot> parentsAfter;

    std::vector<MaterialSnapshot> materialsBefore;
    std::vector<MaterialSnapshot> materialsAfter;
};

class EditorHistory
{
public:
    void Push(EditorAction action)
    {
        undoStack.push_back(std::move(action));
        redoStack.clear();

        if (undoStack.size() > maxEntries)
            undoStack.erase(undoStack.begin());
    }

    bool CanUndo() const
    {
        return !undoStack.empty();
    }
    bool CanRedo() const
    {
        return !redoStack.empty();
    }

    bool PopUndo(EditorAction& outAction)
    {
        if (undoStack.empty())
            return false;

        outAction = undoStack.back();
        undoStack.pop_back();
        redoStack.push_back(outAction);
        return true;
    }

    bool PopRedo(EditorAction& outAction)
    {
        if (redoStack.empty())
            return false;

        outAction = redoStack.back();
        redoStack.pop_back();
        undoStack.push_back(outAction);
        return true;
    }

    void Clear()
    {
        undoStack.clear();
        redoStack.clear();
    }

private:
    static constexpr size_t maxEntries = 64;

    std::vector<EditorAction> undoStack;
    std::vector<EditorAction> redoStack;
};
