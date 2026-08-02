#pragma once

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include "Ecs/Components/AABB.h"
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/GroupComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/TransformOffsetComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Ecs/Systems/EditorHistory.h"
#include "Ecs/Systems/EditorSelection.h"
#include "Editor/EditorCommands.h"
#include "Input/InputSystem.h"
#include "Math/Vector3.h"
#include "Physics/Physics.h"
#include "Window/Window.h"

class EditorPickSystem : public IEcsSystem
{
public:
    EditorPickSystem(InputSystem* inputSystem, Window* window, EditorSelection* selection, EditorHistory* history) :
        inputSystem(inputSystem), window(window), selection(selection), history(history)
    {
    }

    bool enabled = true;
    float handleLength = 1.5f;
    float handleGrabRadius = 0.25f;

    void Run(EcsWorld& world, float deltaTime) override
    {
        if (!enabled)
        {
            wasMouseDown = false;
            return;
        }

        const bool mouseDown = inputSystem->IsLeftMouseDown() && !ImGui::GetIO().WantCaptureMouse;

        Vector3 rayOrigin, rayDirection;
        if (!BuildMouseRay(world, rayOrigin, rayDirection))
        {
            wasMouseDown = mouseDown;
            return;
        }

        if (mouseDown && !wasMouseDown)
            OnMousePressed(world, rayOrigin, rayDirection);
        else if (mouseDown && selection->draggingAxis != GizmoAxis::None)
            UpdateDrag(world, rayOrigin, rayDirection);
        else if (!mouseDown && wasMouseDown)
            EndDrag(world);

        wasMouseDown = mouseDown;
    }

private:
    void OnMousePressed(EcsWorld& world, const Vector3& rayOrigin, const Vector3& rayDirection)
    {
        if (selection->HasSelection() && world.HasComponent<TransformComponent>(selection->entity))
        {
            const Vector3 origin = world.GetComponent<TransformComponent>(selection->entity).position;
            const GizmoAxis axis = PickHandle(origin, rayOrigin, rayDirection);
            if (axis != GizmoAxis::None)
            {
                auto& transform = world.GetComponent<TransformComponent>(selection->entity);

                selection->draggingAxis = axis;
                selection->dragStartPosition = origin;
                selection->dragStartScale = transform.scale;
                selection->dragStartRotation = world.HasComponent<TransformOffsetComponent>(selection->entity)
                                                   ? world.GetComponent<TransformOffsetComponent>(selection->entity).rotationDegrees
                                                   : Vector3(0.0f, 0.0f, 0.0f);
                selection->dragStartProjection = ProjectOntoAxis(origin, AxisVector(axis), rayOrigin, rayDirection);

                dragBefore = CaptureSelection(world);
                return;
            }
        }

        const Entity picked = PickEntity(world, rayOrigin, rayDirection);

        if (ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift)
        {
            if (picked != INVALID_ENTITY)
                selection->Toggle(picked);
        }
        else
        {
            selection->additional.clear();
            selection->entity = picked;
        }

        selection->draggingAxis = GizmoAxis::None;
    }

    void UpdateDrag(EcsWorld& world, const Vector3& rayOrigin, const Vector3& rayDirection)
    {
        if (!selection->HasSelection() || !world.HasComponent<TransformComponent>(selection->entity))
            return;

        const Vector3 axis = AxisVector(selection->draggingAxis);
        const float projection = ProjectOntoAxis(selection->dragStartPosition, axis, rayOrigin, rayDirection);
        const float delta = projection - selection->dragStartProjection;

        if (selection->mode == GizmoMode::Move)
        {
            float amount = delta;
            if (selection->snapEnabled)
                amount = Snap(amount, selection->moveSnap);
            MoveSelection(world, selection->dragStartPosition + axis * amount);
        }
        else if (selection->mode == GizmoMode::Scale)
        {
            float amount = delta;
            if (selection->snapEnabled)
                amount = Snap(amount, selection->scaleSnap);

            Vector3 scale = selection->dragStartScale;
            if (selection->draggingAxis == GizmoAxis::X)
                scale.x = (std::max)(0.001f, scale.x + amount);
            else if (selection->draggingAxis == GizmoAxis::Y)
                scale.y = (std::max)(0.001f, scale.y + amount);
            else
                scale.z = (std::max)(0.001f, scale.z + amount);

            ApplyToGroup(world, [&](Entity entity) { world.GetComponent<TransformComponent>(entity).scale = scale; });
        }
        else
        {
            float degrees = delta * 45.0f;
            if (selection->snapEnabled)
                degrees = Snap(degrees, selection->rotateSnap);

            Vector3 rotation = selection->dragStartRotation;
            if (selection->draggingAxis == GizmoAxis::X)
                rotation.x += degrees;
            else if (selection->draggingAxis == GizmoAxis::Y)
                rotation.y += degrees;
            else
                rotation.z += degrees;

            ApplyToGroup(world,
                         [&](Entity entity)
                         {
                             if (!world.HasComponent<TransformOffsetComponent>(entity))
                                 return;
                             auto& offset = world.GetComponent<TransformOffsetComponent>(entity);
                             offset.rotationDegrees = rotation;
                             world.GetComponent<TransformComponent>(entity).rotation = offset.GetRotationMatrix();
                         });
        }
    }

