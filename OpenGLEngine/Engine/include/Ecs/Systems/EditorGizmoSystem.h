#pragma once

#include <vector>
#include <glad/glad.h>
#include "Ecs/Components/AABB.h"
#include "Ecs/Components/ColliderComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/WorldTransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Ecs/Systems/EditorSelection.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include "Render/RenderEngine.h"
#include "Render/UniformBuffer.h"

struct GizmoVertex
{
    Vector3 position;
    Vector3 color;
};

class EditorGizmoSystem : public IEcsSystem
{
public:
    EditorGizmoSystem(RenderEngine* renderEngine, const UniformBufferPtr& uniformBuffer, const ShaderProgramPtr& lineShader, EditorSelection* selection) :
        renderEngine(renderEngine), uniformBuffer(uniformBuffer), lineShader(lineShader), selection(selection)
    {
    }

    bool showGrid = true;
    bool showAxes = true;
    bool showCollisions = true;
    bool enabled = true;

    void Init(EcsWorld& world) override
    {
        BuildGrid();
        BuildWireCube();
        BuildMoveHandles();
        BuildRotationRings();
    }

    void Run(EcsWorld& world, float deltaTime) override
    {
        if (!enabled || !lineShader)
            return;

        renderEngine->SetShaderProgram(lineShader);

        const Matrix4 identity;
        uniformBuffer->SetSubData(const_cast<Matrix4*>(&identity), 0, sizeof(Matrix4));

        if (showGrid && gridVao)
        {
            renderEngine->SetVertexArrayObject(gridVao);
            renderEngine->DrawLines(gridVertexCount);
        }

        if (showAxes && axisVao)
        {
            glDisable(GL_DEPTH_TEST);
            renderEngine->SetVertexArrayObject(axisVao);
            renderEngine->DrawLines(axisVertexCount);
            glEnable(GL_DEPTH_TEST);
        }

        if (showCollisions && wireCubeVao)
            DrawCollisionBounds(world);

        DrawSelection(world);
    }

private:
    static Vector3 EntityWorldPosition(EcsWorld& world, Entity entity)
    {
        if (world.HasComponent<WorldTransformComponent>(entity))
        {
            const auto& m = world.GetComponent<WorldTransformComponent>(entity).matrix;
            return {m.matrix[3][0], m.matrix[3][1], m.matrix[3][2]};
        }

        return world.GetComponent<TransformComponent>(entity).position;
    }

    void DrawSelection(EcsWorld& world)
    {
        if (!selection || !selection->HasSelection() || !world.HasComponent<TransformComponent>(selection->entity))
            return;

        const Entity entity = selection->entity;

        for (const auto selected : selection->All())
        {
            if (!selectionCubeVao || !world.HasComponent<AABB>(selected))
                continue;

            const auto& aabb = world.GetComponent<AABB>(selected);
            const Vector3 size = {aabb.max.x - aabb.min.x, aabb.max.y - aabb.min.y, aabb.max.z - aabb.min.z};
            const Vector3 center = {(aabb.min.x + aabb.max.x) * 0.5f, (aabb.min.y + aabb.max.y) * 0.5f, (aabb.min.z + aabb.max.z) * 0.5f};

            if (size.x > 0.0f || size.y > 0.0f || size.z > 0.0f)
            {
                Matrix4 world4;
                world4.SetScale(size);
                world4.SetTranslation(center);
                uniformBuffer->SetSubData(&world4, 0, sizeof(Matrix4));

                renderEngine->SetVertexArrayObject(selectionCubeVao);
                renderEngine->DrawLines(selectionCubeVertexCount);
            }
        }

        const bool rotating = selection->mode == GizmoMode::Rotate;
        const VertexArrayObjectPtr& handleVao = rotating ? rotationRingVao : moveHandleVao;
        const unsigned int handleVertexCount = rotating ? rotationRingVertexCount : moveHandleVertexCount;

        if (!handleVao)
            return;

        Matrix4 handleMatrix;
        handleMatrix.SetTranslation(EntityWorldPosition(world, entity));
        uniformBuffer->SetSubData(&handleMatrix, 0, sizeof(Matrix4));

        glDisable(GL_DEPTH_TEST);
        renderEngine->SetVertexArrayObject(handleVao);
        renderEngine->DrawLines(handleVertexCount);

        if (!rotating && moveArrowVao)
        {
            renderEngine->SetVertexArrayObject(moveArrowVao);
            renderEngine->DrawTriangles(TriangleType::List, moveArrowVertexCount, 0);
        }

        glEnable(GL_DEPTH_TEST);
    }

