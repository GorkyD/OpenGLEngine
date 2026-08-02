#pragma once

#include <algorithm>
#include <vector>
#include "Ecs/Core/Entity.h"
#include "Math/Vector3.h"

enum class GizmoAxis
{
    None,
    X,
    Y,
    Z
};

enum class GizmoMode
{
    Move,
    Rotate,
    Scale
};

struct EditorSelection
{
    Entity entity = INVALID_ENTITY;
    GizmoMode mode = GizmoMode::Move;

    GizmoAxis draggingAxis = GizmoAxis::None;
    Vector3 dragStartPosition = {0, 0, 0};
    Vector3 dragStartScale = {1, 1, 1};
    Vector3 dragStartRotation = {0, 0, 0};
    float dragStartProjection = 0.0f;

    bool snapEnabled = false;
    float moveSnap = 0.5f;
    float rotateSnap = 15.0f;
    float scaleSnap = 0.25f;

    std::vector<Entity> additional;

    bool HasSelection() const
    {
        return entity != INVALID_ENTITY;
    }
    void Clear()
    {
        entity = INVALID_ENTITY;
        additional.clear();
        draggingAxis = GizmoAxis::None;
    }

    bool Contains(Entity value) const
    {
        return value == entity || std::find(additional.begin(), additional.end(), value) != additional.end();
    }

    void Toggle(Entity value)
    {
        if (value == INVALID_ENTITY)
            return;

        if (entity == INVALID_ENTITY)
        {
            entity = value;
            return;
        }

        if (value == entity)
        {
            if (additional.empty())
            {
                entity = INVALID_ENTITY;
                return;
            }

            entity = additional.back();
            additional.pop_back();
            return;
        }

        const auto it = std::find(additional.begin(), additional.end(), value);
        if (it != additional.end())
            additional.erase(it);
        else
            additional.push_back(value);
    }

    std::vector<Entity> All() const
    {
        std::vector<Entity> result;
        if (entity != INVALID_ENTITY)
            result.push_back(entity);
        result.insert(result.end(), additional.begin(), additional.end());
        return result;
    }
};