    void EndDrag(EcsWorld& world)
    {
        if (selection->draggingAxis != GizmoAxis::None && history && selection->HasSelection())
        {
            EditorAction action;
            action.type = EditorAction::Type::Transform;
            action.before = dragBefore;
            action.after = CaptureSelection(world);
            if (!action.before.empty())
                history->Push(std::move(action));
        }

        dragBefore.clear();
        selection->draggingAxis = GizmoAxis::None;
    }

    static float Snap(float value, float step)
    {
        if (step <= 0.0001f)
            return value;
        return std::round(value / step) * step;
    }

    std::vector<Entity> SelectedEntities(EcsWorld& world) const
    {
        std::vector<Entity> entities;

        for (const auto selected : selection->All())
        {
            for (const auto entity : EditorCommands::CollectGroup(world, selected))
            {
                if (world.HasComponent<TransformComponent>(entity))
                    entities.push_back(entity);
            }
        }

        return entities;
    }

    template <typename Fn> void ApplyToGroup(EcsWorld& world, Fn&& fn)
    {
        for (const auto entity : SelectedEntities(world))
            fn(entity);
    }

    std::vector<TransformSnapshot> CaptureSelection(EcsWorld& world)
    {
        return EditorCommands::CaptureTransforms(world, SelectedEntities(world));
    }


    void MoveSelection(EcsWorld& world, const Vector3& newPosition)
    {
        const Vector3 delta = newPosition - world.GetComponent<TransformComponent>(selection->entity).position;
        ApplyToGroup(world, [&](Entity entity) { world.GetComponent<TransformComponent>(entity).position += delta; });
    }

    Entity PickEntity(EcsWorld& world, const Vector3& rayOrigin, const Vector3& rayDirection)
    {
        Entity best = INVALID_ENTITY;
        float bestDistance = 0.0f;

        for (auto& [entity, aabb] : world.GetPool<AABB>())
        {
            if (aabb.min.x == aabb.max.x && aabb.min.y == aabb.max.y && aabb.min.z == aabb.max.z)
                continue;

            float distance = 0.0f;
            if (!RayIntersectsAabb(rayOrigin, rayDirection, aabb, distance))
                continue;

            if (best == INVALID_ENTITY || distance < bestDistance)
            {
                best = entity;
                bestDistance = distance;
            }
        }

        return best;
    }