    void BuildRotationRings()
    {
        constexpr int segments = 48;
        constexpr float radius = 1.5f;
        constexpr float twoPi = 6.28318531f;

        const Vector3 colors[3] = {{1.0f, 0.25f, 0.25f}, {0.25f, 1.0f, 0.25f}, {0.35f, 0.55f, 1.0f}};

        std::vector<GizmoVertex> vertices;
        vertices.reserve(segments * 2 * 3);

        for (int axis = 0; axis < 3; axis++)
        {
            for (int i = 0; i < segments; i++)
            {
                const float a0 = (twoPi * i) / segments;
                const float a1 = (twoPi * (i + 1)) / segments;

                vertices.push_back({RingPoint(axis, a0, radius), colors[axis]});
                vertices.push_back({RingPoint(axis, a1, radius), colors[axis]});
            }
        }

        rotationRingVertexCount = static_cast<unsigned int>(vertices.size());
        rotationRingVao = CreateLineVao(vertices);
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

    void BuildMoveHandles()
    {
        constexpr float length = 1.5f;
        std::vector<GizmoVertex> vertices = {
            {{0.0f, 0.0f, 0.0f}, {1.0f, 0.25f, 0.25f}}, {{length, 0.0f, 0.0f}, {1.0f, 0.25f, 0.25f}},
            {{0.0f, 0.0f, 0.0f}, {0.25f, 1.0f, 0.25f}}, {{0.0f, length, 0.0f}, {0.25f, 1.0f, 0.25f}},
            {{0.0f, 0.0f, 0.0f}, {0.35f, 0.55f, 1.0f}}, {{0.0f, 0.0f, length}, {0.35f, 0.55f, 1.0f}},
        };

        moveHandleVertexCount = static_cast<unsigned int>(vertices.size());
        moveHandleVao = CreateLineVao(vertices);

        BuildMoveArrowheads(length);

        const Vector3 highlight = {1.0f, 0.85f, 0.2f};
        const Vector3 corners[8] = {{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
                                    {-0.5f, -0.5f, 0.5f},  {0.5f, -0.5f, 0.5f},  {0.5f, 0.5f, 0.5f},  {-0.5f, 0.5f, 0.5f}};
        const int edges[24] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};

        std::vector<GizmoVertex> selectionVertices;
        selectionVertices.reserve(24);
        for (int i = 0; i < 24; i++)
            selectionVertices.push_back({corners[edges[i]], highlight});

        selectionCubeVertexCount = static_cast<unsigned int>(selectionVertices.size());
        selectionCubeVao = CreateLineVao(selectionVertices);
    }

    void BuildMoveArrowheads(float axisLength)
    {
        constexpr int segments = 10;
        constexpr float coneHeight = 0.35f;
        constexpr float coneRadius = 0.12f;
        constexpr float twoPi = 6.28318531f;

        const Vector3 colors[3] = {{1.0f, 0.25f, 0.25f}, {0.25f, 1.0f, 0.25f}, {0.35f, 0.55f, 1.0f}};

        std::vector<GizmoVertex> vertices;
        vertices.reserve(3 * segments * 3);

        for (int axis = 0; axis < 3; axis++)
        {
            const Vector3 axisDir = AxisUnitVector(axis);
            const Vector3 base = axisDir * axisLength;
            const Vector3 apex = axisDir * (axisLength + coneHeight);

            const Vector3 reference = axis == 1 ? Vector3(1.0f, 0.0f, 0.0f) : Vector3(0.0f, 1.0f, 0.0f);
            const Vector3 u = Vector3::Normalize(Vector3::Cross(axisDir, reference));
            const Vector3 v = Vector3::Cross(axisDir, u);

            for (int i = 0; i < segments; i++)
            {
                const float a0 = (twoPi * i) / segments;
                const float a1 = (twoPi * (i + 1)) / segments;

                const Vector3 p0 = base + (u * std::cos(a0) + v * std::sin(a0)) * coneRadius;
                const Vector3 p1 = base + (u * std::cos(a1) + v * std::sin(a1)) * coneRadius;

                vertices.push_back({apex, colors[axis]});
                vertices.push_back({p0, colors[axis]});
                vertices.push_back({p1, colors[axis]});
            }
        }

        moveArrowVertexCount = static_cast<unsigned int>(vertices.size());
        moveArrowVao = CreateLineVao(vertices);
    }

    static Vector3 AxisUnitVector(int axis)
    {
        if (axis == 0)
            return {1.0f, 0.0f, 0.0f};
        if (axis == 1)
            return {0.0f, 1.0f, 0.0f};
        return {0.0f, 0.0f, 1.0f};
    }

    void DrawCollisionBounds(EcsWorld& world)
    {
        renderEngine->SetVertexArrayObject(wireCubeVao);

        for (auto& [entity, collider] : world.GetPool<ColliderComponent>())
        {
            if (!world.HasComponent<TransformComponent>(entity))
                continue;

            const auto& transform = world.GetComponent<TransformComponent>(entity);
            const Vector3 size = {collider.size.x * transform.scale.x, collider.size.y * transform.scale.y, collider.size.z * transform.scale.z};
            if (size.x <= 0.0f && size.y <= 0.0f && size.z <= 0.0f)
                continue;

            const Vector3 center = transform.position + Vector3(collider.center.x * transform.scale.x, collider.center.y * transform.scale.y,
                                                                collider.center.z * transform.scale.z);

            Matrix4 world4;
            world4.SetScale(size);
            world4.SetTranslation(center);

            uniformBuffer->SetSubData(&world4, 0, sizeof(Matrix4));
            renderEngine->DrawLines(wireCubeVertexCount);
        }
    }

    void BuildGrid()
    {
        std::vector<GizmoVertex> vertices;

        constexpr int halfLines = 500;
        constexpr float spacing = 1.0f;
        const Vector3 minorColor = {0.35f, 0.35f, 0.38f};
        const Vector3 majorColor = {0.5f, 0.5f, 0.55f};
        const float extent = halfLines * spacing;

        for (int i = -halfLines; i <= halfLines; i++)
        {
            const float offset = i * spacing;
            const Vector3 color = (i % 10 == 0) ? majorColor : minorColor;

            vertices.push_back({{offset, 0.0f, -extent}, color});
            vertices.push_back({{offset, 0.0f, extent}, color});
            vertices.push_back({{-extent, 0.0f, offset}, color});
            vertices.push_back({{extent, 0.0f, offset}, color});
        }

        gridVertexCount = static_cast<unsigned int>(vertices.size());
        gridVao = CreateLineVao(vertices);

        std::vector<GizmoVertex> axes = {
            {{0.0f, 0.0f, 0.0f}, {0.9f, 0.2f, 0.2f}}, {{5.0f, 0.0f, 0.0f}, {0.9f, 0.2f, 0.2f}},
            {{0.0f, 0.0f, 0.0f}, {0.2f, 0.9f, 0.2f}}, {{0.0f, 5.0f, 0.0f}, {0.2f, 0.9f, 0.2f}},
            {{0.0f, 0.0f, 0.0f}, {0.3f, 0.5f, 1.0f}}, {{0.0f, 0.0f, 5.0f}, {0.3f, 0.5f, 1.0f}},
        };

        axisVertexCount = static_cast<unsigned int>(axes.size());
        axisVao = CreateLineVao(axes);
    }

    void BuildWireCube()
    {
        const Vector3 color = {0.2f, 1.0f, 0.4f};

        const Vector3 corners[8] = {{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
                                    {-0.5f, -0.5f, 0.5f},  {0.5f, -0.5f, 0.5f},  {0.5f, 0.5f, 0.5f},  {-0.5f, 0.5f, 0.5f}};

        const int edges[24] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};

        std::vector<GizmoVertex> vertices;
        vertices.reserve(24);
        for (int i = 0; i < 24; i++)
            vertices.push_back({corners[edges[i]], color});

        wireCubeVertexCount = static_cast<unsigned int>(vertices.size());
        wireCubeVao = CreateLineVao(vertices);
    }

    VertexArrayObjectPtr CreateLineVao(std::vector<GizmoVertex>& vertices)
    {
        static VertexAttributes attrs[] = {{3}, {3}};
        return renderEngine->CreateVertexArrayObject({static_cast<void*>(vertices.data()), sizeof(GizmoVertex), static_cast<int>(vertices.size()), attrs, 2});
    }

    RenderEngine* renderEngine;
    UniformBufferPtr uniformBuffer;
    ShaderProgramPtr lineShader;
    EditorSelection* selection;

    VertexArrayObjectPtr gridVao;
    VertexArrayObjectPtr axisVao;
    VertexArrayObjectPtr wireCubeVao;
    VertexArrayObjectPtr moveHandleVao;
    VertexArrayObjectPtr moveArrowVao;
    VertexArrayObjectPtr rotationRingVao;
    VertexArrayObjectPtr selectionCubeVao;

    unsigned int gridVertexCount = 0;
    unsigned int axisVertexCount = 0;
    unsigned int wireCubeVertexCount = 0;
    unsigned int moveHandleVertexCount = 0;
    unsigned int moveArrowVertexCount = 0;
    unsigned int rotationRingVertexCount = 0;
    unsigned int selectionCubeVertexCount = 0;
};
