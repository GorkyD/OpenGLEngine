#pragma once

#include <unordered_map>
#include <unordered_set>
#include "Ecs/Components/AnimatorComponent.h"
#include "Ecs/Components/ParentComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/WorldTransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Math/Matrix4.h"

class TransformHierarchySystem : public IEcsSystem
{
public:
    void Run(EcsWorld& world, float deltaTime) override
    {
        resolved.clear();
        inProgress.clear();

        for (auto& [entity, transform] : world.GetPool<TransformComponent>())
            Resolve(world, entity);

        for (const auto& [entity, matrix] : resolved)
        {
            auto& worldTransform = world.HasComponent<WorldTransformComponent>(entity) ? world.GetComponent<WorldTransformComponent>(entity)
                                                                                       : world.AddComponent<WorldTransformComponent>(entity);
            worldTransform.matrix = matrix;
        }
    }

private:
    const Matrix4& Resolve(EcsWorld& world, Entity entity)
    {
        const auto cached = resolved.find(entity);
        if (cached != resolved.end())
            return cached->second;

        if (!inProgress.insert(entity).second)
            return identity;

        Matrix4 local;
        if (world.HasComponent<LocalMatrixComponent>(entity))
            local = world.GetComponent<LocalMatrixComponent>(entity).matrix;
        else if (world.HasComponent<TransformComponent>(entity))
            local = world.GetComponent<TransformComponent>(entity).GetModelMatrix();

        Matrix4 result = local;

        if (world.HasComponent<ParentComponent>(entity))
        {
            const auto& parentComp = world.GetComponent<ParentComponent>(entity);
            if (parentComp.parent != INVALID_ENTITY && world.HasComponent<TransformComponent>(parentComp.parent))
            {
                const Matrix4& parentWorld = Resolve(world, parentComp.parent);
                const Matrix4* bone = FindBoneMatrix(world, parentComp.parent, parentComp.attachBone);

                result = bone ? local * (*bone) * parentWorld : local * parentWorld;
            }
        }

        inProgress.erase(entity);
        return resolved.emplace(entity, result).first->second;
    }

    static const Matrix4* FindBoneMatrix(EcsWorld& world, Entity parent, const std::string& boneName)
    {
        if (boneName.empty() || !world.HasComponent<AnimatorComponent>(parent))
            return nullptr;

        const auto& state = world.GetComponent<AnimatorComponent>(parent).state;
        if (!state)
            return nullptr;

        return state->GetAttachmentTransform(boneName);
    }

    Matrix4 identity;
    std::unordered_map<Entity, Matrix4> resolved;
    std::unordered_set<Entity> inProgress;
};