    GizmoAxis PickHandle(const Vector3& origin, const Vector3& rayOrigin, const Vector3& rayDirection) const
    {
        if (selection->mode == GizmoMode::Rotate)
            return PickRing(origin, rayOrigin, rayDirection);

        GizmoAxis best = GizmoAxis::None;
        float bestDistance = handleGrabRadius;

        const GizmoAxis axes[3] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};
        for (const auto axis : axes)
        {
            const Vector3 tip = origin + AxisVector(axis) * handleLength;
            const float distance = SegmentRayDistance(origin, tip, rayOrigin, rayDirection);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                best = axis;
            }
        }

        return best;
    }

    GizmoAxis PickRing(const Vector3& origin, const Vector3& rayOrigin, const Vector3& rayDirection) const
    {
        constexpr int segments = 32;
        constexpr float twoPi = 6.28318531f;

        GizmoAxis best = GizmoAxis::None;
        float bestDistance = handleGrabRadius;

        const GizmoAxis axes[3] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};

        for (int axisIndex = 0; axisIndex < 3; axisIndex++)
        {
            for (int i = 0; i < segments; i++)
            {
                const Vector3 p0 = origin + RingPoint(axisIndex, (twoPi * i) / segments, handleLength);
                const Vector3 p1 = origin + RingPoint(axisIndex, (twoPi * (i + 1)) / segments, handleLength);

                const float distance = SegmentRayDistance(p0, p1, rayOrigin, rayDirection);
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    best = axes[axisIndex];
                }
            }
        }

        return best;
    }

    static Vector3 RingPoint(int axis, float angle, float radius)
    {
        const float c = std::cos(angle) * radius;
        const float s = std::sin(angle) * radius;

        if (axis == 0)
            return {0.0f, c, s};
        if (axis == 1)
            return {c, 0.0f, s};
        return {c, s, 0.0f};
    }

    bool BuildMouseRay(EcsWorld& world, Vector3& outOrigin, Vector3& outDirection) const
    {
        for (auto& [entity, camera] : world.GetPool<CameraComponent>())
        {
            if (!camera.isActive || !world.HasComponent<TransformComponent>(entity))
                continue;

            const Rect size = window->GetInnerSize();
            if (size.width <= 0 || size.height <= 0)
                return false;

            const float ndcX = (2.0f * inputSystem->GetMouseX() / static_cast<float>(size.width)) - 1.0f;
            const float ndcY = 1.0f - (2.0f * inputSystem->GetMouseY() / static_cast<float>(size.height));

            const float aspect = static_cast<float>(size.width) / static_cast<float>(size.height);
            const float tanHalfFov = std::tan(camera.fovY * 0.5f);

            outOrigin = world.GetComponent<TransformComponent>(entity).position;
            outDirection = Vector3::Normalize(camera.forward + camera.right * (ndcX * tanHalfFov * aspect) + camera.up * (ndcY * tanHalfFov));
            return true;
        }

        return false;
    }

    static Vector3 AxisVector(GizmoAxis axis)
    {
        if (axis == GizmoAxis::X)
            return {1, 0, 0};
        if (axis == GizmoAxis::Y)
            return {0, 1, 0};
        return {0, 0, 1};
    }

    static float ProjectOntoAxis(const Vector3& axisOrigin, const Vector3& axis, const Vector3& rayOrigin, const Vector3& rayDirection)
    {
        const Vector3 w = axisOrigin - rayOrigin;
        const float a = Vector3::Dot(axis, axis);
        const float b = Vector3::Dot(axis, rayDirection);
        const float c = Vector3::Dot(rayDirection, rayDirection);
        const float d = Vector3::Dot(axis, w);
        const float e = Vector3::Dot(rayDirection, w);

        const float denominator = a * c - b * b;
        if (std::fabs(denominator) < 0.0001f)
            return 0.0f;

        return (b * e - c * d) / denominator;
    }

    static float SegmentRayDistance(const Vector3& segmentStart, const Vector3& segmentEnd, const Vector3& rayOrigin, const Vector3& rayDirection)
    {
        const Vector3 axis = segmentEnd - segmentStart;
        const float length = std::sqrt(Vector3::Dot(axis, axis));
        if (length < 0.0001f)
            return 1e9f;

        const Vector3 direction = axis / length;
        float t = ProjectOntoAxis(segmentStart, direction, rayOrigin, rayDirection);
        t = (std::max)(0.0f, (std::min)(length, t));

        const Vector3 pointOnSegment = segmentStart + direction * t;
        const Vector3 toPoint = pointOnSegment - rayOrigin;
        const float rayT = (std::max)(0.0f, Vector3::Dot(toPoint, rayDirection));
        const Vector3 pointOnRay = rayOrigin + rayDirection * rayT;
        const Vector3 diff = pointOnSegment - pointOnRay;

        return std::sqrt(Vector3::Dot(diff, diff));
    }

    static bool RayIntersectsAabb(const Vector3& origin, const Vector3& direction, const AABB& aabb, float& outDistance)
    {
        Vector3 normal;
        return Physics::RayIntersectsBox(origin, direction, aabb.min, aabb.max, 1e9f, outDistance, normal);
    }

    InputSystem* inputSystem;
    Window* window;
    EditorSelection* selection;
    EditorHistory* history;

    std::vector<TransformSnapshot> dragBefore;
    bool wasMouseDown = false;
};
